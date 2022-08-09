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
 * Promise.
 */

#ifndef _RJS_PROMISE_H_
#define _RJS_PROMISE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup promise Promise
 * Promise
 * @{
 */

/**
 * Initialize the promise capability.
 * \param rt The current runtime.
 * \param pc The promise capability to be initialized.
 */
static inline void
rjs_promise_capability_init (RJS_Runtime *rt, RJS_PromiseCapability *pc)
{
    RJS_Value *v = rjs_value_stack_push_n(rt, 3);

    pc->promise = v;
    pc->resolve = rjs_value_buffer_item(rt, v, 1);
    pc->reject  = rjs_value_buffer_item(rt, v, 2);
}

/**
 * Initialize the promise capability from value pointers.
 * \param rt The current runtime.
 * \param pc The promise capability to be initialized.
 * \param promise The promise value's pointer.
 * \param resolve The resolve value's pointer.
 * \param reject The reject value's pointer.
 */
static inline void
rjs_promise_capability_init_vp (RJS_Runtime *rt, RJS_PromiseCapability *pc,
        RJS_Value *promise, RJS_Value *resolve, RJS_Value *reject)
{
    pc->promise = promise;
    pc->resolve = resolve;
    pc->reject  = reject;
}

/**
 * Release the promise capability.
 * \param rt The current runtime.
 * \param pc The promise capability to be released.
 */
static inline void
rjs_promise_capability_deinit (RJS_Runtime *rt, RJS_PromiseCapability *pc)
{
}

/**
 * Copy the promise capability.
 * \param rt The current runtime.
 * \param dst The destination promise capability.
 * \param src The source promise capability.
 */
static inline void
rjs_promise_capability_copy (RJS_Runtime *rt, RJS_PromiseCapability *dst, RJS_PromiseCapability *src)
{
    rjs_value_copy(rt, dst->promise, src->promise);
    rjs_value_copy(rt, dst->resolve, src->resolve);
    rjs_value_copy(rt, dst->reject, src->reject);
}

/**
 * Create a new promise capability.
 * \param rt The current runtime.
 * \param constr The promise constructor.
 * \param[out] pc Return the new promise capability.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_new_promise_capability (RJS_Runtime *rt, RJS_Value *constr, RJS_PromiseCapability *pc);

/**
 * Resolve the promise.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param v The resolve function's parameter value.
 * \param[out] promise Return the promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_promise_resolve (RJS_Runtime *rt, RJS_Value *constr, RJS_Value *v, RJS_Value *promise);

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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

