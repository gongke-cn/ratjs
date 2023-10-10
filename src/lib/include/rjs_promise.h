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
 * Promise internal header.
 */

#ifndef _RJS_PROMISE_INTERNAL_H_
#define _RJS_PROMISE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**State of the promise.*/
typedef enum {
    RJS_PROMISE_STATE_PENDING,   /**< The promise is pending.*/
    RJS_PROMISE_STATE_FULFILLED, /**< The promise is fulfilled.*/
    RJS_PROMISE_STATE_REJECTED   /**< The promise is rejected.*/
} RJS_PromiseState;

/**Promise reaction type.*/
typedef enum {
    RJS_PROMISE_REACTION_FULFILL, /**< Fulfill reaction.*/
    RJS_PROMISE_REACTION_REJECT   /**< Reject reaction.*/
} RJS_PromiseRectionType;

/**Promise reaction.*/
typedef struct {
    RJS_List               ln;         /**< List node data.*/
    RJS_PromiseCapability  capability; /**< Capability.*/
    RJS_Value              promise;    /**< Promise value buffer.*/
    RJS_Value              resolve;    /**< Resolve value buffer.*/
    RJS_Value              reject;     /**< Reject value buffer.*/
    RJS_PromiseRectionType type;       /**< Type.*/
    RJS_Value              handler;    /**< Handler function.*/
} RJS_PromiseReaction;

/**The promise.*/
typedef struct {
    RJS_Object        object;            /**< Base object data.*/
    RJS_PromiseState  state;             /**< The state of the promise.*/
    RJS_Value         result;            /**< Result of the promise.*/
    RJS_List          fulfill_reactions; /**< Fulfill reactions list.*/
    RJS_List          reject_reactions;  /**< Reject reactions list.*/
    RJS_Bool          is_handled;        /**< Whether the promise has ever had a fulfillment or rejection handler.*/
} RJS_Promise;

/**
 * Create a new promise.
 * \param rt The current runtime.
 * \param[out] v Return the new promise.
 * \param exec The executor.
 * \param new_target The new target.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_promise_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *exec, RJS_Value *new_target);

/**
 * Perform promise then operation.
 * \param rt The current runtime.
 * \param promise The promise.
 * \param fulfill On fulfill callback function.
 * \param reject On reject callback function.
 * \param result_pc Result promise capability.
 * \param[out] rpromise The result promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_perform_proimise_then (RJS_Runtime *rt, RJS_Value *promise, RJS_Value *fulfill, RJS_Value *reject,
        RJS_PromiseCapability *result_pc, RJS_Value *rpromise);

/**
 * Invoke reject if arupt.
 * \param rt The current runtime.
 * \param r The result.
 * \param pc The promise capability.
 * \param rv The return value.
 * \return Return r.
 */
extern RJS_Result
if_abrupt_reject_promise (RJS_Runtime *rt, RJS_Result r, RJS_PromiseCapability *pc, RJS_Value *rv);

#ifdef __cplusplus
}
#endif

#endif

