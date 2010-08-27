/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef DecimalNumber_h
#define DecimalNumber_h

#include <math.h>
#include <wtf/dtoa.h>
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

namespace WTF {

enum RoundingSignificantFiguresType { RoundingSignificantFigures };
enum RoundingDecimalPlacesType { RoundingDecimalPlaces };

class DecimalNumber {
public:
    DecimalNumber(double d)
    {
        ASSERT(!isnan(d) && !isinf(d));
        dtoa(m_significand, d, m_sign, m_exponent, m_precision);

        ASSERT(m_precision);
        // Zero should always have exponent 0.
        ASSERT(m_significand[0] != '0' || !m_exponent);
        // No values other than zero should have a leading zero.
        ASSERT(m_significand[0] != '0' || m_precision == 1);
        // No values other than zero should have trailing zeros.
        ASSERT(m_significand[0] == '0' || m_significand[m_precision - 1] != '0');
    }

    DecimalNumber(double d, RoundingSignificantFiguresType, unsigned significantFigures)
    {
        ASSERT(!isnan(d) && !isinf(d));
        dtoaRoundSF(m_significand, d, significantFigures, m_sign, m_exponent, m_precision);

        ASSERT(significantFigures && significantFigures <= sizeof(DtoaBuffer));
        while (m_precision < significantFigures)
            m_significand[m_precision++] = '0';

        ASSERT(m_precision);
        // Zero should always have exponent 0.
        ASSERT(m_significand[0] != '0' || !m_exponent);
    }

    DecimalNumber(double d, RoundingDecimalPlacesType, unsigned decimalPlaces)
    {
        ASSERT(!isnan(d) && !isinf(d));
        dtoaRoundDP(m_significand, d, decimalPlaces, m_sign, m_exponent, m_precision);

        unsigned significantFigures = 1 + m_exponent + decimalPlaces;
        ASSERT(significantFigures && significantFigures <= sizeof(DtoaBuffer));
        while (m_precision < significantFigures)
            m_significand[m_precision++] = '0';

        ASSERT(m_precision);
        // Zero should always have exponent 0.
        ASSERT(m_significand[0] != '0' || !m_exponent);
    }

    unsigned toStringDecimal(NumberToStringBuffer buffer)
    {
        // Should always be at least one digit to add to the string!
        ASSERT(m_precision);
        UChar* next = buffer;

        // if the exponent is negative the number decimal representation is of the form:
        // [<sign>]0.[<zeros>]<significand>
        if (m_exponent < 0) {
            unsigned zeros = -m_exponent - 1;

            if (m_sign)
                *next++ = '-';
            *next++ = '0';
            *next++ = '.';
            for (unsigned i = 0; i < zeros; ++i)
                *next++ = '0';
            for (unsigned i = 0; i < m_precision; ++i)
                *next++ = m_significand[i];

            return next - buffer;
        }

        unsigned digitsBeforeDecimalPoint = m_exponent + 1;

        // If the precision is <= than the number of digits to get up to the decimal
        // point, then there is no fractional part, number is of the form:
        // [<sign>]<significand>[<zeros>]
        if (m_precision <= digitsBeforeDecimalPoint) {
            if (m_sign)
                *next++ = '-';
            for (unsigned i = 0; i < m_precision; ++i)
                *next++ = m_significand[i];
            for (unsigned i = 0; i < (digitsBeforeDecimalPoint - m_precision); ++i)
                *next++ = '0';

            return next - buffer;
        }

        // If we get here, number starts before the decimal point, and ends after it,
        // as such is of the form:
        // [<sign>]<significand-begin>.<significand-end>

        if (m_sign)
            *next++ = '-';
        for (unsigned i = 0; i < digitsBeforeDecimalPoint; ++i)
            *next++ = m_significand[i];
        *next++ = '.';
        for (unsigned i = digitsBeforeDecimalPoint; i < m_precision; ++i)
            *next++ = m_significand[i];

        return next - buffer;
    }

    unsigned toStringExponential(NumberToStringBuffer buffer)
    {
        // Should always be at least one digit to add to the string!
        ASSERT(m_precision);

        UChar* next = buffer;

        // Add the sign
        if (m_sign)
            *next++ = '-';

        // Add the significand
        *next++ = m_significand[0];
        if (m_precision > 1) {
            *next++ = '.';
            for (unsigned i = 1; i < m_precision; ++i)
                *next++ = m_significand[i];
        }

        // Add "e+" or "e-"
        *next++ = 'e';
        int exponent;
        if (m_exponent >= 0) {
            *next++ = '+';
            exponent = m_exponent;
        } else {
            *next++ = '-';
            exponent = -m_exponent;
        }

        // Add the exponent
        if (exponent >= 100)
            *next++ = '0' + exponent / 100;
        if (exponent >= 10)
            *next++ = '0' + (exponent % 100) / 10;
        *next++ = '0' + exponent % 10;

        return next - buffer;
    }

    bool sign() { return m_sign; }
    int exponent() { return m_exponent; }
    const char* significand() { return m_significand; } // significand contains precision characters, is not null-terminated.
    unsigned precision() { return m_precision; }

private:
    bool m_sign;
    int m_exponent;
    DtoaBuffer m_significand;
    unsigned m_precision;
};

} // namespace WTF

using WTF::DecimalNumber;
using WTF::RoundingSignificantFigures;
using WTF::RoundingDecimalPlaces;

#endif // DecimalNumber_h
