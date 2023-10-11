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

/**
 * \file
 * Script function object internal header.
 */

#ifndef _RJS_SCRIPT_FUNC_OBJECT_INTERNAL_H_
#define _RJS_SCRIPT_FUNC_OBJECT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Script function object.*/
typedef struct {
    RJS_BaseFuncObject bfo;         /**< Base function object.*/
    RJS_Environment   *env;         /**< The environment.*/
    RJS_Value          home_object; /**< Home object.*/
    RJS_ScriptFunc    *script_func; /**< Script function.*/
    RJS_Realm         *realm;       /**< The realm.*/
#if ENABLE_PRIV_NAME
    RJS_PrivateEnv    *priv_env;    /**< The private environment.*/
#endif /*ENABLE_PRIV_NAME*/
#if ENABLE_FUNC_SOURCE
    RJS_Value          source;      /**< The function's source.*/
#endif /*ENABLE_FUNC_SOURCE*/
} RJS_ScriptFuncObject;

/**
 * Scan the referenced things in the script function object.
 * \param rt The current runtime.
 * \param ptr The script function object pointer.
 */
RJS_INTERNAL void
rjs_script_func_object_op_gc_scan (RJS_Runtime *rt, void *ptr);

/**
 * Free the script function object.
 * \param rt The current runtime.
 * \param ptr The script function object pointer.
 */
RJS_INTERNAL void
rjs_script_func_object_op_gc_free (RJS_Runtime *rt, void *ptr);

/**
 * Make the script function object as constructor.
 * \param rt The current runtime.
 * \param f The script function object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_script_func_object_make_constructor (RJS_Runtime *rt, RJS_Value *f);

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
RJS_INTERNAL RJS_ScriptContext*
rjs_prepare_for_ordinary_call (RJS_Runtime *rt, RJS_Value *f, RJS_Value *new_target,
        RJS_Value *args, size_t argc, RJS_PromiseCapability *pc);

/**
 * Bind this argument.
 * \param rt The current runtime.
 * \param f The function.
 * \param thiz This argument.
 */
RJS_INTERNAL void
rjs_ordinary_call_bind_this (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz);

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
RJS_INTERNAL RJS_Result
rjs_script_func_object_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv);

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
RJS_INTERNAL RJS_Result
rjs_script_func_object_op_construct (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv);

/**Script function object's operation functions.*/
#define RJS_SCRIPT_FUNCTION_OBJECT_OPS\
    RJS_ORDINARY_OBJECT_OPS,\
    rjs_script_func_object_op_call,\
    rjs_script_func_object_op_construct

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
RJS_INTERNAL RJS_Result
rjs_script_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env, RJS_PrivateEnv *priv_env);

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
RJS_INTERNAL void
rjs_script_func_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_ScriptFuncObject *sfo, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env, RJS_PrivateEnv *priv_env,
        const RJS_ObjectOps *ops);

/**
 * Release the script function object.
 * \param rt The current runtime.
 * \param sfo The script function object to be released.
 */
RJS_INTERNAL void
rjs_script_func_object_deinit (RJS_Runtime *rt, RJS_ScriptFuncObject *sfo);

/**
 * Create an ordinary function.
 * \param rt The current runtime.
 * \param proto The function's prototype.
 * \param script The script.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \param[out] fo Return the function object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_ordinary_function_create (RJS_Runtime *rt, RJS_Value *proto, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *env, RJS_PrivateEnv *priv_env, RJS_Value *fo)
{
    return rjs_script_func_object_new(rt, fo, proto, script, sf, env, priv_env);
}

#ifdef __cplusplus
}
#endif

#endif

