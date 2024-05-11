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
 * Memory allocate and free functions.
 */

#ifndef _RJS_MEM_INTERNAL_H_
#define _RJS_MEM_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Resize a memory buffer.
 * \param rt The current runtime.
 * \param old_ptr The old buffer's pointer.
 * \param old_size The old buffer's size.
 * \param new_size The new buffer's size.
 * \return The new buffer's pointer.
 * \retval NULL Allocate failed.
 */
static inline void*
rjs_realloc (RJS_Runtime *rt, void *old_ptr, size_t old_size, size_t new_size)
{
    void *new_ptr;

#if 0
    if (old_ptr) {
        void *ret_ptr;

        old_ptr = ((uint8_t*)old_ptr) - sizeof(size_t);
        assert(*(size_t*)old_ptr == old_size);

        if (new_size) {
            ret_ptr = realloc(old_ptr, new_size + sizeof(size_t));
            *(size_t*)ret_ptr = new_size;
            new_ptr = ((uint8_t*)ret_ptr) + sizeof(size_t);
        } else {
            free(old_ptr);
            new_ptr = NULL;
        }
    } else {
        void *ret_ptr;

        ret_ptr = realloc(NULL, new_size + sizeof(size_t));
        *(size_t*)ret_ptr = new_size;
        new_ptr = ((uint8_t*)ret_ptr) + sizeof(size_t);
    }
#else
    new_ptr = realloc(old_ptr, new_size);
#endif
    if (new_ptr || !new_size) {
        rt->mem_size += new_size - old_size;
        rt->mem_max_size = RJS_MAX(rt->mem_max_size, rt->mem_size);
    }

    return new_ptr;
}

/**
 * Resize a memory buffer.
 * The program will be terminated if not enough memory left.
 * \param rt The current runtime.
 * \param old_ptr The old buffer's pointer.
 * \param old_size The old buffer's size.
 * \param new_size The new buffer's size.
 * \return The new buffer's pointer.
 */
static inline void*
rjs_realloc_assert (RJS_Runtime *rt, void *old_ptr, size_t old_size, size_t new_size)
{
    void *new_ptr = rjs_realloc(rt, old_ptr, old_size, new_size);

    if (new_size && !new_ptr) {
        RJS_LOGF("allocate %"PRIdPTR"B memory failed", new_size);
        abort();
    }

    return new_ptr;
}

/**
 * Allocate a new buffer.
 * \param rt The current runtime.
 * \param size The new buffer's size.
 * \return The new buffer's pointer.
 * \retval NULL Allocate failed.
 */
static inline void*
rjs_alloc (RJS_Runtime *rt, size_t size)
{
    return rjs_realloc(rt, NULL, 0, size);
}

/**
 * Allocate a new buffer.
 * The program will be terminated if not enough memory left.
 * \param rt The current runtime.
 * \param size The new buffer's size.
 * \return The new buffer's pointer.
 */
static inline void*
rjs_alloc_assert (RJS_Runtime *rt, size_t size)
{
    return rjs_realloc_assert(rt, NULL, 0, size);
}

/**
 * Allocate a new buffer and fill it with 0.
 * \param rt The current runtime.
 * \param size The new buffer's size.
 * \return The new buffer's pointer.
 * \retval NULL Allocate failed.
 */
static inline void*
rjs_alloc_0 (RJS_Runtime *rt, size_t size)
{
    void *ptr;

    ptr = rjs_realloc(rt, NULL, 0, size);
    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}

/**
 * Allocate a new buffer and fill it with 0.
 * The program will be terminated if not enough memory left.
 * \param rt The current runtime.
 * \param size The new buffer's size.
 * \return The new buffer's pointer.
 */
static inline void*
rjs_alloc_assert_0 (RJS_Runtime *rt, size_t size)
{
    void *ptr;
    
    ptr = rjs_realloc_assert(rt, NULL, 0, size);
    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}

/**
 * Free an unused buffer.
 * \param rt The current runtime.
 * \param ptr The buffer's pointer.
 * \param size The buffer's size.
 */
static inline void
rjs_free (RJS_Runtime *rt, void *ptr, size_t size)
{
    rjs_realloc(rt, ptr, size, 0);
}

/**
 * Duplicate a 0 terminated characters buffer.
 * \param rt The current runtime.
 * \param ptr The character buffer.
 * \return The new character buffer.
 */
static inline void*
rjs_char_star_dup (RJS_Runtime *rt, const void *ptr)
{
    void   *nptr;
    size_t  len;

    if (!ptr)
        return NULL;

    len  = strlen(ptr);
    nptr = rjs_alloc_assert(rt, len + 1);
    memcpy(nptr, ptr, len + 1);

    return nptr;
}

/**
 * Free a 0 terminated chararacters buffer.
 * \param rt The current runtime.
 * \param ptr The character buffer to be freed.
 */
static inline void
rjs_char_star_free (RJS_Runtime *rt, void *ptr)
{
    if (ptr) {
        size_t len = strlen(ptr);

        rjs_free(rt, ptr, len + 1);
    }
}

/**Allocate a buffer for a type.*/
#define RJS_NEW(r, p)\
    ((p) = rjs_alloc_assert(r, sizeof(*(p))))

/**Allocate a buffer for a type and fill it with 0.*/
#define RJS_NEW_0(r, p)\
    ((p) = rjs_alloc_assert_0(r, sizeof(*(p))))

/**Allocate a buffer for a type array.*/
#define RJS_NEW_N(r, p, n)\
    ((p) = rjs_alloc_assert(r, sizeof(*(p)) * (n)))

/**Allocate a buffer for a type array and fill it with 0.*/
#define RJS_NEW_N0(r, p, n)\
    ((p) = rjs_alloc_assert_0(r, sizeof(*(p)) * (n)))

/**Free a type pointer.*/
#define RJS_DEL(r, p)\
    rjs_free(r, p, sizeof(*(p)))

/**Free a type array.*/
#define RJS_DEL_N(r, p, n)\
    rjs_free(r, p, sizeof(*(p)) * (n))

/**Resize a type aray.*/
#define RJS_RENEW(r, p, os, ns)\
    ((p) = rjs_realloc_assert(r, p, sizeof(*(p)) * (os), sizeof(*(p)) * ns))

/**
 * Initialize the memory resource in the rt.
 * \param rt The rt to be initialized.
 */
RJS_INTERNAL void
rjs_runtime_mem_init (RJS_Runtime *rt);

/**
 * Release the memory resource in the rt.
 * \param rt The rt to be released.
 */
RJS_INTERNAL void
rjs_runtime_mem_deinit (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

