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

/*Scan the referenced things in the function environment.*/
static void
function_env_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_FunctionEnv *fe = ptr;

    rjs_decl_env_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &fe->this_value);
    rjs_gc_scan_value(rt, &fe->function);
    rjs_gc_scan_value(rt, &fe->new_target);
}

/*Free the function environment.*/
static void
function_env_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_FunctionEnv *fe = ptr;

    rjs_decl_env_deinit(rt, &fe->decl_env);

    RJS_DEL(rt, fe);
}

/*Check if the function environment has this binding.*/
static RJS_Result
function_env_op_has_this_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_FunctionEnv *fe = (RJS_FunctionEnv*)env;

    if (fe->this_status == RJS_THIS_STATUS_LEXICAL)
        return RJS_FALSE;

    return RJS_TRUE;
}

/*Check if the function environemnt has the super binding.*/
static RJS_Result
function_env_op_has_super_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_FunctionEnv *fe = (RJS_FunctionEnv*)env;

    if (fe->this_status == RJS_THIS_STATUS_LEXICAL)
        return RJS_FALSE;

    if (rjs_value_get_gc_thing_type(rt, &fe->function) == RJS_GC_THING_SCRIPT_FUNC) {
        RJS_ScriptFuncObject *sfo;
        
        sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, &fe->function);

        if (rjs_value_is_undefined(rt, &sfo->home_object))
            return RJS_FALSE;
    }

    return RJS_TRUE;
}

/*Get this binding of the function environment.*/
static RJS_Result
function_env_op_get_this_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v)
{
    RJS_FunctionEnv *fe = (RJS_FunctionEnv*)env;

    assert(fe->this_status != RJS_THIS_STATUS_LEXICAL);

    if (fe->this_status == RJS_THIS_STATUS_UNINITIALIZED)
        return rjs_throw_reference_error(rt, _("this binding is uninitialized"));

    rjs_value_copy(rt, v, &fe->this_value);

    return RJS_OK;
}

/*Function environment operation functions.*/
static const RJS_EnvOps
function_env_ops = {
    {
        RJS_GC_THING_FUNCTION_ENV,
        function_env_op_gc_scan,
        function_env_op_gc_free
    },
    rjs_decl_env_op_has_binding,
    rjs_decl_env_op_create_mutable_binding,
    rjs_decl_env_op_create_immutable_binding,
    rjs_decl_env_op_initialize_binding,
    rjs_decl_env_op_set_mutable_binding,
    rjs_decl_env_op_get_binding_value,
    rjs_decl_env_op_delete_binding,
    function_env_op_has_this_binding,
    function_env_op_has_super_binding,
    rjs_decl_env_op_with_base_object,
    function_env_op_get_this_binding
};

/*Get the function environment.*/
static RJS_FunctionEnv*
function_env_get (RJS_Environment *env)
{
    assert(env->gc_thing.ops->type == RJS_GC_THING_FUNCTION_ENV);

    return (RJS_FunctionEnv*)env;
}

/**
 * Create a new function environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param func The function.
 * \param new_target The new target.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_function_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_Value *func, RJS_Value *new_target)
{
    RJS_FunctionEnv      *fe;
    RJS_ScriptFuncObject *sfo;
    RJS_GcThingType       gtt;

    gtt = rjs_value_get_gc_thing_type(rt, func);

    assert((gtt == RJS_GC_THING_SCRIPT_FUNC)
#if ENABLE_GENERATOR
            || (gtt == RJS_GC_THING_GENERATOR)
#if ENABLE_ASYNC
            || (gtt == RJS_GC_THING_ASYNC_GENERATOR)
#endif /*ENABLE_ASYNC*/
#endif /*ENABLE_GENERATOR*/
            );

    RJS_NEW(rt, fe);

    rjs_value_set_undefined(rt, &fe->this_value);
    rjs_value_copy(rt, &fe->function, func);

    sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, func);

#if ENABLE_ARROW_FUNC
    if (sfo->script_func->flags & RJS_FUNC_FL_ARROW)
        fe->this_status = RJS_THIS_STATUS_LEXICAL;
    else
#endif /*ENABLE_ARROW_FUNC*/
        fe->this_status = RJS_THIS_STATUS_UNINITIALIZED;

    rjs_value_copy(rt, &fe->new_target, new_target);

    rjs_decl_env_init(rt, &fe->decl_env, NULL, sfo->env);

    *pe = &fe->decl_env.env;

    rjs_gc_add(rt, fe, &function_env_ops.gc_thing_ops);

    return RJS_OK;
}

/**
 * Bind this value to the environemnt..
 * \param rt The current runtime.
 * \param env The environment.
 * \param v This value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_bind_this_value (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v)
{
    RJS_FunctionEnv *fe = function_env_get(env);

    assert(fe->this_status != RJS_THIS_STATUS_LEXICAL);

    if (fe->this_status == RJS_THIS_STATUS_INITIALIZED)
        return rjs_throw_reference_error(rt, _("this binding is already initialized"));

    fe->this_status = RJS_THIS_STATUS_INITIALIZED;
    rjs_value_copy(rt, &fe->this_value, v);

    return RJS_OK;
}

/**
 * Get super base object of the environment.
 * \param rt The current runtime.
 * \param env The environment.
 * \param[out] sb Return the super base value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_env_get_super_base (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *sb)
{
    RJS_FunctionEnv      *fe  = function_env_get(env);
    RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, &fe->function);

    if (rjs_value_is_undefined(rt, &sfo->home_object)) {
        rjs_value_set_undefined(rt, sb);
        return RJS_OK;
    }

    assert(rjs_value_is_object(rt, &sfo->home_object));

    return rjs_object_get_prototype_of(rt, &sfo->home_object, sb);
}

/**
 * Clear the function environment.
 * \param rt The current runtime.
 * \param env The function environment to be cleared.
 */
void
rjs_function_env_clear (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_FunctionEnv      *fe = (RJS_FunctionEnv*)env;
    RJS_ScriptFuncObject *sfo;

    rjs_decl_env_clear(rt, env);

    rjs_value_set_undefined(rt, &fe->this_value);
    rjs_value_set_undefined(rt, &fe->new_target);

    sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, &fe->function);

#if ENABLE_ARROW_FUNC
    if (sfo->script_func->flags & RJS_FUNC_FL_ARROW)
        fe->this_status = RJS_THIS_STATUS_LEXICAL;
    else
#endif /*ENABLE_ARROW_FUNC*/
        fe->this_status = RJS_THIS_STATUS_UNINITIALIZED;
}
