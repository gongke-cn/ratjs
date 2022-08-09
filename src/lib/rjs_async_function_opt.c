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

/*Call the async function object.*/
static RJS_Result
async_function_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_PromiseCapability  pc;
    RJS_Value             *fp;
    RJS_Value             *thisp;
    RJS_Value             *argsp;
    RJS_Value             *rvp;
    RJS_Realm             *realm = rjs_realm_current(rt);
    size_t                 top   = rjs_value_stack_save(rt);

    rjs_promise_capability_init(rt, &pc);
    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    fp    = rjs_value_get_pointer(rt, o);
    thisp = rjs_value_get_pointer(rt, thiz);
    argsp = argc ? rjs_value_get_pointer(rt, args) : NULL;
    rvp   = rjs_value_get_pointer(rt, rv);

    rjs_prepare_for_ordinary_call(rt, fp, rjs_v_undefined(rt), argsp, argc, &pc);
    rjs_ordinary_call_bind_this(rt, fp, thisp);

    rjs_script_func_call(rt, RJS_SCRIPT_CALL_ASYNC_START, NULL, rvp);

    rjs_context_pop(rt);

    rjs_value_copy(rt, rv, pc.promise);

    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Async function object operation functions.*/
static const RJS_ObjectOps
async_function_ops = {
    {
        RJS_GC_THING_SCRIPT_FUNC,
        rjs_script_func_object_op_gc_scan,
        rjs_script_func_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    async_function_op_call
};

/**
 * Create a new async function object.
 * \param rt The current runtime.
 * \param[out] f Return the new function.
 * \param proto The prototype.
 * \param script The script.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_async_function_new (RJS_Runtime *rt, RJS_Value *f, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env,
        RJS_PrivateEnv *priv_env)
{
    RJS_Realm            *realm = rjs_realm_current(rt);
    RJS_ScriptFuncObject *sfo;

    if (!proto)
        proto = rjs_o_AsyncFunction_prototype(realm);

    RJS_NEW(rt, sfo);

    rjs_script_func_object_init(rt, f, sfo, proto, script, sf, env, priv_env,
            &async_function_ops);

    return RJS_OK;
}
