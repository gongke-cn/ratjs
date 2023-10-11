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
 * Generator internal header.
 */

#ifndef _RJS_GENERATOR_INTERNAL_H_
#define _RJS_GENERATOR_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Generator state.*/
typedef enum {
    RJS_GENERATOR_STATE_UNDEFINED,       /**< Undefined.*/
    RJS_GENERATOR_STATE_SUSPENDED_START, /**< Start.*/
    RJS_GENERATOR_STATE_SUSPENDED_YIELD, /**< Yield.*/
    RJS_GENERATOR_STATE_EXECUTING,       /**< Executing.*/
    RJS_GENERATOR_STATE_AWAIT_RETURN,    /**< Async generator wait return.*/
    RJS_GENERATOR_STATE_COMPLETED        /**< Completed.*/
} RJS_GeneratorState;

/**Generator abrupt type.*/
typedef enum {
    RJS_GENERATOR_ABRUPT_RETURN, /**< Return from the generator.*/
    RJS_GENERATOR_ABRUPT_THROW   /**< Throw an error.*/
} RJS_GeneratorAbruptType;

/**Generator request type.*/
typedef enum {
    RJS_GENERATOR_REQUEST_NEXT,   /**< Run the next block.*/
    RJS_GENERATOR_REQUEST_RETURN, /**< Return.*/
    RJS_GENERATOR_REQUEST_THROW,  /**< Throw an error.*/
    RJS_GENERATOR_REQUEST_END     /**< End the loop.*/
} RJS_GeneratorRequestType;

/**Generator.*/
typedef struct {
    RJS_ScriptFuncObject     sfo;           /**< Base script function object data.*/
    RJS_GeneratorState       state;         /**< State.*/
    RJS_Context             *context;       /**< The context.*/
    RJS_Value                brand;         /**< The brand string.*/
    RJS_Iterator             iterator;      /**< The iterator record.*/
    RJS_Value                iteratorv;     /**< The iterator value.*/
    RJS_Value                nextv;         /**< The next method.*/
    RJS_Value                receivedv;     /**< Received value.*/
    RJS_GeneratorRequestType received_type; /**< Received type.*/
} RJS_Generator;

#if ENABLE_ASYNC

/**Async generator request.*/
typedef struct {
    RJS_List                 ln;         /**< List node data.*/
    RJS_GeneratorRequestType type;       /**< Request type.*/
    RJS_Value                value;      /**< Value.*/
    RJS_PromiseCapability    capability; /**< Promise capability.*/
    RJS_Value                promise;    /**< Promise value buffer.*/
    RJS_Value                resolve;    /**< Resolve value buffer.*/
    RJS_Value                reject;     /**< Reject value buffer.*/
} RJS_AsyncGeneratorRequest;

/**Async generator.*/
typedef struct {
    RJS_Generator generator; /**< Base generator.*/
    RJS_List      queue;     /**< Request queue.*/
} RJS_AsyncGenerator;

#endif /*ENABLE_ASYNC*/

/**
 * Create a new generator function object.
 * \param rt The current runtime.
 * \param[out] f Return the new generator function object.
 * \param proto The prototype.
 * \param script The script.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_generator_function_new (RJS_Runtime *rt, RJS_Value *f, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env,
        RJS_PrivateEnv *priv_env);

/**
 * Yield the generator.
 * \param rt The current runtime.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_SUSPEND Suspended.
 */
RJS_INTERNAL RJS_Result
rjs_yield (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Resume the yield expression.
 * \param rt The current runtime.
 * \param[out] result The result value.
 * \param[out] rv The return value.
 * \retval RJS_NEXT Resumed.
 * \retval RJS_THROW An error throwed.
 * \retval RJS_RETURN Return from the generator.
 */
RJS_INTERNAL RJS_Result
rjs_yield_resume (RJS_Runtime *rt, RJS_Value *result, RJS_Value *rv);

/**
 * Start iterator yield operation.
 * \param rt The current runtime.
 * \param v The input value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_iterator_yield_start (RJS_Runtime *rt, RJS_Value *v);

/**
 * Next iterator yield operation.
 * \param rt The current runtime.
 * \param[out] rv The return value.
 * \retval RJS_NEXT The iterator is done.
 * \retval RJS_SUSPEND The iterator is not done.
 * \retval RJS_THROW Throw an error.
 * \retval RJS_RETURN Return from the generator.
 */
RJS_INTERNAL RJS_Result
rjs_iterator_yield_next (RJS_Runtime *rt, RJS_Value *rv);

#if ENABLE_ASYNC

/**
 * Run an async generator complete step.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \param type Request type.
 * \param rv The return value.
 * \param done The generator is done.
 * \param realm The realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_async_generator_complete_step (RJS_Runtime *rt, RJS_Value *gv, RJS_GeneratorRequestType type,
        RJS_Value *rv, RJS_Bool done, RJS_Realm *realm);

/**
 * Drain the async generator's request queue.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_async_generator_drain_queue (RJS_Runtime *rt, RJS_Value *gv);

/**
 * AsyncGenerator.prototype.next operation.
 * \param rt The current runtime.
 * \param ag The async generator.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_async_generator_next (RJS_Runtime *rt, RJS_Value *ag, RJS_Value *v, RJS_Value *rv);

/**
 * AsyncGenerator.prototype.return operation.
 * \param rt The current runtime.
 * \param ag The async generator.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_async_generator_return (RJS_Runtime *rt, RJS_Value *ag, RJS_Value *v, RJS_Value *rv);

/**
 * AsyncGenerator.prototype.throw operation.
 * \param rt The current runtime.
 * \param ag The async generator.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_async_generator_throw (RJS_Runtime *rt, RJS_Value *ag, RJS_Value *v, RJS_Value *rv);

#endif /*ENABLE_ASYNC*/

/**
 * Create and start a generator.
 * \param rt The current runtime.
 * \param[out] rv Return the generator.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_generator_start (RJS_Runtime *rt, RJS_Value *rv);

/**
 * Resume the generator.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \param v The parameter value.
 * \param brand The brand.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_generator_resume (RJS_Runtime *rt, RJS_Value *gv, RJS_Value *v, RJS_Value *brand, RJS_Value *rv);

/**
 * Resume the generator with abrupt.
 * \param rt The current runtime.
 * \param gv The generator value.
 * \param type The abrupt type.
 * \param v The parameter value.
 * \param brand The brand.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_generator_resume_abrupt (RJS_Runtime *rt, RJS_Value *gv, RJS_GeneratorAbruptType type,
        RJS_Value *v, RJS_Value *brand, RJS_Value *rv);

#ifdef __cplusplus
}
#endif

#endif

