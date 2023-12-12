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
 * Data block internal header.
 */

#ifndef _RJS_DATA_BLOCK_INTERNAL_H_
#define _RJS_DATA_BLOCK_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**The data block.*/
struct RJS_DataBlock_s {
    atomic_int      ref;          /**< Reference count.*/
    size_t          size;         /**< Size of the data block.*/
    int             flags;        /**< The flags of the data block.*/
#if ENABLE_SHARED_ARRAY_BUFFER
    pthread_mutex_t lock;         /**< The mutex.*/
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/
#if ENABLE_ATOMICS
    RJS_List        waiter_lists; /**< Waiter lists.*/
#endif /*ENABLE_ATOMICS*/
    uint8_t        *data;         /**< Data buffer.*/
};

#if ENABLE_ATOMICS

/**Waiter.*/
struct RJS_Waiter_s {
    RJS_List       ln;   /**< List node data.*/
    pthread_cond_t cond; /**< Condtion variable.*/
};

/**Waiter list.*/
struct RJS_WaiterList_s {
    RJS_List  ln;      /**< List node data.*/
    RJS_List  waiters; /**< Waiters.*/
    size_t    pos;     /**< The position of the waiter list.*/
};

/**
 * Get the waiter list.
 * \param rt The current runtime.
 * \param db The data block.
 * \param pos The value position in the buffer.
 * \return The waiter list.
 */
extern RJS_WaiterList*
rjs_get_waiter_list (RJS_Runtime *rt, RJS_DataBlock *db, size_t pos);

/**
 * Add a waiter to the waiter list.
 * \param rt The current runtime.
 * \param db The data block.
 * \param wl The waiter list.
 * \param timeout Wait timeout.
 * \retval RJS_TRUE When the waiter is notified.
 * \retval RJS_FALSE When timeout.
 */
extern RJS_Result
rjs_add_waiter (RJS_Runtime *rt, RJS_DataBlock *db, RJS_WaiterList *wl, RJS_Number timeout);

/**
 * Notify the waiter.
 * \param rt The current runtime.
 * \param w The waiter.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_notify_waiter (RJS_Runtime *rt, RJS_Waiter *w);

#endif /*ENABLE_ATOMICS*/

#ifdef __cplusplus
}
#endif

#endif

