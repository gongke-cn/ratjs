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
 * Execution context.
 */

#ifndef _RJS_CONTEXT_H_
#define _RJS_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup context Context
 * Execution context
 * @{
 */

/**
 * Get the running execution context.
 * \param rt The current runtime.
 * \return The running context.
 */
static inline RJS_Context*
rjs_context_running (RJS_Runtime *rt)
{
    RJS_RuntimeBase *rb   = (RJS_RuntimeBase*)rt;
    RJS_Context     *ctxt = rb->ctxt_stack;

    return ctxt;
}

/**
 * Get the current realm.
 * \param rt The current runtime.
 * \return The current realm.
 */
static inline RJS_Realm*
rjs_realm_current (RJS_Runtime *rt)
{
    RJS_RuntimeBase *rb   = (RJS_RuntimeBase*)rt;
    RJS_Context     *ctxt = rb->ctxt_stack;

    return ctxt ? ctxt->realm : rb->bot_realm;
}

/**
 * Get the running execution context's lexical environment.
 * \param rt The current runtime.
 * \return The lexical environment.
 */
static inline RJS_Environment*
rjs_lex_env_running (RJS_Runtime *rt)
{
    RJS_Context           *ctxt = rjs_context_running(rt);
    RJS_ScriptContextBase *scb;

    assert((ctxt->gc_thing.ops->type == RJS_GC_THING_SCRIPT_CONTEXT)
            || (ctxt->gc_thing.ops->type == RJS_GC_THING_GENERATOR_CONTEXT)
            || (ctxt->gc_thing.ops->type == RJS_GC_THING_ASYNC_CONTEXT));

    scb = (RJS_ScriptContextBase*)ctxt;

    return scb->lex_env;
}

/**
 * Get the running execution context's variable environment.
 * \param rt The current runtime.
 * \return The variable environment.
 */
static inline RJS_Environment*
rjs_var_env_running (RJS_Runtime *rt)
{
    RJS_Context           *ctxt = rjs_context_running(rt);
    RJS_ScriptContextBase *scb;

    assert((ctxt->gc_thing.ops->type == RJS_GC_THING_SCRIPT_CONTEXT)
            || (ctxt->gc_thing.ops->type == RJS_GC_THING_GENERATOR_CONTEXT)
            || (ctxt->gc_thing.ops->type == RJS_GC_THING_ASYNC_CONTEXT));

    scb = (RJS_ScriptContextBase*)ctxt;

    return scb->var_env;
}

#if ENABLE_PRIV_NAME
/**
 * Get the running execution context's private environment.
 * \param rt The current runtime.
 * \return The lexical environment.
 */
static inline RJS_PrivateEnv*
rjs_private_env_running (RJS_Runtime *rt)
{
    RJS_Context           *ctxt = rjs_context_running(rt);
    RJS_ScriptContextBase *scb;

    assert((ctxt->gc_thing.ops->type == RJS_GC_THING_SCRIPT_CONTEXT)
            || (ctxt->gc_thing.ops->type == RJS_GC_THING_GENERATOR_CONTEXT)
            || (ctxt->gc_thing.ops->type == RJS_GC_THING_ASYNC_CONTEXT));

    scb = (RJS_ScriptContextBase*)ctxt;

    return scb->priv_env;
}
#endif /*ENABLE_PRIV_NAME*/

/**
 * Get the environment has this binding.
 * \param rt The current runtime.
 * \return This environment.
 */
extern RJS_Environment*
rjs_get_this_environment (RJS_Runtime *rt);

/**
 * Resolve this binding.
 * \param rt The current runtime.
 * \param[out] v Return this binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_resolve_this_binding (RJS_Runtime *rt, RJS_Value *v);

/**
 * Get the new target value.
 * \param rt The current runtime.
 * \param[out] nt Return the new target value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_new_target (RJS_Runtime *rt, RJS_Value *nt);

/**
 * Get the super constructor value.
 * \param rt The current runtime.
 * \param[out] sc Return the super constructor value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_super_constructor (RJS_Runtime *rt, RJS_Value *sc);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
