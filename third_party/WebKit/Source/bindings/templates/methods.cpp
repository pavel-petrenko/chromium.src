{##############################################################################}
{% macro generate_method(method, world_suffix) %}
{% filter conditional(method.conditional_string) %}
static void {{method.name}}{{method.overload_index}}Method{{world_suffix}}(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    {% if method.name in ['addEventListener', 'removeEventListener'] %}
    {{add_remove_event_listener_method(method.name) | indent}}
    {% else %}
    {% if method.number_of_required_arguments %}
    if (UNLIKELY(info.Length() < {{method.number_of_required_arguments}})) {
        throwTypeError(ExceptionMessages::failedToExecute("{{method.name}}", "{{interface_name}}", ExceptionMessages::notEnoughArguments({{method.number_of_required_arguments}}, info.Length())), info.GetIsolate());
        return;
    }
    {% endif %}
    {% if not method.is_static %}
    {{cpp_class_name}}* imp = {{v8_class_name}}::toNative(info.Holder());
    {% endif %}
    {% if method.is_custom_element_callbacks %}
    CustomElementCallbackDispatcher::CallbackDeliveryScope deliveryScope;
    {% endif %}
    {% if method.is_raises_exception %}
    ExceptionState es(info.GetIsolate());
    {% endif %}
    {% if method.is_check_security_for_node %}
    if (!BindingSecurity::shouldAllowAccessToNode(imp->{{method.name}}(es), es)) {
        v8SetReturnValueNull(info);
        es.throwIfNeeded();
        return;
    }
    {% endif %}
    {% for argument in method.arguments %}
    {% if argument.is_optional and not argument.has_default and
          argument.idl_type != 'Dictionary' %}
    {# Optional arguments without a default value generate an early call with
       fewer arguments if they are omitted.
       Optional Dictionary arguments default to empty dictionary. #}
    if (UNLIKELY(info.Length() <= {{argument.index}})) {
        {{cpp_method_call(method, argument.v8_set_return_value, argument.cpp_value) | indent(8)}}
        return;
    }
    {% endif %}
    {% if method.is_strict_type_checking and argument.is_wrapper_type %}
    {# Type checking for wrapper interface types (if interface not implemented,
       throw TypeError), per http://www.w3.org/TR/WebIDL/#es-interface #}
    if (info.Length() > {{argument.index}} && !isUndefinedOrNull(info[{{argument.index}}]) && !V8{{argument.idl_type}}::hasInstance(info[{{argument.index}}], info.GetIsolate(), worldType(info.GetIsolate()))) {
        throwTypeError(ExceptionMessages::failedToExecute("{{method.name}}", "{{interface_name}}", "parameter {{argument.index + 1}} is not of type '{{argument.idl_type}}'."), info.GetIsolate());
        return;
    }
    {% endif %}
    {% if argument.is_clamp %}
    {# NaN is treated as 0: http://www.w3.org/TR/WebIDL/#es-type-mapping #}
    {{argument.cpp_type}} {{argument.name}} = 0;
    V8TRYCATCH_VOID(double, {{argument.name}}NativeValue, info[{{argument.index}}]->NumberValue());
    if (!std::isnan({{argument.name}}NativeValue))
        {# IDL type is used for clamping, for the right bounds, since different
           IDL integer types have same internal C++ type (int or unsigned) #}
        {{argument.name}} = clampTo<{{argument.idl_type}}>({{argument.name}}NativeValue);
    {% elif argument.idl_type == 'SerializedScriptValue' %}
    bool {{argument.name}}DidThrow = false;
    {{argument.cpp_type}} {{argument.name}} = SerializedScriptValue::create(info[{{argument.index}}], 0, 0, {{argument.name}}DidThrow, info.GetIsolate());
    if ({{argument.name}}DidThrow)
        return;
    {% elif argument.is_variadic_wrapper_type %}
    Vector<{{argument.cpp_type}} > {{argument.name}};
    for (int i = {{argument.index}}; i < info.Length(); ++i) {
        if (!V8{{argument.idl_type}}::hasInstance(info[i], info.GetIsolate(), worldType(info.GetIsolate()))) {
            throwTypeError(ExceptionMessages::failedToExecute("{{method.name}}", "{{interface_name}}", "parameter {{argument.index + 1}} is not of type '{{argument.idl_type}}'."), info.GetIsolate());
            return;
        }
        {{argument.name}}.append(V8{{argument.idl_type}}::toNative(v8::Handle<v8::Object>::Cast(info[i])));
    }
    {% else %}
    {{argument.v8_value_to_local_cpp_value}};
    {% endif %}
    {% if argument.enum_validation_expression %}
    {# Methods throw on invalid enum values: http://www.w3.org/TR/WebIDL/#idl-enums #}
    String string = {{argument.name}};
    if (!({{argument.enum_validation_expression}})) {
        throwTypeError(ExceptionMessages::failedToExecute("{{method.name}}", "{{interface_name}}", "parameter {{argument.index + 1}} ('" + string + "') is not a valid enum value."), info.GetIsolate());
        return;
    }
    {% endif %}
    {% if argument.idl_type in ['Dictionary', 'Promise'] %}
    if (!{{argument.name}}.isUndefinedOrNull() && !{{argument.name}}.isObject()) {
        throwTypeError(ExceptionMessages::failedToExecute("{{method.name}}", "{{interface_name}}", "parameter {{argument.index + 1}} ('{{argument.name}}') is not an object."), info.GetIsolate());
        return;
    }
    {% endif %}
    {% endfor %}{# arguments #}
    {{cpp_method_call(method, method.v8_set_return_value, method.cpp_value) | indent}}
    {% endif %}{# addEventListener, removeEventListener #}
}
{% endfilter %}
{% endmacro %}


{######################################}
{% macro add_remove_event_listener_method(method_name) %}
{# Set template values for addEventListener vs. removeEventListener #}
{% set listener_lookup_type, listener, hidden_dependency_action =
    ('ListenerFindOrCreate', 'listener', 'createHiddenDependency')
    if method_name == 'addEventListener' else
    ('ListenerFindOnly', 'listener.get()', 'removeHiddenDependency')
%}
EventTarget* impl = {{v8_class_name}}::toNative(info.Holder());
if (DOMWindow* window = impl->toDOMWindow()) {
    ExceptionState es(info.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToFrame(window->frame(), es)) {
        es.throwIfNeeded();
        return;
    }
    if (!window->document())
        return;
}
RefPtr<EventListener> listener = V8EventListenerList::getEventListener(info[1], false, {{listener_lookup_type}});
if (listener) {
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<WithNullCheck>, eventName, info[0]);
    impl->{{method_name}}(eventName, {{listener}}, info[2]->BooleanValue());
    if (!impl->toNode())
        {{hidden_dependency_action}}(info.Holder(), info[1], {{v8_class_name}}::eventListenerCacheIndex, info.GetIsolate());
}
{% endmacro %}


{######################################}
{% macro cpp_method_call(method, v8_set_return_value, cpp_value) %}
{% if method.is_call_with_script_state %}
ScriptState* currentState = ScriptState::current();
if (!currentState)
    return;
ScriptState& state = *currentState;
{% endif %}
{% if method.is_call_with_execution_context %}
ExecutionContext* scriptContext = getExecutionContext();
{% endif %}
{% if method.is_call_with_script_arguments %}
RefPtr<ScriptArguments> scriptArguments(createScriptArguments(info, {{method.number_of_arguments}}));
{% endif %}
{% if method.idl_type == 'void' %}
{{cpp_value}};
{% elif method.is_call_with_script_state %}
{# FIXME: consider always using a local variable #}
{{method.cpp_type}} result = {{cpp_value}};
{% endif %}
{% if method.is_raises_exception %}
if (es.throwIfNeeded())
    return;
{% endif %}
{% if method.is_call_with_script_state %}
if (state.hadException()) {
    v8::Local<v8::Value> exception = state.exception();
    state.clearException();
    throwError(exception, info.GetIsolate());
    return;
}
{% endif %}
{% if v8_set_return_value %}{{v8_set_return_value}};{% endif %}{# None for void #}
{% endmacro %}


{##############################################################################}
{% macro overload_resolution_method(overloads, world_suffix) %}
static void {{overloads.name}}Method{{world_suffix}}(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    {% for method in overloads.methods %}
    if ({{method.overload_resolution_expression}}) {
        {{method.name}}{{method.overload_index}}Method{{world_suffix}}(info);
        return;
    }
    {% endfor %}
    {% if overloads.minimum_number_of_required_arguments %}
    if (UNLIKELY(info.Length() < {{overloads.minimum_number_of_required_arguments}})) {
        throwTypeError(ExceptionMessages::failedToExecute("{{overloads.name}}", "{{interface_name}}", ExceptionMessages::notEnoughArguments({{overloads.minimum_number_of_required_arguments}}, info.Length())), info.GetIsolate());
        return;
    }
    {% endif %}
    throwUninformativeAndGenericTypeError(info.GetIsolate());
}
{% endmacro %}


{##############################################################################}
{% macro method_callback(method, world_suffix) %}
{% filter conditional(method.conditional_string) %}
static void {{method.name}}MethodCallback{{world_suffix}}(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    {% if method.measure_as %}
    UseCounter::count(activeDOMWindow(), UseCounter::{{method.measure_as}});
    {% endif %}
    {% if method.deprecate_as %}
    UseCounter::countDeprecation(activeExecutionContext(), UseCounter::{{method.deprecate_as}});
    {% endif %}
    {% if world_suffix in method.activity_logging_world_list %}
    V8PerContextData* contextData = V8PerContextData::from(info.GetIsolate()->GetCurrentContext());
    if (contextData && contextData->activityLogger()) {
        {# FIXME: replace toVectorOfArguments with toNativeArguments(info, 0)
           and delete toVectorOfArguments #}
        Vector<v8::Handle<v8::Value> > loggerArgs = toVectorOfArguments(info);
        contextData->activityLogger()->log("{{interface_name}}.{{method.name}}", info.Length(), loggerArgs.data(), "Method");
    }
    {% endif %}
    {% if method.is_custom %}
    {{v8_class_name}}::{{method.name}}MethodCustom(info);
    {% else %}
    {{cpp_class_name}}V8Internal::{{method.name}}Method{{world_suffix}}(info);
    {% endif %}
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}
{% endfilter %}
{% endmacro %}
