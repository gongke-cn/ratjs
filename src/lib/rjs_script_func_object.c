/*****************************************************************************
 *                             Rat Javascript                                *
 *                                                                           *
 * Copyright 2022 Gong Ke                                                    *
 *                                                                           *
 * Permission is hereby granted, free of charge, to any person obtaining a   *
 * copy of this software and associated documentation files (the             *
 * "Software"), to deal in the Software without restriction, including       *
 * without limitation the rights to use, copy, modify, merge, publish,       *
 * distribute, sublicense, and/or sell copies of the Software, and to permit *
 * persons to whom the Software is furnished to do so, subject to the        *
 * following conditions:                                                     *
 *                                                                           *
 * The above copyright notice and this permission notice shall be included   *
 * in all copies or substantial portions of the Software.                    *
 *                                                                           *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN *
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                    *
 *****************************************************************************/

#include "ratjs_internal.h"

/**
 * Scan the referenced things in the script function object.
 * \param rt The current runtime.
 * \param ptr The script function object pointer.
 */
void
rjs_script_func_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ScriptFuncObject *sfo = ptr;

    rjs_base_func_object_op_gc_scan(rt, &sfo->bfo);

    if (sfo->env)
        rjs_gc_mark(rt, sfo->env);

    if (sfo->realm)
        rjs_gc_mark(rt, sfo->realm);

#if ENABLE_PRIV_NAME
    if (sfo->priv_env)
        rjs_gc_mark(rt, sfo->priv_env);
#endif /*ENABLE_PRIV_NAME*/

    if (sfo->bfo.script)
        rjs_gc_mark(rt, sfo->bfo.script);

    rjs_gc_scan_value(rt, &sfo->home_object);

#if ENABLE_FUNC_SOURCE
    rjs_gc_scan_value(rt, &sfo->source);
#endif /*ENABLE_FUNC_SOURCE*/
}

/**
 * Free the script function object.
 * \param rt The current runtime.
 * \param ptr The script function object pointer.
 */
void
rjs_script_func_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ScriptFuncObject *sfo = ptr;

    rjs_script_func_object_deinit(rt, sfo);

    RJS_DEL(rt, sfo);
}

/**
 * Create a new context for ordinary call.
 * \param rt The current runtime.
 * \param f The function.
 * \param new_target The new target.
 * \param args Arguments.
 * \param argc Arguments' count.
 * \param pc The promise capability.
 * \return The new context.
 */
RJS_ScriptContext*
rjs_prepare_for_ordinary_call (RJS_Runtime *rt, RJS_Value *f, RJS_Value *new_target,
        RJS_Value *args, size_t argc, RJS_PromiseCapability *pc)
{
    RJS_ScriptContext    *callee_ctxt;
    RJS_ScriptFuncObject *sfo      = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, f);
#if ENABLE_ASYNC || ENABLE_GENERATOR
    RJS_ScriptFunc       *sf       = sfo->script_func;
#endif /*ENABLE_ASYNC || ENABLE_GENERATOR*/
    RJS_PrivateEnv       *priv_env = NULL;

#if ENABLE_PRIV_NAME
    priv_env = sfo->priv_env;
#endif /*ENABLE_PRIV_NAME*/

    rjs_function_env_new(rt, &rt->env, f, new_target);

#if ENABLE_ASYNC
    if (sf->flags & RJS_FUNC_FL_ASYNC)
        callee_ctxt = (RJS_ScriptContext*)rjs_async_context_push(rt, f, sfo->bfo.script,
                sfo->script_func, rt->env, rt->env, priv_env, args, argc, pc);
    else
#endif /*ENABLE_ASYNC*/
#if ENABLE_GENERATOR
    if (sf->flags & RJS_FUNC_FL_GENERATOR)
        callee_ctxt = (RJS_ScriptContext*)rjs_generator_context_push(rt, f, sfo->bfo.script,
                sfo->script_func, rt->env, rt->env, priv_env, args, argc);
    else
#endif /*ENABLE_GENERATOR*/
        callee_ctxt = (RJS_ScriptContext*)rjs_script_context_push(rt, f, sfo->bfo.script,
                sfo->script_func, rt->env, rt->env, priv_env, args, argc);

    callee_ctxt->scb.context.realm = sfo->realm;

    return callee_ctxt;
}

/**
 * Bind this argument.
 * \param rt The current runtime.
 * \param f The function.
 * \param thiz This argument.
 */
void
rjs_ordinary_call_bind_this (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz)
{
    RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, f);
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *tb  = rjs_value_stack_push(rt);
    RJS_ScriptContext    *sc  = (RJS_ScriptContext*)rjs_context_running(rt);

#if ENABLE_ARROW_FUNC
    if (sfo->script_func->flags & RJS_FUNC_FL_ARROW)
        goto end;
#endif /*ENABLE_ARROW_FUNC*/

    if (sfo->script_func->flags & RJS_FUNC_FL_STRICT) {
        rjs_value_copy(rt, tb, thiz);
    } else {
        if (rjs_value_is_undefined(rt, thiz) || rjs_value_is_null(rt, thiz)) {
            RJS_Script    *script = sfo->bfo.script;
            RJS_GlobalEnv *ge     = (RJS_GlobalEnv*)rjs_global_env(script->realm);

            rjs_value_copy(rt, tb, &ge->global_this);
        } else {
            rjs_to_object(rt, thiz, tb);
        }
    }

    rjs_env_bind_this_value(rt, sc->scb.lex_env, tb);
end:
    rjs_value_stack_restore(rt, top);
}

/**
 * Call the script function object.
 * \param rt The current runtime.
 * \param o The script function object.
 * \param thiz This argument.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_func_object_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_Result            r;
    RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, o);
    size_t                top = rjs_value_stack_save(rt);

    rjs_prepare_for_ordinary_call(rt, o, rjs_v_undefined(rt), args, argc, NULL);

    if (sfo->script_func->flags & RJS_FUNC_FL_CLASS_CONSTR) {
        rjs_throw_type_error(rt, _("class's constructor cannot be called"));
        rjs_context_pop(rt);

        return RJS_ERR;
    }

    rjs_ordinary_call_bind_this(rt, o, thiz);

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_SYNC_START, NULL, rv);

    rjs_context_pop(rt);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Construct a new object from a script function.
 * \param rt The current runtime.
 * \param o The script function object.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param target The new target.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_func_object_op_construct (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv)
{
    RJS_Result            r;
    RJS_Environment      *constr_env;
    RJS_ScriptContext    *sc;
    RJS_ScriptFuncObject *sfo  = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, o);
    size_t                top  = rjs_value_stack_save(rt);
    RJS_Value            *thiz = rjs_value_stack_push(rt);

    if(!(sfo->script_func->flags & RJS_FUNC_FL_DERIVED)) {
        if ((r = rjs_ordinary_create_from_constructor(rt, target, RJS_O_Object_prototype, thiz)) == RJS_ERR)
            goto end;
    }

    sc = rjs_prepare_for_ordinary_call(rt, o, target, args, argc, NULL);

    constr_env = sc->scb.lex_env;

    if(!(sfo->script_func->flags & RJS_FUNC_FL_DERIVED)) {
        rjs_ordinary_call_bind_this(rt, o, thiz);

        if (sfo->bfo.clazz) {
            r = rjs_initialize_instance_elements(rt, thiz, o);

            if (r == RJS_ERR) {
                rjs_context_pop(rt);
                goto end;
            }
        }
    }

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_CONSTRUCT, target, rv);

    rt->env = constr_env;
    rjs_context_pop(rt);

    if (r == RJS_ERR)
        goto end;

    if (rjs_value_is_object(rt, rv)) {
        r = RJS_OK;
        goto end;
    }
        
    if(!(sfo->script_func->flags & RJS_FUNC_FL_DERIVED)) {
        rjs_value_copy(rt, rv, thiz);
        r = RJS_OK;
        goto end;
    }
    
    if (!rjs_value_is_undefined(rt, rv)) {
        rjs_throw_type_error(rt, _("construct result is not an object"));
        r = RJS_ERR;
        goto end;
    }

    r = rjs_env_get_this_binding(rt, constr_env, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Function operation functions.*/
static const RJS_ObjectOps
script_func_object_ops = {
    {
        RJS_GC_THING_SCRIPT_FUNC,
        rjs_script_func_object_op_gc_scan,
        rjs_script_func_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    rjs_script_func_object_op_call
};

/*Constructor operation functions.*/
static const RJS_ObjectOps
script_constructor_object_ops = {
    {
        RJS_GC_THING_SCRIPT_FUNC,
        rjs_script_func_object_op_gc_scan,
        rjs_script_func_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    rjs_script_func_object_op_call,
    rjs_script_func_object_op_construct
};

/**
 * Create a new script function object.
 * \param rt The current runtime.
 * \param[out] v Return the function object.
 * \param proto The prototype of the object.
 * \param script The script constains this function.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env, RJS_PrivateEnv *priv_env)
{
    RJS_ScriptFuncObject *sfo;
    const RJS_ObjectOps  *ops;

    RJS_NEW(rt, sfo);

    ops = &script_func_object_ops;

    rjs_script_func_object_init(rt, v, sfo, proto, script, sf, env, priv_env, ops);

    return RJS_OK;
}

/**
 * Create a new script function object.
 * \param rt The current runtime.
 * \param[out] v Return the function object.
 * \param proto The prototype of the object.
 * \param script The script constains this function.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \param ops The object operation functions.
 */
void
rjs_script_func_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_ScriptFuncObject *sfo, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env, RJS_PrivateEnv *priv_env,
        const RJS_ObjectOps *ops)
{
    if (!proto) {
#if ENABLE_GENERATOR && ENABLE_ASYNC
        if ((sf->flags & (RJS_FUNC_FL_ASYNC|RJS_FUNC_FL_GENERATOR))
                == (RJS_FUNC_FL_ASYNC|RJS_FUNC_FL_GENERATOR)) {
            proto = rjs_o_AsyncGenerator_prototype(script->realm);
        } else
#endif /*ENABLE_GENERATOR && ENABLE_ASYNC*/
#if ENABLE_ASYNC
        if (sf->flags & RJS_FUNC_FL_ASYNC) {
            proto = rjs_o_AsyncFunction_prototype(script->realm);
        } else
#endif /*ENABLE_ASYNC*/
#if ENABLE_GENERATOR
        if (sf->flags & RJS_FUNC_FL_GENERATOR) {
            proto = rjs_o_Generator_prototype(script->realm);
        } else
#endif /*ENABLE_GENERATOR*/
        {
            proto = rjs_o_Function_prototype(script->realm);
        }
    }

    sfo->script_func = sf;
    sfo->env         = env;
    sfo->realm       = rjs_realm_current(rt);

#if ENABLE_PRIV_NAME
    sfo->priv_env    = priv_env;
#endif /*ENABLE_PRIV_NAME*/

#if ENABLE_FUNC_SOURCE
    if (sf->source_idx != RJS_INVALID_VALUE_INDEX) {
        RJS_Value *v = &script->value_table[sf->source_idx];

        rjs_value_copy(rt, &sfo->source, v);
    } else {
        rjs_value_set_undefined(rt, &sfo->source);
    }
#endif /*ENABLE_FUNC_SOURCE*/

    rjs_value_set_undefined(rt, &sfo->home_object);

    rjs_base_func_object_init(rt, v, &sfo->bfo, proto, ops, script);

    rjs_set_function_length(rt, v, sf->param_len);

    if ((sf->name_idx != RJS_INVALID_VALUE_INDEX)
#if ENABLE_GENERATOR
            && !(sf->flags & RJS_FUNC_FL_GENERATOR)
#endif /*ENABLE_GENERATOR*/
            ) {
        RJS_Value *n = &script->value_table[sf->name_idx];

        rjs_set_function_name(rt, v, n, NULL);
    }
}

/**
 * Release the script function object.
 * \param rt The current runtime.
 * \param sfo The script function object to be released.
 */
void
rjs_script_func_object_deinit (RJS_Runtime *rt, RJS_ScriptFuncObject *sfo)
{
    rjs_base_func_object_deinit(rt, &sfo->bfo);
}

/**
 * Make the script function object as constructor.
 * \param rt The current runtime.
 * \param f The script function object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_func_object_make_constructor (RJS_Runtime *rt, RJS_Value *f)
{
    RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, f);

    if (sfo->bfo.object.gc_thing.ops == (RJS_GcThingOps*)&script_func_object_ops)
        sfo->bfo.object.gc_thing.ops = (RJS_GcThingOps*)&script_constructor_object_ops;

    return RJS_OK;
}

/**
 * Create a dynamic function.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param nt The new target.
 * \param flags RJS_FUNC_FL_ASYNC, RJS_FUNC_FL_GENERATOR.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] func Return the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_dynamic_function (RJS_Runtime *rt, RJS_Value *constr, RJS_Value *nt,
        int flags, RJS_Value *args, size_t argc, RJS_Value *func)
{
    size_t           top        = rjs_value_stack_save(rt);
    RJS_Value       *str        = rjs_value_stack_push(rt);
    RJS_Value       *src        = rjs_value_stack_push(rt);
    RJS_Value       *scriptv    = rjs_value_stack_push(rt);
    RJS_Value       *proto      = rjs_value_stack_push(rt);
    RJS_Bool         need_close = RJS_FALSE;
    size_t           i          = 0;
    int              fallback_proto;
    RJS_Environment *global_env;
    RJS_Value       *arg;
    RJS_UCharBuffer  ucb;
    RJS_Input        si;
    RJS_Script      *script;
    RJS_ScriptFunc  *sf;
    RJS_Result       r;

    /*Create the input string.*/
    rjs_uchar_buffer_init(rt, &ucb);

#if ENABLE_ASYNC
    if (flags & RJS_FUNC_FL_ASYNC)
        rjs_uchar_buffer_append_chars(rt, &ucb, "async ", -1);
#endif /*ENABLE_ASYNC*/

    rjs_uchar_buffer_append_chars(rt, &ucb, "function", -1);

#if ENABLE_GENERATOR
    if (flags & RJS_FUNC_FL_GENERATOR)
        rjs_uchar_buffer_append_uchar(rt, &ucb, '*');
#endif /*ENABLE_GENERATOR*/

    rjs_uchar_buffer_append_chars(rt, &ucb, " anonymous(", -1);

    if (argc > 1) {
        for (i = 0; i < argc - 1; i ++) {
            if (i != 0)
                rjs_uchar_buffer_append_uchar(rt, &ucb, ',');

            arg = rjs_argument_get(rt, args, argc, i);

            if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
                goto end;

            rjs_uchar_buffer_append_string(rt, &ucb, str);
        }
    }

    rjs_uchar_buffer_append_chars(rt, &ucb, "\n) {\n", -1);

    if (i < argc) {
        arg = rjs_argument_get(rt, args, argc, i);

        if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
            goto end;

        rjs_uchar_buffer_append_string(rt, &ucb, str);
    }

    rjs_uchar_buffer_append_chars(rt, &ucb, "\n}", -1);
    rjs_string_from_uchars(rt, src, ucb.items, ucb.item_num);

    rjs_uchar_buffer_deinit(rt, &ucb);

    /*Create the input source.*/
    if ((r = rjs_string_input_init(rt, &si, src)) == RJS_ERR)
        goto end;

    si.flags |= RJS_INPUT_FL_CRLF_TO_LF;
    need_close = RJS_TRUE;

    /*Parse the function.*/
    if ((r = rjs_parse_function(rt, &si, rjs_realm_current(rt), scriptv)) == RJS_ERR) {
        r = rjs_throw_syntax_error(rt, _("function syntax error"));
        goto end;
    }

    script     = rjs_value_get_gc_thing(rt, scriptv);
    sf         = script->func_table;
    global_env = rjs_global_env(script->realm);

    /*Get the prototype of the function.*/
    if (!nt)
        nt = constr;

#if ENABLE_ASYNC
    if (sf->flags & RJS_FUNC_FL_ASYNC) {
#if ENABLE_GENERATOR
        if (sf->flags & RJS_FUNC_FL_GENERATOR)
            fallback_proto = RJS_O_AsyncGeneratorFunction_prototype;
        else
#endif /*ENABLE_GENERATOR*/
            fallback_proto = RJS_O_AsyncFunction_prototype;
    } else
#endif /*ENABLE_ASYNC*/
#if ENABLE_GENERATOR
    if (sf->flags & RJS_FUNC_FL_GENERATOR) {
        fallback_proto = RJS_O_GeneratorFunction_prototype;
    } else
#endif /*ENABLE_GENERATOR*/
    {
        fallback_proto = RJS_O_Function_prototype;
    }

    rjs_get_prototype_from_constructor(rt, nt, fallback_proto, proto);

    /*Create the function.*/
#if ENABLE_GENERATOR
    if (sf->flags & RJS_FUNC_FL_GENERATOR) {
        r = rjs_generator_function_new(rt, func, proto, script, sf, global_env, NULL);
    } else
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
    if (sf->flags & RJS_FUNC_FL_ASYNC) {
        r = rjs_async_function_new(rt, func, proto, script, sf, global_env, NULL);
    } else
#endif /*ENABLE_ASYNC*/
    {
        r = rjs_script_func_object_new(rt, func, proto, script, sf, global_env, NULL);
        if (r == RJS_OK)
            rjs_make_constructor(rt, func, RJS_TRUE, NULL);
    }
end:
    if (need_close)
        rjs_input_deinit(rt, &si);

    rjs_value_stack_restore(rt, top);
    return r;
}
