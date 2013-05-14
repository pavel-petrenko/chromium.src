/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "modules/webaudio/OscillatorNode.h"

#include "core/dom/ExceptionCode.h"
#include "core/platform/audio/AudioUtilities.h"
#include "core/platform/audio/VectorMath.h"
#include "modules/webaudio/AudioContext.h"
#include "modules/webaudio/AudioNodeOutput.h"
#include "modules/webaudio/WaveTable.h"
#include <algorithm>
#include "wtf/MathExtras.h"

using namespace std;

namespace WebCore {

using namespace VectorMath;

WaveTable* OscillatorNode::s_waveTableSine = 0;
WaveTable* OscillatorNode::s_waveTableSquare = 0;
WaveTable* OscillatorNode::s_waveTableSawtooth = 0;
WaveTable* OscillatorNode::s_waveTableTriangle = 0;

PassRefPtr<OscillatorNode> OscillatorNode::create(AudioContext* context, float sampleRate)
{
    return adoptRef(new OscillatorNode(context, sampleRate));
}

OscillatorNode::OscillatorNode(AudioContext* context, float sampleRate)
    : AudioScheduledSourceNode(context, sampleRate)
    , m_type(SINE)
    , m_firstRender(true)
    , m_virtualReadIndex(0)
    , m_phaseIncrements(AudioNode::ProcessingSizeInFrames)
    , m_detuneValues(AudioNode::ProcessingSizeInFrames)
{
    ScriptWrappable::init(this);
    setNodeType(NodeTypeOscillator);

    // Use musical pitch standard A440 as a default.
    m_frequency = AudioParam::create(context, "frequency", 440, 0, 100000);
    // Default to no detuning.
    m_detune = AudioParam::create(context, "detune", 0, -4800, 4800);

    // Sets up default wavetable.
    setType(m_type);

    // An oscillator is always mono.
    addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

    initialize();
}

OscillatorNode::~OscillatorNode()
{
    uninitialize();
}

String OscillatorNode::type() const
{
    switch (m_type) {
    case SINE:
        return "sine";
    case SQUARE:
        return "square";
    case SAWTOOTH:
        return "sawtooth";
    case TRIANGLE:
        return "triangle";
    case CUSTOM:
        return "custom";
    default:
        ASSERT_NOT_REACHED();
        return "custom";
    }
}

void OscillatorNode::setType(const String& type)
{
    if (type == "sine")
        setType(SINE);
    else if (type == "square")
        setType(SQUARE);
    else if (type == "sawtooth")
        setType(SAWTOOTH);
    else if (type == "triangle")
        setType(TRIANGLE);
    else
        ASSERT_NOT_REACHED();
}

bool OscillatorNode::setType(unsigned type)
{
    WaveTable* waveTable = 0;
    float sampleRate = this->sampleRate();

    switch (type) {
    case SINE:
        if (!s_waveTableSine)
            s_waveTableSine = WaveTable::createSine(sampleRate).leakRef();
        waveTable = s_waveTableSine;
        break;
    case SQUARE:
        if (!s_waveTableSquare)
            s_waveTableSquare = WaveTable::createSquare(sampleRate).leakRef();
        waveTable = s_waveTableSquare;
        break;
    case SAWTOOTH:
        if (!s_waveTableSawtooth)
            s_waveTableSawtooth = WaveTable::createSawtooth(sampleRate).leakRef();
        waveTable = s_waveTableSawtooth;
        break;
    case TRIANGLE:
        if (!s_waveTableTriangle)
            s_waveTableTriangle = WaveTable::createTriangle(sampleRate).leakRef();
        waveTable = s_waveTableTriangle;
        break;
    case CUSTOM:
    default:
        // Return error for invalid types, including CUSTOM since setWaveTable() method must be
        // called explicitly.
        return false;
    }

    setWaveTable(waveTable);
    m_type = type;
    return true;
}

bool OscillatorNode::calculateSampleAccuratePhaseIncrements(size_t framesToProcess)
{
    bool isGood = framesToProcess <= m_phaseIncrements.size() && framesToProcess <= m_detuneValues.size();
    ASSERT(isGood);
    if (!isGood)
        return false;

    if (m_firstRender) {
        m_firstRender = false;
        m_frequency->resetSmoothedValue();
        m_detune->resetSmoothedValue();
    }

    bool hasSampleAccurateValues = false;
    bool hasFrequencyChanges = false;
    float* phaseIncrements = m_phaseIncrements.data();

    float finalScale = m_waveTable->rateScale();

    if (m_frequency->hasSampleAccurateValues()) {
        hasSampleAccurateValues = true;
        hasFrequencyChanges = true;

        // Get the sample-accurate frequency values and convert to phase increments.
        // They will be converted to phase increments below.
        m_frequency->calculateSampleAccurateValues(phaseIncrements, framesToProcess);
    } else {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_frequency->smooth();
        float frequency = m_frequency->smoothedValue();
        finalScale *= frequency;
    }

    if (m_detune->hasSampleAccurateValues()) {
        hasSampleAccurateValues = true;

        // Get the sample-accurate detune values.
        float* detuneValues = hasFrequencyChanges ? m_detuneValues.data() : phaseIncrements;
        m_detune->calculateSampleAccurateValues(detuneValues, framesToProcess);

        // Convert from cents to rate scalar.
        float k = 1.0 / 1200;
        vsmul(detuneValues, 1, &k, detuneValues, 1, framesToProcess);
        for (unsigned i = 0; i < framesToProcess; ++i)
            detuneValues[i] = powf(2, detuneValues[i]); // FIXME: converting to expf() will be faster.

        if (hasFrequencyChanges) {
            // Multiply frequencies by detune scalings.
            vmul(detuneValues, 1, phaseIncrements, 1, phaseIncrements, 1, framesToProcess);
        }
    } else {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_detune->smooth();
        float detune = m_detune->smoothedValue();
        float detuneScale = powf(2, detune / 1200);
        finalScale *= detuneScale;
    }

    if (hasSampleAccurateValues) {
        // Convert from frequency to wavetable increment.
        vsmul(phaseIncrements, 1, &finalScale, phaseIncrements, 1, framesToProcess);
    }

    return hasSampleAccurateValues;
}

void OscillatorNode::process(size_t framesToProcess)
{
    AudioBus* outputBus = output(0)->bus();

    if (!isInitialized() || !outputBus->numberOfChannels()) {
        outputBus->zero();
        return;
    }

    ASSERT(framesToProcess <= m_phaseIncrements.size());
    if (framesToProcess > m_phaseIncrements.size())
        return;

    // The audio thread can't block on this lock, so we call tryLock() instead.
    MutexTryLocker tryLocker(m_processLock);
    if (!tryLocker.locked()) {
        // Too bad - the tryLock() failed. We must be in the middle of changing wave-tables.
        outputBus->zero();
        return;
    }

    // We must access m_waveTable only inside the lock.
    if (!m_waveTable.get()) {
        outputBus->zero();
        return;
    }

    size_t quantumFrameOffset;
    size_t nonSilentFramesToProcess;

    updateSchedulingInfo(framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

    if (!nonSilentFramesToProcess) {
        outputBus->zero();
        return;
    }

    unsigned waveTableSize = m_waveTable->waveTableSize();
    double invWaveTableSize = 1.0 / waveTableSize;

    float* destP = outputBus->channel(0)->mutableData();

    ASSERT(quantumFrameOffset <= framesToProcess);

    // We keep virtualReadIndex double-precision since we're accumulating values.
    double virtualReadIndex = m_virtualReadIndex;

    float rateScale = m_waveTable->rateScale();
    float invRateScale = 1 / rateScale;
    bool hasSampleAccurateValues = calculateSampleAccuratePhaseIncrements(framesToProcess);

    float frequency = 0;
    float* higherWaveData = 0;
    float* lowerWaveData = 0;
    float tableInterpolationFactor;

    if (!hasSampleAccurateValues) {
        frequency = m_frequency->smoothedValue();
        float detune = m_detune->smoothedValue();
        float detuneScale = powf(2, detune / 1200);
        frequency *= detuneScale;
        m_waveTable->waveDataForFundamentalFrequency(frequency, lowerWaveData, higherWaveData, tableInterpolationFactor);
    }

    float incr = frequency * rateScale;
    float* phaseIncrements = m_phaseIncrements.data();

    unsigned readIndexMask = waveTableSize - 1;

    // Start rendering at the correct offset.
    destP += quantumFrameOffset;
    int n = nonSilentFramesToProcess;

    while (n--) {
        unsigned readIndex = static_cast<unsigned>(virtualReadIndex);
        unsigned readIndex2 = readIndex + 1;

        // Contain within valid range.
        readIndex = readIndex & readIndexMask;
        readIndex2 = readIndex2 & readIndexMask;

        if (hasSampleAccurateValues) {
            incr = *phaseIncrements++;

            frequency = invRateScale * incr;
            m_waveTable->waveDataForFundamentalFrequency(frequency, lowerWaveData, higherWaveData, tableInterpolationFactor);
        }

        float sample1Lower = lowerWaveData[readIndex];
        float sample2Lower = lowerWaveData[readIndex2];
        float sample1Higher = higherWaveData[readIndex];
        float sample2Higher = higherWaveData[readIndex2];

        // Linearly interpolate within each table (lower and higher).
        float interpolationFactor = static_cast<float>(virtualReadIndex) - readIndex;
        float sampleHigher = (1 - interpolationFactor) * sample1Higher + interpolationFactor * sample2Higher;
        float sampleLower = (1 - interpolationFactor) * sample1Lower + interpolationFactor * sample2Lower;

        // Then interpolate between the two tables.
        float sample = (1 - tableInterpolationFactor) * sampleHigher + tableInterpolationFactor * sampleLower;

        *destP++ = sample;

        // Increment virtual read index and wrap virtualReadIndex into the range 0 -> waveTableSize.
        virtualReadIndex += incr;
        virtualReadIndex -= floor(virtualReadIndex * invWaveTableSize) * waveTableSize;
    }

    m_virtualReadIndex = virtualReadIndex;

    outputBus->clearSilentFlag();
}

void OscillatorNode::reset()
{
    m_virtualReadIndex = 0;
}

void OscillatorNode::setWaveTable(WaveTable* waveTable)
{
    ASSERT(isMainThread());

    // This synchronizes with process().
    MutexLocker processLocker(m_processLock);
    m_waveTable = waveTable;
    m_type = CUSTOM;
}

bool OscillatorNode::propagatesSilence() const
{
    return !isPlayingOrScheduled() || hasFinished() || !m_waveTable.get();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
