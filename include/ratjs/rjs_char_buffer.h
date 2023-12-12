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
 * Character buffer.
 */

#ifndef _RJS_CHAR_BUFFER_H_
#define _RJS_CHAR_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup char_buf Character buffer
 * Character buffer
 * @{
 */

/**
 * Char buffer initialize value.
 */
#define RJS_CHAR_BUFFER_INIT {NULL, 0, 0}

/**
 * Initialize a character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer to be initialized.
 */
extern void
rjs_char_buffer_init (RJS_Runtime *rt, RJS_CharBuffer *cb);

/**
 * Release the character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer to be released.
 */
extern void
rjs_char_buffer_deinit (RJS_Runtime *rt, RJS_CharBuffer *cb);

/**
 * Clear the character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer to be cleared.
 */
static inline void
rjs_char_buffer_clear (RJS_Runtime *rt, RJS_CharBuffer *cb)
{
    cb->item_num = 0;
}

/**
 * Get the 0 terminated C string from a character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \return The 0 terminated C string.
 */
extern const char*
rjs_char_buffer_to_c_string (RJS_Runtime *rt, RJS_CharBuffer *cb);

/**
 * Append a character to the character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param c The character to be added.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_char_buffer_append_char (RJS_Runtime *rt, RJS_CharBuffer *cb, int c);

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
extern RJS_Result
rjs_char_buffer_append_chars (RJS_Runtime *rt, RJS_CharBuffer *cb,
        const char *chars, size_t len);

/**
 * Print message to a character buffer.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_char_buffer_printf (RJS_Runtime *rt, RJS_CharBuffer *cb,
        const char *fmt, ...);

/**
 * Print message to a character buffer with va_list argument.
 * \param rt The current runtime.
 * \param cb The character buffer.
 * \param fmt The output format pattern.
 * \param ap Variable arguments list.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_char_buffer_vprintf (RJS_Runtime *rt, RJS_CharBuffer *cb,
        const char *fmt, va_list ap);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

