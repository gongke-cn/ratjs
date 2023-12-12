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

#if ENABLE_ATOMICS
/*Free the waiter.*/
static void
waiter_free (RJS_Waiter *w)
{
    pthread_cond_destroy(&w->cond);

    free(w);
}
#endif /*ENABLE_ATOMICS*/

/**
 * Get the buffer of the data block.
 * \param rt The current runtime.
 * \param db The data block.
 * \return The buffer pointer.
 */
uint8_t*
rjs_data_block_get_buffer (RJS_DataBlock *db)
{
    return db->data;
}

#if ENABLE_SHARED_ARRAY_BUFFER

/**
 * Check if the data block is shared.
 * \param db The data block.
 * \retval RJS_TRUE The data block is shared.
 * \retval RJS_FALSE The data block is not shared.
 */
RJS_Bool
rjs_data_block_is_shared (RJS_DataBlock *db)
{
    return db->flags & RJS_DATA_BLOCK_FL_SHARED;
}

/**
 * Lock the data block.
 * \param db The data block.
 */
void
rjs_data_block_lock (RJS_DataBlock *db)
{
    if (db->flags & RJS_DATA_BLOCK_FL_SHARED)
        pthread_mutex_lock(&db->lock);
}

/**
 * Unlock the data block.
 * \param db The data block.
 */
void
rjs_data_block_unlock (RJS_DataBlock *db)
{
    if (db->flags & RJS_DATA_BLOCK_FL_SHARED)
        pthread_mutex_unlock(&db->lock);
}

#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

/**
 * Get the data block's size.
 * \param db The data block.
 * \return The data block's size.
 */
size_t
rjs_data_block_get_size (RJS_DataBlock *db)
{
    return db->size;
}

/**
 * Allocate a new data block.
 * \param size Size of the data block.
 * \param flags The data block's flags.
 * \return The new data block.
 * \retval NULL On error.
 */
RJS_DataBlock*
rjs_data_block_new (int64_t size, int flags)
{
    RJS_DataBlock *db;

    if ((size < 0) || (size > LONG_MAX))
        return NULL;

    db = malloc(sizeof(RJS_DataBlock));
    if (!db)
        return NULL;

    if (size) {
        db->data = malloc(size);
        if (!db->data) {
            free(db);
            return NULL;
        }
    } else {
        db->data = NULL;
    }

    atomic_store(&db->ref, 1);

    db->size  = size;
    db->flags = flags;

    if (size)
        memset(db->data, 0, size);

#if ENABLE_SHARED_ARRAY_BUFFER
    if (flags & RJS_DATA_BLOCK_FL_SHARED)
        pthread_mutex_init(&db->lock, NULL);
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

#if ENABLE_ATOMICS
    rjs_list_init(&db->waiter_lists);
#endif /*ENABLE_ATOMICS*/

    return db;
}

/**
 * Free the data block.
 * \param db The data block to be freed.
 */
void
rjs_data_block_free (RJS_DataBlock *db)
{
    if (db) {
#if ENABLE_ATOMICS
        /*Clear waiters.*/
        RJS_WaiterList *wl, *nwl;

        rjs_list_foreach_safe_c(&db->waiter_lists, wl, nwl, RJS_WaiterList, ln) {
            RJS_Waiter *w, *nw;

            rjs_list_foreach_safe_c(&wl->waiters, w, nw, RJS_Waiter, ln) {
                waiter_free(w);
            }

            free(wl);
        }
#endif /*ENABLE_ATOMICS*/

#if ENABLE_SHARED_ARRAY_BUFFER
        /*Free the mutex.*/
        if (db->flags & RJS_DATA_BLOCK_FL_SHARED)
            pthread_mutex_destroy(&db->lock);
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

        if (db->data && !(db->flags & RJS_DATA_BLOCK_FL_EXTERN))
            free(db->data);

        free(db);
    }
}

/**
 * Increase the data block's reference count.
 * \param db The data block.
 * \return The data block.
 */
RJS_DataBlock*
rjs_data_block_ref (RJS_DataBlock *db)
{
    atomic_fetch_add(&db->ref, 1);

    return db;
}

/**
 * Decrease the data block's reference count.
 * The data block will be freed the the reference count == 0.
 * \param db The data block.
 */
void
rjs_data_block_unref (RJS_DataBlock *db)
{
    if (atomic_fetch_sub(&db->ref, 1) == 1)
        rjs_data_block_free (db);
}

#if ENABLE_ATOMICS
/**
 * Get the waiter list.
 * \param rt The current runtime.
 * \param db The data block.
 * \param pos The value position in the buffer.
 * \return The waiter list.
 */
RJS_WaiterList*
rjs_get_waiter_list (RJS_Runtime *rt, RJS_DataBlock *db, size_t pos)
{
    RJS_WaiterList *wl, *find_wl = NULL;

    rjs_list_foreach_c(&db->waiter_lists, wl, RJS_WaiterList, ln) {
        if (wl->pos == pos) {
            find_wl = wl;
            break;
        }
    }

    if (!find_wl) {
        find_wl = malloc(sizeof(RJS_WaiterList));
        assert(find_wl);

        find_wl->pos = pos;
        rjs_list_init(&find_wl->waiters);

        rjs_list_append(&db->waiter_lists, &find_wl->ln);
    }

    return find_wl;
}

/**
 * Add a waiter to the waiter list.
 * \param rt The current runtime.
 * \param db The data block.
 * \param wl The waiter list.
 * \param timeout Wait timeout.
 * \retval RJS_TRUE When the waiter is notified.
 * \retval RJS_FALSE When timeout.
 */
RJS_Result
rjs_add_waiter (RJS_Runtime *rt, RJS_DataBlock *db, RJS_WaiterList *wl, RJS_Number timeout)
{
    RJS_Waiter      *w;
    struct timespec  ts;
    RJS_Result       r;

    w = malloc(sizeof(RJS_Waiter));
    assert(w);

    pthread_cond_init(&w->cond, NULL);

    rjs_list_append(&wl->waiters, &w->ln);

    if (timeout == INFINITY) {
        r = pthread_cond_wait(&w->cond, &db->lock);
    } else {
        uint64_t ms = timeout;

        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_sec  += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000000;

        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec ++;
            ts.tv_nsec -= 1000000000;
        }

        r = pthread_cond_timedwait(&w->cond, &db->lock, &ts);
    }

    rjs_list_remove(&w->ln);
    waiter_free(w);

    return (r == ETIMEDOUT) ? RJS_FALSE : RJS_TRUE;
}

/**
 * Notify the waiter.
 * \param rt The current runtime.
 * \param w The waiter.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_notify_waiter (RJS_Runtime *rt, RJS_Waiter *w)
{
    pthread_cond_signal(&w->cond);

    return RJS_OK;
}
#endif /*ENABLE_ATIOMICS*/
