/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/animation/AnimatableLength.h"

#include "core/css/CSSCalculationValue.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/platform/CalculationValue.h"
#include "core/rendering/style/RenderStyle.h"
#include "core/rendering/style/StyleInheritedData.h"
#include "wtf/MathExtras.h"

#include <gtest/gtest.h>

using namespace WebCore;

namespace {

class AnimatableLengthTest : public ::testing::Test {
protected:
    virtual void SetUp()
    {
        style = RenderStyle::createDefaultStyle();
    }

    PassRefPtr<AnimatableLength> create(double value, CSSPrimitiveValue::UnitTypes type)
    {
        return AnimatableLength::create(CSSPrimitiveValue::create(value, type).get());
    }

    PassRefPtr<AnimatableLength> create(double valueLeft, CSSPrimitiveValue::UnitTypes typeLeft, double valueRight, CSSPrimitiveValue::UnitTypes typeRight)
    {
        return AnimatableLength::create(createCalc(valueLeft, typeLeft, valueRight, typeRight).get());
    }

    PassRefPtr<CSSCalcValue> createCalc(double valueLeft, CSSPrimitiveValue::UnitTypes typeLeft, double valueRight, CSSPrimitiveValue::UnitTypes typeRight)
    {
        return CSSCalcValue::create(CSSCalcValue::createExpressionNode(
            CSSCalcValue::createExpressionNode(CSSPrimitiveValue::create(valueLeft, typeLeft), valueLeft == trunc(valueLeft)),
            CSSCalcValue::createExpressionNode(CSSPrimitiveValue::create(valueRight, typeRight), valueRight == trunc(valueRight)),
            CalcAdd
        ));
    }

    bool testToCSSValue(CSSValue* cssValue)
    {
        return testToCSSValue(cssValue, cssValue);
    }
    bool testToCSSValue(CSSValue* cssValueExpected, CSSValue* cssValue)
    {
        return AnimatableLength::create(cssValue)->toCSSValue()->equals(*cssValueExpected);
    }
    bool testInterpolate(CSSValue* cssValueExpected, AnimatableLength* numberA, AnimatableLength* numberB, double fraction)
    {
        return toAnimatableLength(AnimatableValue::interpolate(numberA, numberB, fraction).get())->toCSSValue()->equals(*cssValueExpected);
    }
    bool testAdd(CSSValue* cssValueExpected, AnimatableLength* numberA, AnimatableLength* numberB)
    {
        return toAnimatableLength(AnimatableValue::add(numberA, numberB).get())->toCSSValue()->equals(*cssValueExpected);
    }

    RefPtr<RenderStyle> style;
};

TEST_F(AnimatableLengthTest, CanCreateFrom)
{
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_PX).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_CM).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_MM).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_IN).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_PT).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_PC).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_EMS).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_EXS).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_REMS).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_PERCENTAGE).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_VW).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_VH).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_VMIN).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(5, CSSPrimitiveValue::CSS_VMAX).get()));

    EXPECT_TRUE(AnimatableLength::canCreateFrom(createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_CM).get()));
    EXPECT_TRUE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create(createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_CM)).get()));

    EXPECT_FALSE(AnimatableLength::canCreateFrom(CSSPrimitiveValue::create("NaN", CSSPrimitiveValue::CSS_STRING).get()));
}

TEST_F(AnimatableLengthTest, Create)
{
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_PX).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_CM).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_MM).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_IN).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_PT).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_PC).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_EMS).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_EXS).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_REMS).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_PERCENTAGE).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_VW).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_VH).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_VMIN).get()));
    EXPECT_TRUE(static_cast<bool>(create(5, CSSPrimitiveValue::CSS_VMAX).get()));

    EXPECT_TRUE(static_cast<bool>(
        AnimatableLength::create(createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_CM).get()).get()
    ));
    EXPECT_TRUE(static_cast<bool>(
        AnimatableLength::create(CSSPrimitiveValue::create(createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_CM)).get()).get()
    ));
}

TEST_F(AnimatableLengthTest, ToCSSValue)
{
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_PX).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_CM).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_MM).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_IN).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_PT).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_PC).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_EMS).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_EXS).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_REMS).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_PERCENTAGE).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_VW).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_VH).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_VMIN).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(-5, CSSPrimitiveValue::CSS_VMAX).get()));

    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_IN)).get()));
    EXPECT_TRUE(testToCSSValue(CSSPrimitiveValue::create(createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_IN)).get(),
        createCalc(3, CSSPrimitiveValue::CSS_PX, 5, CSSPrimitiveValue::CSS_IN).get()));
}

TEST_F(AnimatableLengthTest, ToLength)
{
    EXPECT_EQ(Length(-5, WebCore::Fixed), create(-5, CSSPrimitiveValue::CSS_PX)->toLength(style.get(), style.get(), 1));
    EXPECT_EQ(Length(-15, WebCore::Fixed), create(-5, CSSPrimitiveValue::CSS_PX)->toLength(style.get(), style.get(), 3));
    EXPECT_EQ(Length(0, WebCore::Fixed), create(-5, CSSPrimitiveValue::CSS_PX)->toLength(style.get(), style.get(), 1, NonNegativeValues));
    EXPECT_EQ(Length(0, WebCore::Fixed), create(-5, CSSPrimitiveValue::CSS_PX)->toLength(style.get(), style.get(), 3, NonNegativeValues));

    EXPECT_EQ(Length(-5, Percent), create(-5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 1));
    EXPECT_EQ(Length(-5, Percent), create(-5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 3));
    EXPECT_EQ(Length(0, Percent), create(-5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 1, NonNegativeValues));
    EXPECT_EQ(Length(0, Percent), create(-5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 3, NonNegativeValues));

    EXPECT_EQ(
        Length(CalculationValue::create(
            adoptPtr(new CalcExpressionBinaryOperation(
                adoptPtr(new CalcExpressionLength(Length(-5, WebCore::Fixed))),
                adoptPtr(new CalcExpressionLength(Length(-5, Percent))),
                CalcAdd)),
            CalculationRangeAll)),
        create(-5, CSSPrimitiveValue::CSS_PX, -5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 1));
    EXPECT_EQ(
        Length(CalculationValue::create(
            adoptPtr(new CalcExpressionBinaryOperation(
                adoptPtr(new CalcExpressionLength(Length(-15, WebCore::Fixed))),
                adoptPtr(new CalcExpressionLength(Length(-5, Percent))),
                CalcAdd)),
            CalculationRangeAll)),
        create(-5, CSSPrimitiveValue::CSS_PX, -5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 3));
    EXPECT_EQ(
        Length(CalculationValue::create(
            adoptPtr(new CalcExpressionBinaryOperation(
                adoptPtr(new CalcExpressionLength(Length(-5, WebCore::Fixed))),
                adoptPtr(new CalcExpressionLength(Length(-5, Percent))),
                CalcAdd)),
            CalculationRangeNonNegative)),
        create(-5, CSSPrimitiveValue::CSS_PX, -5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 1, NonNegativeValues));
    EXPECT_EQ(
        Length(CalculationValue::create(
            adoptPtr(new CalcExpressionBinaryOperation(
                adoptPtr(new CalcExpressionLength(Length(-15, WebCore::Fixed))),
                adoptPtr(new CalcExpressionLength(Length(-5, Percent))),
                CalcAdd)),
            CalculationRangeNonNegative)),
        create(-5, CSSPrimitiveValue::CSS_PX, -5, CSSPrimitiveValue::CSS_PERCENTAGE)->toLength(style.get(), style.get(), 3, NonNegativeValues));
}

TEST_F(AnimatableLengthTest, Interpolate)
{
    RefPtr<AnimatableLength> from10px = create(10, CSSPrimitiveValue::CSS_PX);
    RefPtr<AnimatableLength> to20pxAsInches = create(20.0 / 96, CSSPrimitiveValue::CSS_IN);
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(5,  CSSPrimitiveValue::CSS_PX).get(), from10px.get(), to20pxAsInches.get(), -0.5));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(10, CSSPrimitiveValue::CSS_PX).get(), from10px.get(), to20pxAsInches.get(),  0));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(14, CSSPrimitiveValue::CSS_PX).get(), from10px.get(), to20pxAsInches.get(),  0.4));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(15, CSSPrimitiveValue::CSS_PX).get(), from10px.get(), to20pxAsInches.get(),  0.5));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(16, CSSPrimitiveValue::CSS_PX).get(), from10px.get(), to20pxAsInches.get(),  0.6));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(20.0 / 96, CSSPrimitiveValue::CSS_IN).get(), from10px.get(), to20pxAsInches.get(),  1));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(25, CSSPrimitiveValue::CSS_PX).get(), from10px.get(), to20pxAsInches.get(),  1.5));

    RefPtr<AnimatableLength> from10em = create(10, CSSPrimitiveValue::CSS_EMS);
    RefPtr<AnimatableLength> to20rem = create(20, CSSPrimitiveValue::CSS_REMS);
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(createCalc(15, CSSPrimitiveValue::CSS_EMS, -10, CSSPrimitiveValue::CSS_REMS)).get(),
        from10em.get(), to20rem.get(), -0.5));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(10, CSSPrimitiveValue::CSS_EMS).get(),
        from10em.get(), to20rem.get(),  0));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(createCalc(6, CSSPrimitiveValue::CSS_EMS, 8, CSSPrimitiveValue::CSS_REMS)).get(),
        from10em.get(), to20rem.get(),  0.4));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(createCalc(5, CSSPrimitiveValue::CSS_EMS, 10, CSSPrimitiveValue::CSS_REMS)).get(),
        from10em.get(), to20rem.get(),  0.5));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(createCalc(4, CSSPrimitiveValue::CSS_EMS, 12, CSSPrimitiveValue::CSS_REMS)).get(),
        from10em.get(), to20rem.get(),  0.6));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(20, CSSPrimitiveValue::CSS_REMS).get(),
        from10em.get(), to20rem.get(),  1));
    EXPECT_TRUE(testInterpolate(CSSPrimitiveValue::create(createCalc(-5, CSSPrimitiveValue::CSS_EMS, 30, CSSPrimitiveValue::CSS_REMS)).get(),
        from10em.get(), to20rem.get(),  1.5));
}

TEST_F(AnimatableLengthTest, Add)
{
    EXPECT_TRUE(testAdd(CSSPrimitiveValue::create(10, CSSPrimitiveValue::CSS_PX).get(), create(10, CSSPrimitiveValue::CSS_PX).get(), create(0, CSSPrimitiveValue::CSS_MM).get()));
    EXPECT_TRUE(testAdd(CSSPrimitiveValue::create(100, CSSPrimitiveValue::CSS_PX).get(), create(4, CSSPrimitiveValue::CSS_PX).get(), create(1, CSSPrimitiveValue::CSS_IN).get()));
    EXPECT_TRUE(testAdd(
        CSSPrimitiveValue::create(createCalc(10, CSSPrimitiveValue::CSS_EMS, 20, CSSPrimitiveValue::CSS_REMS)).get(),
        create(10, CSSPrimitiveValue::CSS_EMS).get(),
        create(20, CSSPrimitiveValue::CSS_REMS).get()));
    EXPECT_TRUE(testAdd(
        CSSPrimitiveValue::create(10, CSSPrimitiveValue::CSS_EMS).get(),
        create(10, CSSPrimitiveValue::CSS_EMS).get(),
        create(0, CSSPrimitiveValue::CSS_REMS).get()));
    EXPECT_TRUE(testAdd(
        CSSPrimitiveValue::create(20, CSSPrimitiveValue::CSS_REMS).get(),
        create(0, CSSPrimitiveValue::CSS_EMS).get(),
        create(20, CSSPrimitiveValue::CSS_REMS).get()));
}

}
