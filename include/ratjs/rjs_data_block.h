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
 * Data block.
 */

#ifndef _RJS_DATA_BLOCK_H_
#define _RJS_DATA_BLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup data_block Data block
 * Raw data block
 * @{
 */

/**The data block.*/
typedef struct RJS_DataBlock_s  RJS_DataBlock;

#if ENABLE_ATOMICS
/**Waiter.*/
typedef struct RJS_Waiter_s     RJS_Waiter;

/**Waiter list.*/
typedef struct RJS_WaiterList_s RJS_WaiterList;
#endif /*ENABLE_ATOMICS*/

/**
 * Get the buffer of the data block.
 * \param db The data block.
 * \return The buffer pointer.
 */
extern uint8_t*
rjs_data_block_get_buffer (RJS_DataBlock *db);

#if ENABLE_SHARED_ARRAY_BUFFER

/**
 * Check if the data block is shared.
 * \param db The data block.
 * \retval RJS_TRUE The data block is shared.
 * \retval RJS_FALSE The data block is not shared.
 */
extern RJS_Bool
rjs_data_block_is_shared (RJS_DataBlock *db);

/**
 * Lock the data block.
 * \param db The data block.
 */
extern void
rjs_data_block_lock (RJS_DataBlock *db);

/**
 * Unlock the data block.
 * \param db The data block.
 */
extern void
rjs_data_block_unlock (RJS_DataBlock *db);

#else /*!ENABLE_SHARED_ARRAY_BUFFER*/

static inline RJS_Bool
rjs_data_block_is_shared (RJS_DataBlock *db)
{
    return RJS_FALSE;
}

/**
 * Lock the data block.
 * \param db The data block.
 */
static inline void
rjs_data_block_lock (RJS_DataBlock *db)
{
}

/**
 * Unlock the data block.
 * \param db The data block.
 */
static inline void
rjs_data_block_unlock (RJS_DataBlock *db)
{
}

#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

/**
 * Get the data block's size.
 * \param db The data block.
 * \return The data block's size.
 */
extern size_t
rjs_data_block_get_size (RJS_DataBlock *db);

/**
 * Allocate a new data block.
 * \param size Size of the data block.
 * \param shared The data block is used for a shared buffer.
 * \return The new data block.
 * \retval NULL On error.
 */
extern RJS_DataBlock*
rjs_data_block_new (int64_t size, RJS_Bool shared);

/**
 * Free the data block.
 * \param db The data block to be freed.
 */
extern void
rjs_data_block_free (RJS_DataBlock *db);

/**
 * Increase the data block's reference count.
 * \param db The data block.
 * \return The data block.
 */
extern RJS_DataBlock*
rjs_data_block_ref (RJS_DataBlock *db);

/**
 * Decrease the data block's reference count.
 * The data block will be freed the the reference count == 0.
 * \param db The data block.
 */
extern void
rjs_data_block_unref (RJS_DataBlock *db);

#if ENABLE_ATOMICS
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
#endif /*ENABLE_ATIOMICS*/

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

