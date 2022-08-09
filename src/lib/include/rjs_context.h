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
 * Context internal header.
 */

#ifndef _RJS_CONTEXT_INTERNAL_H_
#define _RJS_CONTEXT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define ASYNC_OP_DEBUG() RJS_LOGD("%s", __FUNCTION__) 
#else
#define ASYNC_OP_DEBUG()
#endif

/**Script function.*/
typedef struct RJS_ScriptFunc_s RJS_ScriptFunc;

/**Script private environment.*/
typedef struct RJS_ScriptPrivEnv_s RJS_ScriptPrivEnv;

/**Script context.*/
typedef struct {
    RJS_ScriptContextBase scb;         /**< Script context base data.*/
    RJS_Script           *script;      /**< The script.*/
    RJS_ScriptFunc       *script_func; /**< The script function.*/
    RJS_Value            *regs;        /**< Registers.*/
    RJS_Value            *args;        /**< The arguments.*/
    size_t                argc;        /**< The arguments count.*/
    RJS_Value             retv;        /**< The return value.*/
    size_t                ip;          /**< The instruction pointer.*/
} RJS_ScriptContext;

/**Generator context.*/
typedef struct {
    RJS_ScriptContext scontext;         /**< Base script context data.*/
    RJS_List          ln;               /**< List node data.*/
    RJS_NativeStack   native_stack;     /**< The native stack.*/
    RJS_NativeStack  *bot_native_stack; /**< The bottom native stack.*/
} RJS_GeneratorContext;

/**Script call type.*/
typedef enum {
    RJS_SCRIPT_CALL_CONSTRUCT,        /**< Construct.*/
    RJS_SCRIPT_CALL_SYNC_START,       /**< Start the function.*/
    RJS_SCRIPT_CALL_GENERATOR_RESUME, /**< Resume the generator.*/
    RJS_SCRIPT_CALL_GENERATOR_RETURN, /**< Return in generator.*/
    RJS_SCRIPT_CALL_GENERATOR_THROW,  /**< Throw in generator.*/
    RJS_SCRIPT_CALL_ASYNC_START,      /**< Async function starting.*/
    RJS_SCRIPT_CALL_ASYNC_FULFILL,    /**< Await fultill*/
    RJS_SCRIPT_CALL_ASYNC_REJECT      /**< Await reject.*/
} RJS_ScriptCallType;

/**Async operation function.*/
typedef RJS_Result (*RJS_AsyncOpFunc) (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv);

/**Async context.*/
typedef struct {
    RJS_GeneratorContext   gcontext;   /**< Base generator context data.*/
    RJS_PromiseCapability  capability; /**< Promise capability.*/
    RJS_Value              promise;    /**< Promise value buffer.*/
    RJS_Value              resolve;    /**< Resolve value buffer.*/
    RJS_Value              reject;     /**< Reject value buffer.*/
    RJS_AsyncOpFunc        op;         /**< Operation.*/
    size_t                 i0;         /**< Integer value 0.*/
    RJS_Value              v0;         /**< Value 0.*/
} RJS_AsyncContext;

/**
 * Push a new execution context to the stack.
 * \param rt The current runtime.
 * \param func The function.
 * \return The new context.
 */
extern RJS_Context*
rjs_context_push (RJS_Runtime *rt, RJS_Value *func);

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
extern RJS_Context*
rjs_script_context_push (RJS_Runtime *rt, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc);

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
extern RJS_Context*
rjs_generator_context_push (RJS_Runtime *rt, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc);
#endif /*ENABLE_GENERTOR*/

#if ENABLE_ASYNC

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
extern RJS_Context*
rjs_async_context_push (RJS_Runtime *rt, RJS_Value *func, RJS_Script *script,
        RJS_ScriptFunc *sf, RJS_Environment *var_env, RJS_Environment *lex_env,
        RJS_PrivateEnv *priv_env, RJS_Value *args, size_t argc, RJS_PromiseCapability *pc);

/**
 * Register the async operation function.
 * \param rt The current runtime.
 * \param op The operation function.
 * \param i The integer parameter of the function.
 * \param v The value parameter of the function.
 */
extern void
rjs_async_context_set_op (RJS_Runtime *rt, RJS_AsyncOpFunc op, size_t i, RJS_Value *v);

#endif /*ENABLE_ASYNC*/

/**
 * Popup the top context from the stack.
 * \param rt The current runtime.
 */
extern void
rjs_context_pop (RJS_Runtime *rt);

/**
 * Restore the context to the stack.
 * \param rt The current runtime.
 * \param ctxt The context.
 */
extern void
rjs_context_restore (RJS_Runtime *rt, RJS_Context *ctxt);

#if ENABLE_GENERATOR || ENABLE_ASYNC
/**
 * Solve all the generator contexts.
 * \param rt The current runtime.
 */
extern void
rjs_solve_generator_contexts (RJS_Runtime *rt);
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

/**
 * Initialize the context stack in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_context_init (RJS_Runtime *rt);

/**
 * Release the context stack in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_context_deinit (RJS_Runtime *rt);

/**
 * Scan the referenced things in the context stack.
 * \param rt The current runtime.
 */
extern void
rjs_gc_scan_context_stack (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

