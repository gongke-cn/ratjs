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
 * Convert the characters' encoding and output to a character buffer.
 * \param rt The current runtime.
 * \param conv The convertor.
 * \param in The input characters' buffer.
 * \param in_len The characters number in the input buffer.
 * \param cb The output character buffer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_conv_to_buffer (RJS_Runtime *rt, RJS_Conv *conv, const char *in,
        size_t in_len, RJS_CharBuffer *cb)
{
    char      *c;
    size_t     left;
    RJS_Result r;
    RJS_Bool   overflow = RJS_FALSE;

    while (in_len || overflow) {
        size_t c_old, cn;

        left = cb->item_cap - cb->item_num;
        if ((left < in_len) || overflow) {
            size_t cap = RJS_MAX(32, RJS_MAX(in_len + cb->item_num, cb->item_cap * 2));

            rjs_vector_set_capacity(cb, cap, rt);

            left = cb->item_cap - cb->item_num;
        }

        c = cb->items + cb->item_num;

        c_old = left;

        r = rjs_conv_run(rt, conv, &in, &in_len, &c, &left);

        cn = c_old - left;
        cb->item_num += cn;

        if (r == RJS_ERR)
            return r;

        if (r == RJS_FALSE) {
            overflow = RJS_TRUE;
        } else {
            if (cn == 0)
                return RJS_ERR;
                
            overflow = RJS_FALSE;
        }
    }

    return RJS_OK;
}
