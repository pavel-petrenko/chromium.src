/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "RegExpPrototype.h"

#include "ArrayPrototype.h"
#include "Error.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSObject.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "JSValue.h"
#include "ObjectPrototype.h"
#include "RegExpObject.h"
#include "RegExp.h"
#include "RegExpCache.h"
#include "StringRecursionChecker.h"
#include "UStringConcatenate.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL regExpProtoFuncTest(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoFuncExec(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoFuncCompile(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoFuncToString(ExecState*);

}

#include "RegExpPrototype.lut.h"

namespace JSC {

const ClassInfo RegExpPrototype::s_info = { "RegExp", &RegExpObject::s_info, 0, ExecState::regExpPrototypeTable };

/* Source for RegExpPrototype.lut.h
@begin regExpPrototypeTable
  compile   regExpProtoFuncCompile      DontEnum|Function 2
  exec      regExpProtoFuncExec         DontEnum|Function 1
  test      regExpProtoFuncTest         DontEnum|Function 1
  toString  regExpProtoFuncToString     DontEnum|Function 0
@end
*/

ASSERT_CLASS_FITS_IN_CELL(RegExpPrototype);

RegExpPrototype::RegExpPrototype(ExecState*, JSGlobalObject* globalObject, Structure* structure, RegExp* regExp)
    : RegExpObject(globalObject, structure, regExp)
{
}

bool RegExpPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<RegExpObject>(exec, ExecState::regExpPrototypeTable(exec), this, propertyName, slot);
}

bool RegExpPrototype::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<RegExpObject>(exec, ExecState::regExpPrototypeTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

EncodedJSValue JSC_HOST_CALL regExpProtoFuncTest(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&RegExpObject::s_info))
        return throwVMTypeError(exec);
    return JSValue::encode(asRegExpObject(thisValue)->test(exec));
}

EncodedJSValue JSC_HOST_CALL regExpProtoFuncExec(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&RegExpObject::s_info))
        return throwVMTypeError(exec);
    return JSValue::encode(asRegExpObject(thisValue)->exec(exec));
}

EncodedJSValue JSC_HOST_CALL regExpProtoFuncCompile(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&RegExpObject::s_info))
        return throwVMTypeError(exec);

    RegExp* regExp;
    JSValue arg0 = exec->argument(0);
    JSValue arg1 = exec->argument(1);
    
    if (arg0.inherits(&RegExpObject::s_info)) {
        if (!arg1.isUndefined())
            return throwVMError(exec, createTypeError(exec, "Cannot supply flags when constructing one RegExp from another."));
        regExp = asRegExpObject(arg0)->regExp();
    } else {
        UString pattern = !exec->argumentCount() ? UString("") : arg0.toString(exec);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());

        RegExpFlags flags = NoFlags;
        if (!arg1.isUndefined()) {
            flags = regExpFlags(arg1.toString(exec));
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            if (flags == InvalidFlags)
                return throwVMError(exec, createSyntaxError(exec, "Invalid flags supplied to RegExp constructor."));
        }
        regExp = exec->globalData().regExpCache()->lookupOrCreate(pattern, flags);
    }

    if (!regExp->isValid())
        return throwVMError(exec, createSyntaxError(exec, regExp->errorMessage()));

    asRegExpObject(thisValue)->setRegExp(exec->globalData(), regExp);
    asRegExpObject(thisValue)->setLastIndex(0);
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL regExpProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&RegExpObject::s_info)) {
        if (thisValue.inherits(&RegExpPrototype::s_info))
            return JSValue::encode(jsNontrivialString(exec, "//"));
        return throwVMTypeError(exec);
    }

    RegExpObject* thisObject = asRegExpObject(thisValue);

    StringRecursionChecker checker(exec, thisObject);
    if (EncodedJSValue earlyReturnValue = checker.earlyReturnValue())
        return earlyReturnValue;

    char postfix[5] = { '/', 0, 0, 0, 0 };
    int index = 1;
    if (thisObject->get(exec, exec->propertyNames().global).toBoolean(exec))
        postfix[index++] = 'g';
    if (thisObject->get(exec, exec->propertyNames().ignoreCase).toBoolean(exec))
        postfix[index++] = 'i';
    if (thisObject->get(exec, exec->propertyNames().multiline).toBoolean(exec))
        postfix[index] = 'm';
    UString source = thisObject->get(exec, exec->propertyNames().source).toString(exec);
    // If source is empty, use "/(?:)/" to avoid colliding with comment syntax
    return JSValue::encode(jsMakeNontrivialString(exec, "/", source.length() ? source : UString("(?:)"), postfix));
}

} // namespace JSC
