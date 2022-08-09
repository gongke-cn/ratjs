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
 * Iterator.
 */

#ifndef _RJS_ITERATOR_H_
#define _RJS_ITERATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup itertor Iterator
 * Iterator
 * @{
 */

/**Iterator type.*/
typedef enum {
    RJS_ITERATOR_SYNC,  /**< Sync iterator.*/
    RJS_ITERATOR_ASYNC  /**< Async iterator.*/
} RJS_IteratorType;

/**Iterator method.*/
typedef struct {
    RJS_Value       *iterator;    /**< Iterator value.*/
    RJS_Value       *next_method; /**< Next method.*/
    RJS_IteratorType type;        /**< Iterator type.*/
    RJS_Bool         done;        /**< The iterator is done?*/
} RJS_Iterator;

/**
 * Initialize the iterator record.
 * \param rt The current runtime.
 * \param iter The iterator to be initialized.
 */
static inline void
rjs_iterator_init (RJS_Runtime *rt, RJS_Iterator *iter)
{
    iter->iterator    = rjs_value_stack_push(rt);
    iter->next_method = rjs_value_stack_push(rt);
    iter->done        = RJS_FALSE;
}

/**
 * Initialize the iterator record from the value buffers.
 * \param rt The current runtime.
 * \param iter The iterator to be initialized.
 * \param iterv The iterator value buffer.
 * \param methodv The method value buffer.
 */
static inline void
rjs_iterator_init_vp (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *iterv, RJS_Value *methodv)
{
    iter->iterator    = iterv;
    iter->next_method = methodv;
    iter->done        = RJS_FALSE;
}

/**
 * Release the iterator record.
 * \param rt The current runtime.
 * \param iter The iterator to be released
 */
static inline void
rjs_iterator_deinit (RJS_Runtime *rt, RJS_Iterator *iter)
{
}

/**
 * Get the iterator of the object.
 * \param rt The current runtime.
 * \param obj The object.
 * \param hint Sync or async.
 * \param method The method to get the iterator.
 * \param[out] iter Return the iterator record.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_iterator (RJS_Runtime *rt, RJS_Value *obj, RJS_IteratorType hint,
        RJS_Value *method, RJS_Iterator *iter);

/**
 * Move the iterator to the next position.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \param v Argument of the next method.
 * \param[out] rv The result object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iterator_next (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *v, RJS_Value *rv);

/**
 * Check if the iterator is completed.
 * \param rt The current runtime.
 * \param ir The iterator result.
 * \retval RJS_TRUE The iterator is completed.
 * \retval RJS_FALSE The iterator is not completed.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iterator_complete (RJS_Runtime *rt, RJS_Value *ir);

/**
 * Get the current value of the iterator.
 * \param rt The current runtime.
 * \param ir The iterator result.
 * \param[out] v Return the current value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The iterator is completed.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iterator_value (RJS_Runtime *rt, RJS_Value *ir, RJS_Value *v);

/**
 * Move the iterator to the next position and check if it is completed.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \param[out] rv The result value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The iterator is completed.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iterator_step (RJS_Runtime *rt, RJS_Iterator *iter, RJS_Value *rv);

/**
 * Close the iterator.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iterator_close (RJS_Runtime *rt, RJS_Iterator *iter);

/**
 * Create an iterator result object.
 * \param rt The current runtime.
 * \param v The current value.
 * \param done The iterator is done or not.
 * \param[out] rv Return the iterator result.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_iter_result_object (RJS_Runtime *rt, RJS_Value *v, RJS_Bool done, RJS_Value *rv);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

