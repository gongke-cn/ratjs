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

/*Scan the referenced things in the context.*/
static void
context_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Context *ctxt = ptr;

    if (ctxt->bot)
        rjs_gc_mark(rt, ctxt->bot);

    if (ctxt->realm)
        rjs_gc_mark(rt, ctxt->realm);

    rjs_gc_scan_value(rt, &ctxt->function);
}

/*Release the context.*/
static void
context_deinit (RJS_Runtime *rt, RJS_Context *ctxt)
{
}

/*Free the context.*/
static void
context_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Context *ctxt = ptr;

    context_deinit(rt, ctxt);

    RJS_DEL(rt, ctxt);
}

/*Context operation functions.*/
static const RJS_GcThingOps
context_ops = {
    RJS_GC_THING_CONTEXT,
    context_op_gc_scan,
    context_op_gc_free
};

/*Scan the referenced things in the script context.*/
static void
script_context_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ScriptContext *sc = ptr;

    context_op_gc_scan(rt, &sc->scb.context);

    rjs_gc_scan_value(rt, &sc->retv);

    if (sc->args)
        rjs_gc_scan_value_buffer(rt, sc->args, sc->argc);

    if (sc->script)
        rjs_gc_mark(rt, sc->script);

    if (sc->scb.lex_env)
        rjs_gc_mark(rt, sc->scb.lex_env);

    if (sc->scb.var_env)
        rjs_gc_mark(rt, sc->scb.var_env);

#if ENABLE_PRIV_NAME
    if (sc->scb.priv_env)
        rjs_gc_mark(rt, sc->scb.priv_env);
#endif /*ENABLE_PRIV_NAME*/
}

/*Release the script context.*/
static void
script_context_deinit (RJS_Runtime *rt, RJS_ScriptContext *sc)
{
    if (sc->args)
        RJS_DEL_N(rt, sc->args, sc->argc);

    context_deinit(rt, &sc->scb.context);
}

/*Free the script context.*/
static void
script_context_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ScriptContext *sc = ptr;

    script_context_deinit(rt, sc);

    RJS_DEL(rt, sc);
}

/*Script context operation functions.*/
static const RJS_GcThingOps
script_context_ops = {
    RJS_GC_THING_SCRIPT_CONTEXT,
    script_context_op_gc_scan,
    script_context_op_gc_free
};

#if ENABLE_GENERATOR || ENABLE_ASYNC

/*Release the generator context.*/
static void
generator_context_deinit (RJS_Runtime *rt, RJS_GeneratorContext *gc)
{
    rjs_list_remove(&gc->ln);

    script_context_deinit(rt, &gc->scontext);
    rjs_native_stack_deinit(rt, &gc->native_stack);
}

/*Scan the referenced things in the generator context.*/
static void
generator_context_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_GeneratorContext *gc = ptr;

    script_context_op_gc_scan(rt, &gc->scontext);

    rjs_gc_scan_native_stack(rt, &gc->native_stack);
}

#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

#if ENABLE_GENERATOR

/*Free the generator context.*/
static void
generator_context_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_GeneratorContext *gc = ptr;

    generator_context_deinit(rt, gc);

    RJS_DEL(rt, gc);
}

/*Generator context operation functions.*/
static const RJS_GcThingOps
generator_context_ops = {
    RJS_GC_THING_GENERATOR_CONTEXT,
    generator_context_op_gc_scan,
    generator_context_op_gc_free
};

#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC

/*Scan the referenced things in the async context.*/
static void
async_context_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncContext *ac = ptr;

    generator_context_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &ac->v0);
    rjs_gc_scan_value(rt, &ac->promise);
    rjs_gc_scan_value(rt, &ac->resolve);
    rjs_gc_scan_value(rt, &ac->reject);
}

/*Free the async context.*/
static void
async_context_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncContext *ac = ptr;

    rjs_promise_capability_deinit(rt, &ac->capability);
    generator_context_deinit(rt, &ac->gcontext);

    RJS_DEL(rt, ac);
}

/*Async context operation functions.*/
static const RJS_GcThingOps
async_context_ops = {
    RJS_GC_THING_ASYNC_CONTEXT,
    async_context_op_gc_scan,
    async_context_op_gc_free
};

#endif /*ENABLE_ASYNC*/

/*Initialize a context.*/
static void
context_init (RJS_Runtime *rt, RJS_Context *ctxt, RJS_Value *func)
{
    if (func)
        rjs_value_copy(rt, &ctxt->function, func);
    else
        rjs_value_set_null(rt, &ctxt->function);

    ctxt->realm = NULL;

    ctxt->bot = rt->rb.ctxt_stack;
    rt->rb.ctxt_stack = ctxt;
}

/**
 * Push a new execution context to the stack.
 * \param rt The current runtime.
 * \param func The function.
 * \return The new context.
 */
RJS_Context*
rjs_context_push (RJS_Runtime *rt, RJS_Value *func)
{
    RJS_Context *ctxt;

    RJS_NEW(rt, ctxt);

    context_init(rt, ctxt, func);

    rjs_gc_add(rt, ctxt, &context_ops);

    return ctxt;
}

/*Initialize the script context.*/
static void
script_context_init (RJS_Runtime *rt, RJS_ScriptContext *sc, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc)
{
    sc->script      = script;
    sc->script_func = sf;
    sc->scb.var_env = var_env;
    sc->scb.lex_env = lex_env;
    sc->ip          = sf->byte_code_start;

#if ENABLE_PRIV_NAME
    sc->scb.priv_env = priv_env;
#endif /*ENABLE_PRIV_NAME*/

    rjs_value_set_undefined(rt, &sc->retv);

    if (argc) {
        RJS_NEW_N(rt, sc->args, argc);

        rjs_value_buffer_copy(rt, sc->args, args, argc);

        sc->argc = argc;
    } else {
        sc->args = NULL;
        sc->argc = 0;
    }

    if (sf->reg_num)
        sc->regs = rjs_value_stack_push_n(rt, sf->reg_num);
    else
        sc->regs = NULL;

    context_init(rt, &sc->scb.context, func);
}

/**
 * Push a new script execution context to the stack.
 * \param rt The current runtime.
 * \param func The script function.
 * \param script The script.
 * \param sf The script function.
 * \param var_env The variable environment.
 * \param lex_env The lexical environment.
 * \param priv_env The private environment.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \return The new context.
 */
RJS_Context*
rjs_script_context_push (RJS_Runtime *rt, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc)
{
    RJS_ScriptContext *sc;

    RJS_NEW(rt, sc);

    script_context_init(rt, sc, func, script, sf, var_env, lex_env, priv_env, args, argc);

    rjs_gc_add(rt, sc, &script_context_ops);

    return &sc->scb.context;
}

#if ENABLE_GENERATOR || ENABLE_ASYNC

/*Initialize the generator context.*/
static void
generator_context_init (RJS_Runtime *rt, RJS_GeneratorContext *gc, RJS_Value *func,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc)
{
    /*Push the new native stack.*/
    rjs_native_stack_init(&gc->native_stack);

    gc->bot_native_stack     = rt->rb.curr_native_stack;
    rt->rb.curr_native_stack = &gc->native_stack;

    script_context_init(rt, &gc->scontext, func, script, sf, var_env, lex_env, priv_env, args, argc);

    rjs_list_append(&rt->gen_ctxt_list, &gc->ln);
}

/**
 * Solve all the generator contexts.
 * \param rt The current runtime.
 */
void
rjs_solve_generator_contexts (RJS_Runtime *rt)
{
    RJS_GeneratorContext *gc;

    rjs_list_foreach_c(&rt->gen_ctxt_list, gc, RJS_GeneratorContext, ln) {
        RJS_GcThing *gt = (RJS_GcThing*)gc;

        /*Clear the generator context.*/
        if (!(gt->next_flags & RJS_GC_THING_FL_MARKED)) {
            rjs_native_stack_clear(rt, &gc->native_stack);
        }
    }
}

#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

#if ENABLE_GENERATOR

/**
 * Push a new generator execution context to the stack.
 * \param rt The current runtime.
 * \param func The script function.
 * \param script The script.
 * \param sf The script function.
 * \param var_env The variable environment.
 * \param lex_env The lexical environment.
 * \param priv_env The private environment.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \return The new context.
 */
RJS_Context*
rjs_generator_context_push (RJS_Runtime *rt, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc)
{
    RJS_GeneratorContext *gc;

    RJS_NEW(rt, gc);

    generator_context_init(rt, gc, func, script, sf, var_env, lex_env, priv_env, args, argc);

    rjs_gc_add(rt, gc, &generator_context_ops);

    return &gc->scontext.scb.context;
}

#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC

/**
 * Register the async operation function.
 * \param rt The current runtime.
 * \param op The operation function.
 * \param i The integer parameter of the function.
 * \param v The value parameter of the function.
 */
void
rjs_async_context_set_op (RJS_Runtime *rt, RJS_AsyncOpFunc op, size_t i, RJS_Value *v)
{
    RJS_AsyncContext *ac = (RJS_AsyncContext*)rjs_context_running(rt);

    ac->op = op;
    ac->i0 = i;

    if (v)
        rjs_value_copy(rt, &ac->v0, v);
}

/**
 * Push a new async execution context to the stack.
 * \param rt The current runtime.
 * \param func The script function.
 * \param script The script.
 * \param sf The script function.
 * \param var_env The variable environment.
 * \param lex_env The lexical environment.
 * \param priv_env The private environment.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param pc The promise capability.
 * \return The new context.
 */
RJS_Context*
rjs_async_context_push (RJS_Runtime *rt, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc, RJS_PromiseCapability *pc)
{
    RJS_AsyncContext *ac;

    RJS_NEW(rt, ac);

    rjs_value_set_undefined(rt, &ac->promise);
    rjs_value_set_undefined(rt, &ac->resolve);
    rjs_value_set_undefined(rt, &ac->reject);

    rjs_promise_capability_init_vp(rt, &ac->capability, &ac->promise, &ac->resolve, &ac->reject);
    if (pc)
        rjs_promise_capability_copy(rt, &ac->capability, pc);

    ac->op = NULL;

    rjs_value_set_undefined(rt, &ac->v0);

    generator_context_init(rt, &ac->gcontext, func, script, sf, var_env, lex_env, priv_env,
            args, argc);

    rjs_gc_add(rt, ac, &async_context_ops);

    return &ac->gcontext.scontext.scb.context;
}

#endif /*ENABLE_ASYNC*/

/**
 * Popup the top context from the stack.
 * \param rt The current runtime.
 */
void
rjs_context_pop (RJS_Runtime *rt)
{
    RJS_Context    *ctxt = rt->rb.ctxt_stack;
#if ENABLE_GENERATOR || ENABLE_ASYNC
    RJS_GcThingType gtt  = ctxt->gc_thing.ops->type;
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

    assert(ctxt);

    rt->rb.ctxt_stack = ctxt->bot;

#if ENABLE_GENERATOR || ENABLE_ASYNC
    if (
#if ENABLE_GENERATOR
            (gtt == RJS_GC_THING_GENERATOR_CONTEXT)
#endif /*ENABLE_GENERATOR*/
#if ENABLE_GENERATOR && ENABLE_ASYNC
            ||
#endif /*ENABLE_GENERATOR && ENABLE_ASYNC*/
#if ENABLE_ASYNC
            (gtt == RJS_GC_THING_ASYNC_CONTEXT)
#endif /*ENABLE_ASYNC*/
            ) {
        RJS_GeneratorContext *gc = (RJS_GeneratorContext*)ctxt;

        rt->rb.curr_native_stack = gc->bot_native_stack;
    }
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/
}

/**
 * Restore the context to the stack.
 * \param rt The current runtime.
 * \param ctxt The context.
 */
void
rjs_context_restore (RJS_Runtime *rt, RJS_Context *ctxt)
{
#if ENABLE_GENERATOR || ENABLE_ASYNC
    RJS_GcThingType gtt = ctxt->gc_thing.ops->type;
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

    ctxt->bot         = rt->rb.ctxt_stack;
    rt->rb.ctxt_stack = ctxt;

#if ENABLE_GENERATOR || ENABLE_ASYNC
    if (
#if ENABLE_GENERATOR
            (gtt == RJS_GC_THING_GENERATOR_CONTEXT)
#endif /*ENABLE_GENERATOR*/
#if ENABLE_GENERATOR && ENABLE_ASYNC
            ||
#endif /*ENABLE_GENERATOR && ENABLE_ASYNC*/
#if ENABLE_ASYNC
            (gtt == RJS_GC_THING_ASYNC_CONTEXT)
#endif /*ENABLE_ASYNC*/
            ) {
        RJS_GeneratorContext *gc = (RJS_GeneratorContext*)ctxt;

        gc->bot_native_stack     = rt->rb.curr_native_stack;
        rt->rb.curr_native_stack = &gc->native_stack;
    }
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/
}

/**
 * Get the environment has this binding.
 * \param rt The current runtime.
 * \return This environment.
 */
RJS_Environment*
rjs_get_this_environment (RJS_Runtime *rt)
{
    RJS_ScriptContext *sc;
    RJS_Environment   *env;

    sc = (RJS_ScriptContext*)rjs_context_running(rt);

    for (env = sc->scb.lex_env; env; env = env->outer) {
        if (rjs_env_has_this_binding(rt, env))
            return env;
    }

    return NULL;
}

/**
 * Resolve this binding.
 * \param rt The current runtime.
 * \param[out] v Return this binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_resolve_this_binding (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Environment *env;

    env = rjs_get_this_environment(rt);
    assert(env);

    return rjs_env_get_this_binding(rt, env, v);
}

/**
 * Get the new target value.
 * \param rt The current runtime.
 * \param[out] nt Return the new target value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_new_target (RJS_Runtime *rt, RJS_Value *nt)
{
    RJS_FunctionEnv *fe;

    fe = (RJS_FunctionEnv*)rjs_get_this_environment(rt);
    assert(fe);

    rjs_value_copy(rt, nt, &fe->new_target);
    return RJS_OK;
}

/**
 * Get the super constructor value.
 * \param rt The current runtime.
 * \param[out] sc Return the super constructor value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_super_constructor (RJS_Runtime *rt, RJS_Value *sc)
{
    RJS_FunctionEnv *fe;

    fe = (RJS_FunctionEnv*)rjs_get_this_environment(rt);
    assert(fe);

    return rjs_object_get_prototype_of(rt, &fe->function, sc);
}

/**
 * Initialize the context stack in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_context_init (RJS_Runtime *rt)
{
    rt->rb.ctxt_stack = NULL;
}

/**
 * Release the context stack in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_context_deinit (RJS_Runtime *rt)
{
}

/**
 * Scan the referenced things in the context stack.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_context_stack (RJS_Runtime *rt)
{
    if (rt->rb.ctxt_stack)
        rjs_gc_mark(rt, rt->rb.ctxt_stack);
}
