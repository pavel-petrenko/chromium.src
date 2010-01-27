description('Tests for .valueAsNumber with &lt;input type=number>.');

var input = document.createElement('input');
input.type = 'number';

function valueAsNumberFor(stringValue) {
    input.value = stringValue;
    return input.valueAsNumber;
}

function setValueAsNumberAndGetValue(num) {
    input.valueAsNumber = num;
    return input.value;
}

shouldBe('valueAsNumberFor("0")', '0');
shouldBe('valueAsNumberFor("10")', '10');
shouldBe('valueAsNumberFor("01")', '1');
shouldBe('valueAsNumberFor("-0")', '0'); // "-0" is 0 in HTML5.
shouldBe('valueAsNumberFor("-1.2")', '-1.2');
shouldBe('valueAsNumberFor("1.2E10")', '1.2E10');
shouldBe('valueAsNumberFor("1.2E-10")', '1.2E-10');
shouldBe('valueAsNumberFor("1.2E+10")', '1.2E10');
shouldBe('valueAsNumberFor("12345678901234567890123456789012345678901234567890")', '1.2345678901234567E+49');
shouldBe('valueAsNumberFor("0.12345678901234567890123456789012345678901234567890")', '0.123456789012345678');

debug('valueAsNumber for invalid string values:');
shouldBeTrue('isNaN(valueAsNumberFor(""))');
shouldBeTrue('isNaN(valueAsNumberFor("abc"))');
shouldBeTrue('isNaN(valueAsNumberFor("0xff"))');
shouldBeTrue('isNaN(valueAsNumberFor("+1"))');
shouldBeTrue('isNaN(valueAsNumberFor(" 10"))');
shouldBeTrue('isNaN(valueAsNumberFor("10 "))');
shouldBeTrue('isNaN(valueAsNumberFor(".2"))');
shouldBeTrue('isNaN(valueAsNumberFor("1E"))');
shouldBeTrue('isNaN(valueAsNumberFor("NaN"))');
shouldBeTrue('isNaN(valueAsNumberFor("nan"))');
shouldBeTrue('isNaN(valueAsNumberFor("Inf"))');
shouldBeTrue('isNaN(valueAsNumberFor("inf"))');
shouldBeTrue('isNaN(valueAsNumberFor("Infinity"))');
shouldBeTrue('isNaN(valueAsNumberFor("infinity"))');

debug('Too huge exponent to support');
shouldBeTrue('isNaN(valueAsNumberFor("1.2E65535"))');

debug('Tests for the valueAsNumber setter:');
shouldBe('setValueAsNumberAndGetValue(0)', '"0"');
shouldBe('setValueAsNumberAndGetValue(10)', '"10"');
shouldBe('setValueAsNumberAndGetValue(01)', '"1"');
shouldBe('setValueAsNumberAndGetValue(-0)', '"0"');
shouldBe('setValueAsNumberAndGetValue(-1.2)', '"-1.2"');
shouldBe('setValueAsNumberAndGetValue(1.2e10)', '"12000000000"');
shouldBe('setValueAsNumberAndGetValue(1.2e-10)', '"1.2e-10"');
shouldBe('setValueAsNumberAndGetValue(1.2345678901234567e+49)', '"1.2345678901234567e+49"');

debug('Tests to set invalid values to valueAsNumber:');
shouldBe('setValueAsNumberAndGetValue(null)', '"0"');
shouldThrow('setValueAsNumberAndGetValue("foo")', '"Error: NOT_SUPPORTED_ERR: DOM Exception 9"');
shouldThrow('setValueAsNumberAndGetValue(NaN)', '"Error: NOT_SUPPORTED_ERR: DOM Exception 9"');
shouldThrow('setValueAsNumberAndGetValue(Number.NaN)', '"Error: NOT_SUPPORTED_ERR: DOM Exception 9"');
shouldThrow('setValueAsNumberAndGetValue(Infinity)', '"Error: NOT_SUPPORTED_ERR: DOM Exception 9"');
shouldThrow('setValueAsNumberAndGetValue(Number.POSITIVE_INFINITY)', '"Error: NOT_SUPPORTED_ERR: DOM Exception 9"');
shouldThrow('setValueAsNumberAndGetValue(Number.NEGATIVE_INFINITY)', '"Error: NOT_SUPPORTED_ERR: DOM Exception 9"');

var successfullyParsed = true;
