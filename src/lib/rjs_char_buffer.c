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
 * Initialize a character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer to be initialized.
 */
void
rjs_char_buffer_init (RJS_Runtime *rt, RJS_CharBuffer *cb)
{
    rjs_vector_init(cb);
}

/**
 * Release the character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer to be released.
 */
void
rjs_char_buffer_deinit (RJS_Runtime *rt, RJS_CharBuffer *cb)
{
    rjs_vector_deinit(cb, rt);
}

/**
 * Get the 0 terminated C string from a character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \return The 0 terminated C string.
 */
const char*
rjs_char_buffer_to_c_string (RJS_Runtime *rt, RJS_CharBuffer *cb)
{
    rjs_vector_set_capacity(cb, cb->item_num + 1, rt);

    cb->items[cb->item_num] = 0;

    return cb->items;
}

/**
 * Append a character to the character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param c The character to be added.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_char_buffer_append_char (RJS_Runtime *rt, RJS_CharBuffer *cb, int c)
{
    size_t p = cb->item_num;

    rjs_vector_resize(cb, cb->item_num + 1, rt);

    cb->items[p] = c;

    return RJS_OK;
}

/**
 * Append characters to the character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param chars The characters to be added.
 * \param len Characters' number to be added.
 * If len == -1, means chars is terminated by 0.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_char_buffer_append_chars (RJS_Runtime *rt, RJS_CharBuffer *cb,
        const char *chars, size_t len)
{
    size_t p = cb->item_num;

    if (len == (size_t)-1)
        len = strlen(chars);

    rjs_vector_resize(cb, cb->item_num + len, rt);

    RJS_ELEM_CPY(cb->items + p, chars, len);

    return RJS_OK;
}

/**
 * Print message to a character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_char_buffer_printf (RJS_Runtime *rt, RJS_CharBuffer *cb,
        const char *fmt, ...)
{
    va_list    ap;
    RJS_Result r;

    va_start(ap, fmt);

    r = rjs_char_buffer_vprintf(rt, cb, fmt, ap);

    va_end(ap);

    return r;
}

/**
 * Print message to a character buffer with va_list argument.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param fmt The output format pattern.
 * \param ap Variable arguments list.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_char_buffer_vprintf (RJS_Runtime *rt, RJS_CharBuffer *cb,
        const char *fmt, va_list ap)
{
    size_t  space = cb->item_cap - cb->item_num;
    char   *c;
    int     n;
    va_list ap_store;

    c = cb->items + cb->item_num;

    va_copy(ap_store, ap);

    n = vsnprintf(c, space, fmt, ap);
    if (n >= space) {
        rjs_vector_set_capacity(cb, cb->item_num + n + 1, rt);

        space = cb->item_cap - cb->item_num;
        c     = cb->items + cb->item_num;

        n = vsnprintf(c, space, fmt, ap_store);

        assert(n < space);
    }
    
    cb->item_num += n;

    va_end(ap_store);

    return RJS_OK;
}
