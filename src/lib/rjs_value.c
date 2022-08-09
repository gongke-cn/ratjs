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

/**
 * Fill the value buffer with undefined.
 * \param rt The current runtime.
 * \param v The value buffer's pointer.
 * \param n Number of values in the buffer.
 */
void
rjs_value_buffer_fill_undefined (RJS_Runtime *rt, RJS_Value *v, size_t n)
{
    RJS_Value *p, *e;

    v = rjs_value_get_pointer(rt, v);
    p = v;
    e = v + n;
    
    while (p < e) {
        rjs_value_pointer_set_undefined(rt, p);
        p ++;
    }
}

/**
 * Scan the referenced things in a value buffer.
 * \param rt The current runtime.
 * \param v The value buffer.
 * \param n Number of values in the buffer.
 */
void
rjs_gc_scan_value_buffer (RJS_Runtime *rt, RJS_Value *v, size_t n)
{
    RJS_Value *p = v, *e = v + n;

    while (p < e) {
        rjs_gc_scan_value(rt, p);
        p ++;
    }
}
