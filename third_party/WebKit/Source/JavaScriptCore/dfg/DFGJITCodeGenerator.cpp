/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DFGJITCodeGenerator.h"

#if ENABLE(DFG_JIT)

#include "DFGNonSpeculativeJIT.h"
#include "DFGSpeculativeJIT.h"
#include "LinkBuffer.h"

namespace JSC { namespace DFG {

GPRReg JITCodeGenerator::fillInteger(NodeIndex nodeIndex, DataFormat& returnFormat)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        GPRReg gpr = allocate();

        if (node.isConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(nodeIndex)) {
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                info.fillInteger(gpr);
                returnFormat = DataFormatInteger;
                return gpr;
            }
            if (isDoubleConstant(nodeIndex)) {
                JSValue jsValue = jsNumber(valueOfDoubleConstant(nodeIndex));
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            } else {
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            }
        } else {
            ASSERT(info.spillFormat() == DataFormatJS || info.spillFormat() == DataFormatJSInteger);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
        }

        // Since we statically know that we're filling an integer, and values
        // in the RegisterFile are boxed, this must be DataFormatJSInteger.
        // We will check this with a jitAssert below.
        info.fillJSValue(gpr, DataFormatJSInteger);
        unlock(gpr);
    }

    switch (info.registerFormat()) {
    case DataFormatNone:
        // Should have filled, above.
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJS:
    case DataFormatCell:
    case DataFormatJSCell:
        // Should only be calling this function if we know this operand to be integer.
        ASSERT_NOT_REACHED();

    case DataFormatJSInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.jitAssertIsJSInt32(gpr);
        returnFormat = DataFormatJSInteger;
        return gpr;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.jitAssertIsInt32(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

FPRReg JITCodeGenerator::fillDouble(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        GPRReg gpr = allocate();

        if (node.isConstant()) {
            if (isInt32Constant(nodeIndex)) {
                // FIXME: should not be reachable?
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                info.fillInteger(gpr);
                unlock(gpr);
            } else if (isDoubleConstant(nodeIndex)) {
                FPRReg fpr = fprAllocate();
                m_jit.move(MacroAssembler::ImmPtr(reinterpret_cast<void*>(reinterpretDoubleToIntptr(valueOfDoubleConstant(nodeIndex)))), gpr);
                m_jit.movePtrToDouble(gpr, fpr);
                unlock(gpr);

                m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
                info.fillDouble(fpr);
                return fpr;
            } else {
                // FIXME: should not be reachable?
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                info.fillJSValue(gpr, DataFormatJS);
                unlock(gpr);
            }
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(gpr, m_isSpeculative ? spillFormat : DataFormatJS);
            unlock(gpr);
        }
    }

    switch (info.registerFormat()) {
    case DataFormatNone:
        // Should have filled, above.
    case DataFormatCell:
    case DataFormatJSCell:
        // Should only be calling this function if we know this operand to be numeric.
        ASSERT_NOT_REACHED();

    case DataFormatJS: {
        GPRReg jsValueGpr = info.gpr();
        m_gprs.lock(jsValueGpr);
        FPRReg fpr = fprAllocate();
        GPRReg tempGpr = allocate(); // FIXME: can we skip this allocation on the last use of the virtual register?

        JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);

        m_jit.jitAssertIsJSDouble(jsValueGpr);

        // First, if we get here we have a double encoded as a JSValue
        m_jit.move(jsValueGpr, tempGpr);
        m_jit.addPtr(GPRInfo::tagTypeNumberRegister, tempGpr);
        m_jit.movePtrToDouble(tempGpr, fpr);
        JITCompiler::Jump hasUnboxedDouble = m_jit.jump();

        // Finally, handle integers.
        isInteger.link(&m_jit);
        m_jit.convertInt32ToDouble(jsValueGpr, fpr);
        hasUnboxedDouble.link(&m_jit);

        m_gprs.release(jsValueGpr);
        m_gprs.unlock(jsValueGpr);
        m_gprs.unlock(tempGpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
        info.fillDouble(fpr);
        return fpr;
    }

    case DataFormatJSInteger:
    case DataFormatInteger: {
        FPRReg fpr = fprAllocate();
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.convertInt32ToDouble(gpr, fpr);
        m_gprs.unlock(gpr);
        return fpr;
    }

    // Unbox the double
    case DataFormatJSDouble: {
        GPRReg gpr = info.gpr();
        FPRReg fpr = unboxDouble(gpr);

        m_gprs.release(gpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);

        info.fillDouble(fpr);
        return fpr;
    }

    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        m_fprs.lock(fpr);
        return fpr;
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidFPRReg;
}

GPRReg JITCodeGenerator::fillJSValue(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (node.isConstant()) {
            if (isInt32Constant(nodeIndex)) {
                info.fillJSValue(gpr, DataFormatJSInteger);
                JSValue jsValue = jsNumber(valueOfInt32Constant(nodeIndex));
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            } else if (isDoubleConstant(nodeIndex)) {
                info.fillJSValue(gpr, DataFormatJSDouble);
                JSValue jsValue(JSValue::EncodeAsDouble, valueOfDoubleConstant(nodeIndex));
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            } else {
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
                info.fillJSValue(gpr, DataFormatJS);
            }

            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(gpr, m_isSpeculative ? spillFormat : DataFormatJS);
        }
        return gpr;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInteger, not DataFormatJSInteger.
        if (m_gprs.isLocked(gpr)) {
            GPRReg result = allocate();
            m_jit.orPtr(GPRInfo::tagTypeNumberRegister, gpr, result);
            return result;
        }
        m_gprs.lock(gpr);
        m_jit.orPtr(GPRInfo::tagTypeNumberRegister, gpr);
        info.fillJSValue(gpr, DataFormatJSInteger);
        return gpr;
    }

    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        GPRReg gpr = boxDouble(fpr);

        // Update all info
        info.fillJSValue(gpr, DataFormatJSDouble);
        m_fprs.release(fpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderJS);

        return gpr;
    }

    case DataFormatCell:
        // No retag required on JSVALUE64!
    case DataFormatJS:
    case DataFormatJSInteger:
    case DataFormatJSDouble:
    case DataFormatJSCell: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

void JITCodeGenerator::useChildren(Node& node)
{
    if (node.op & NodeHasVarArgs) {
        for (unsigned childIdx = node.firstChild(); childIdx < node.firstChild() + node.numChildren(); childIdx++)
            use(m_jit.graph().m_varArgChildren[childIdx]);
    } else {
        NodeIndex child1 = node.child1();
        if (child1 == NoNode) {
            ASSERT(node.child2() == NoNode && node.child3() == NoNode);
            return;
        }
        use(child1);
        
        NodeIndex child2 = node.child2();
        if (child2 == NoNode) {
            ASSERT(node.child3() == NoNode);
            return;
        }
        use(child2);
        
        NodeIndex child3 = node.child3();
        if (child3 == NoNode)
            return;
        use(child3);
    }
}

JITCompiler::Call JITCodeGenerator::cachedGetById(GPRReg baseGPR, GPRReg resultGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget, NodeType nodeType)
{
    GPRReg scratchGPR;
    
    if (resultGPR == baseGPR)
        scratchGPR = tryAllocate();
    else
        scratchGPR = resultGPR;
    
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::Jump structureCheck = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::offsetOfPropertyStorage()), resultGPR);
    JITCompiler::DataLabelCompact loadWithPatch = m_jit.loadPtrWithCompactAddressOffsetPatch(JITCompiler::Address(resultGPR, 0), resultGPR);
    
    JITCompiler::Jump done = m_jit.jump();

    structureCheck.link(&m_jit);
    
    if (slowPathTarget.isSet())
        slowPathTarget.link(&m_jit);
    
    JITCompiler::Label slowCase = m_jit.label();

    silentSpillAllRegisters(resultGPR, baseGPR);
    m_jit.move(baseGPR, GPRInfo::argumentGPR1);
    m_jit.move(JITCompiler::ImmPtr(identifier(identifierNumber)), GPRInfo::argumentGPR2);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    JITCompiler::Call functionCall;
    switch (nodeType) {
    case GetById:
        functionCall = appendCallWithExceptionCheck(operationGetByIdOptimize);
        break;
        
    case GetMethod:
        functionCall = appendCallWithExceptionCheck(operationGetMethodOptimize);
        break;
        
    default:
        ASSERT_NOT_REACHED();
        return JITCompiler::Call();
    }
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    silentFillAllRegisters(resultGPR);
    
    done.link(&m_jit);
    
    JITCompiler::Label doneLabel = m_jit.label();

    int8_t checkImmToCall = static_cast<int8_t>(m_jit.differenceBetween(structureToCompare, functionCall));
    int8_t callToCheck = static_cast<int8_t>(m_jit.differenceBetween(functionCall, structureCheck));
    int8_t callToLoad = static_cast<int8_t>(m_jit.differenceBetween(functionCall, loadWithPatch));
    int8_t callToSlowCase = static_cast<int8_t>(m_jit.differenceBetween(functionCall, slowCase));
    int8_t callToDone = static_cast<int8_t>(m_jit.differenceBetween(functionCall, doneLabel));
    
    m_jit.addPropertyAccess(functionCall, checkImmToCall, callToCheck, callToLoad, callToSlowCase, callToDone, static_cast<int8_t>(baseGPR), static_cast<int8_t>(resultGPR), static_cast<int8_t>(scratchGPR));
    
    if (scratchGPR != resultGPR && scratchGPR != InvalidGPRReg)
        unlock(scratchGPR);
    
    return functionCall;
}

void JITCodeGenerator::writeBarrier(MacroAssembler&, GPRReg owner, GPRReg scratch)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch);
    ASSERT(owner != scratch);
}

void JITCodeGenerator::cachedPutById(GPRReg baseGPR, GPRReg valueGPR, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::Jump structureCheck = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    writeBarrier(m_jit, baseGPR, scratchGPR);

    m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::offsetOfPropertyStorage()), scratchGPR);
    JITCompiler::DataLabel32 storeWithPatch = m_jit.storePtrWithAddressOffsetPatch(valueGPR, JITCompiler::Address(scratchGPR, 0));

    JITCompiler::Jump done = m_jit.jump();

    structureCheck.link(&m_jit);

    if (slowPathTarget.isSet())
        slowPathTarget.link(&m_jit);

    JITCompiler::Label slowCase = m_jit.label();

    silentSpillAllRegisters(InvalidGPRReg, baseGPR, valueGPR);
    setupTwoStubArgs<GPRInfo::argumentGPR1, GPRInfo::argumentGPR2>(valueGPR, baseGPR);
    m_jit.move(JITCompiler::ImmPtr(identifier(identifierNumber)), GPRInfo::argumentGPR3);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    V_DFGOperation_EJJI optimizedCall;
    if (m_jit.codeBlock()->isStrictMode()) {
        if (putKind == Direct)
            optimizedCall = operationPutByIdDirectStrictOptimize;
        else
            optimizedCall = operationPutByIdStrictOptimize;
    } else {
        if (putKind == Direct)
            optimizedCall = operationPutByIdDirectNonStrictOptimize;
        else
            optimizedCall = operationPutByIdNonStrictOptimize;
    }
    JITCompiler::Call functionCall = appendCallWithExceptionCheck(optimizedCall);
    silentFillAllRegisters(InvalidGPRReg);

    done.link(&m_jit);
    JITCompiler::Label doneLabel = m_jit.label();

    int8_t checkImmToCall = static_cast<int8_t>(m_jit.differenceBetween(structureToCompare, functionCall));
    int8_t callToCheck = static_cast<int8_t>(m_jit.differenceBetween(functionCall, structureCheck));
    int8_t callToStore = static_cast<int8_t>(m_jit.differenceBetween(functionCall, storeWithPatch));
    int8_t callToSlowCase = static_cast<int8_t>(m_jit.differenceBetween(functionCall, slowCase));
    int8_t callToDone = static_cast<int8_t>(m_jit.differenceBetween(functionCall, doneLabel));

    m_jit.addPropertyAccess(functionCall, checkImmToCall, callToCheck, callToStore, callToSlowCase, callToDone, static_cast<int8_t>(baseGPR), static_cast<int8_t>(valueGPR), static_cast<int8_t>(scratchGPR));
}

void JITCodeGenerator::cachedGetMethod(GPRReg baseGPR, GPRReg resultGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget)
{
    JITCompiler::Call slowCall;
    JITCompiler::DataLabelPtr structToCompare, protoObj, protoStructToCompare, putFunction;
    
    JITCompiler::Jump wrongStructure = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    protoObj = m_jit.moveWithPatch(JITCompiler::TrustedImmPtr(0), resultGPR);
    JITCompiler::Jump wrongProtoStructure = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(resultGPR, JSCell::structureOffset()), protoStructToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    putFunction = m_jit.moveWithPatch(JITCompiler::TrustedImmPtr(0), resultGPR);
    
    JITCompiler::Jump done = m_jit.jump();
    
    wrongStructure.link(&m_jit);
    wrongProtoStructure.link(&m_jit);
    
    slowCall = cachedGetById(baseGPR, resultGPR, identifierNumber, slowPathTarget, GetMethod);
    
    done.link(&m_jit);
    
    m_jit.addMethodGet(slowCall, structToCompare, protoObj, protoStructToCompare, putFunction);
}

void JITCodeGenerator::emitCall(Node& node)
{
    P_DFGOperation_E slowCallFunction;
    bool isCall;
    
    if (node.op == Call) {
        slowCallFunction = operationLinkCall;
        isCall = true;
    } else {
        ASSERT(node.op == Construct);
        slowCallFunction = operationLinkConstruct;
        isCall = false;
    }
    
    NodeIndex calleeNodeIndex = m_jit.graph().m_varArgChildren[node.firstChild()];
    JSValueOperand callee(this, calleeNodeIndex);
    GPRReg calleeGPR = callee.gpr();
    use(calleeNodeIndex);
    
    // the call instruction's first child is either the function (normal call) or the
    // receiver (method call). subsequent children are the arguments.
    int numArgs = node.numChildren() - 1;
    
    // amount of stuff (in units of sizeof(Register)) that we need to place at the
    // top of the JS stack.
    int callDataSize = 0;

    // first there are the arguments
    callDataSize += numArgs;
    
    // and then there is the call frame header
    callDataSize += RegisterFile::CallFrameHeaderSize;
    
    m_jit.storePtr(MacroAssembler::TrustedImmPtr(JSValue::encode(jsNumber(numArgs))), addressOfCallData(RegisterFile::ArgumentCount));
    m_jit.storePtr(GPRInfo::callFrameRegister, addressOfCallData(RegisterFile::CallerFrame));
    
    for (int argIdx = 0; argIdx < numArgs; argIdx++) {
        NodeIndex argNodeIndex = m_jit.graph().m_varArgChildren[node.firstChild() + 1 + argIdx];
        JSValueOperand arg(this, argNodeIndex);
        GPRReg argGPR = arg.gpr();
        use(argNodeIndex);
        
        m_jit.storePtr(argGPR, addressOfCallData(-callDataSize + argIdx));
    }
    
    m_jit.storePtr(calleeGPR, addressOfCallData(RegisterFile::Callee));
    
    flushRegisters();
    
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::Jump slowPath;
    
    slowPath = m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleeGPR, targetToCheck, MacroAssembler::TrustedImmPtr(JSValue::encode(JSValue())));
    m_jit.loadPtr(MacroAssembler::Address(calleeGPR, OBJECT_OFFSETOF(JSFunction, m_scopeChain)), resultGPR);
    m_jit.storePtr(resultGPR, addressOfCallData(RegisterFile::ScopeChain));

    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    
    JITCompiler::Call fastCall = m_jit.nearCall();
    m_jit.notifyCall(fastCall, m_jit.graph()[m_compileIndex].exceptionInfo);
    
    JITCompiler::Jump done = m_jit.jump();
    
    slowPath.link(&m_jit);
    
    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    JITCompiler::Call slowCall = m_jit.appendCallWithFastExceptionCheck(slowCallFunction, m_jit.graph()[m_compileIndex].exceptionInfo);
    m_jit.move(Imm32(numArgs), GPRInfo::regT1);
    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    m_jit.notifyCall(m_jit.call(GPRInfo::returnValueGPR), m_jit.graph()[m_compileIndex].exceptionInfo);
    
    done.link(&m_jit);
    
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    
    jsValueResult(resultGPR, m_compileIndex, DataFormatJS, UseChildrenCalledExplicitly);
    
    m_jit.addJSCall(fastCall, slowCall, targetToCheck, isCall, m_jit.graph()[m_compileIndex].exceptionInfo);
}

#ifndef NDEBUG
static const char* dataFormatString(DataFormat format)
{
    // These values correspond to the DataFormat enum.
    const char* strings[] = {
        "[  ]",
        "[ i]",
        "[ d]",
        "[ c]",
        "Err!",
        "Err!",
        "Err!",
        "Err!",
        "[J ]",
        "[Ji]",
        "[Jd]",
        "[Jc]",
        "Err!",
        "Err!",
        "Err!",
        "Err!",
    };
    return strings[format];
}

void JITCodeGenerator::dump(const char* label)
{
    if (label)
        fprintf(stderr, "<%s>\n", label);

    fprintf(stderr, "  gprs:\n");
    m_gprs.dump();
    fprintf(stderr, "  fprs:\n");
    m_fprs.dump();
    fprintf(stderr, "  VirtualRegisters:\n");
    for (unsigned i = 0; i < m_generationInfo.size(); ++i) {
        GenerationInfo& info = m_generationInfo[i];
        if (info.alive())
            fprintf(stderr, "    % 3d:%s%s", i, dataFormatString(info.registerFormat()), dataFormatString(info.spillFormat()));
        else
            fprintf(stderr, "    % 3d:[__][__]", i);
        if (info.registerFormat() == DataFormatDouble)
            fprintf(stderr, ":fpr%d\n", info.fpr());
        else if (info.registerFormat() != DataFormatNone) {
            ASSERT(info.gpr() != InvalidGPRReg);
            fprintf(stderr, ":%s\n", GPRInfo::debugName(info.gpr()));
        } else
            fprintf(stderr, "\n");
    }
    if (label)
        fprintf(stderr, "</%s>\n", label);
}
#endif


#if DFG_CONSISTENCY_CHECK
void JITCodeGenerator::checkConsistency()
{
    bool failed = false;

    for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
        if (iter.isLocked()) {
            fprintf(stderr, "DFG_CONSISTENCY_CHECK failed: gpr %s is locked.\n", iter.debugName());
            failed = true;
        }
    }
    for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
        if (iter.isLocked()) {
            fprintf(stderr, "DFG_CONSISTENCY_CHECK failed: fpr %s is locked.\n", iter.debugName());
            failed = true;
        }
    }

    for (unsigned i = 0; i < m_generationInfo.size(); ++i) {
        VirtualRegister virtualRegister = (VirtualRegister)i;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (!info.alive())
            continue;
        switch (info.registerFormat()) {
        case DataFormatNone:
            break;
        case DataFormatInteger:
        case DataFormatCell:
        case DataFormatJS:
        case DataFormatJSInteger:
        case DataFormatJSDouble:
        case DataFormatJSCell: {
            GPRReg gpr = info.gpr();
            ASSERT(gpr != InvalidGPRReg);
            if (m_gprs.name(gpr) != virtualRegister) {
                fprintf(stderr, "DFG_CONSISTENCY_CHECK failed: name mismatch for virtual register %d (gpr %s).\n", virtualRegister, GPRInfo::debugName(gpr));
                failed = true;
            }
            break;
        }
        case DataFormatDouble: {
            FPRReg fpr = info.fpr();
            ASSERT(fpr != InvalidFPRReg);
            if (m_fprs.name(fpr) != virtualRegister) {
                fprintf(stderr, "DFG_CONSISTENCY_CHECK failed: name mismatch for virtual register %d (fpr %s).\n", virtualRegister, FPRInfo::debugName(fpr));
                failed = true;
            }
            break;
        }
        }
    }

    for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
        VirtualRegister virtualRegister = iter.name();
        if (virtualRegister == InvalidVirtualRegister)
            continue;

        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (iter.regID() != info.gpr()) {
            fprintf(stderr, "DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
            failed = true;
        }
    }

    for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
        VirtualRegister virtualRegister = iter.name();
        if (virtualRegister == InvalidVirtualRegister)
            continue;

        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (iter.regID() != info.fpr()) {
            fprintf(stderr, "DFG_CONSISTENCY_CHECK failed: name mismatch for fpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
            failed = true;
        }
    }

    if (failed) {
        dump();
        CRASH();
    }
}
#endif

GPRTemporary::GPRTemporary(JITCodeGenerator* jit)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, GPRReg specific)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    m_gpr = m_jit->allocate(specific);
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, SpeculateIntegerOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, SpeculateIntegerOperand& op1, SpeculateIntegerOperand& op2)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else if (m_jit->canReuse(op2.index()))
        m_gpr = m_jit->reuse(op2.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, IntegerOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, IntegerOperand& op1, IntegerOperand& op2)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else if (m_jit->canReuse(op2.index()))
        m_gpr = m_jit->reuse(op2.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, SpeculateCellOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(JITCodeGenerator* jit, JSValueOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

FPRTemporary::FPRTemporary(JITCodeGenerator* jit)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(JITCodeGenerator* jit, DoubleOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(JITCodeGenerator* jit, DoubleOperand& op1, DoubleOperand& op2)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else if (m_jit->canReuse(op2.index()))
        m_fpr = m_jit->reuse(op2.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(JITCodeGenerator* jit, SpeculateDoubleOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(JITCodeGenerator* jit, SpeculateDoubleOperand& op1, SpeculateDoubleOperand& op2)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else if (m_jit->canReuse(op2.index()))
        m_fpr = m_jit->reuse(op2.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

} } // namespace JSC::DFG

#endif
