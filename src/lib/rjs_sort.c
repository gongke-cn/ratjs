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

/*Merge the 2 sorted arrays.*/
static RJS_Result
merge (void *base, size_t size, RJS_CompareFunc func, void *arg,
        void *tmp, size_t min, size_t mid, size_t max)
{
    uint8_t          *l, *r, *m, *e, *t;
    size_t            n;
    RJS_CompareResult cr;

    l = ((uint8_t*)base) + size * min;
    m = ((uint8_t*)base) + size * mid;
    r = m + size;
    e = ((uint8_t*)base) + size * max;
    t = tmp;

    while ((l <= m) && (r <= e)) {
        if ((cr = func(l, r, arg)) == RJS_ERR)
            return cr;

        if (cr != RJS_COMPARE_GREATER) {
            memcpy(t, l, size);
            l += size;
        } else {
            memcpy(t, r, size);
            r += size;
        }

        t += size;
    }

    if (l <= m) {
        n = m - l + size;
        memcpy(t, l, n);
        t += n;
    }

    if (r <= e) {
        n = e - r + size;
        memcpy(t, r, n);
        t += n;
    }

    l = ((uint8_t*)base) + size * min;
    memcpy(l, tmp, (max - min + 1) * size);
    return RJS_OK;
}

/*Sort function.*/
static RJS_Result
sort (void *base, size_t size, RJS_CompareFunc func, void *arg,
        void *tmp, size_t min, size_t max)
{
    size_t     mid;
    RJS_Result r;

    if (max <= min)
        return RJS_OK;

    mid = (min + max) >> 1;

    if ((r = sort(base, size, func, arg, tmp, min, mid)) == RJS_ERR)
        return r;

    if ((r = sort(base, size, func, arg, tmp, mid + 1, max)) == RJS_ERR)
        return r;

    return merge(base, size, func, arg, tmp, min, mid, max);
}

/**
 * Sort an array.
 * \param base The base of the array.
 * \param num Number of items in the array.
 * \param size The item size of the array.
 * \param compare Item compare function.
 * \param arg Argument. 
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_sort (void *base, size_t num, size_t size, RJS_CompareFunc func, void *arg)
{
    void      *tmp;
    RJS_Result r;

    if (num < 2)
        return RJS_OK;

    tmp = malloc(num * size);
    assert(tmp);

    r = sort(base, size, func, arg, tmp, 0, num - 1);

    free(tmp);
    return r;
}
