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
 * Unicode character buffer.
 */

#ifndef _RJS_UCHAR_BUFFER_H_
#define _RJS_UCHAR_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup uchar_buf Unicode character buffer
 * Unicode character buffer
 * @{
 */

/**
 * Initialize an unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer to be initialized.
 */
extern void
rjs_uchar_buffer_init (RJS_Runtime *rt, RJS_UCharBuffer *ucb);

/**
 * Release an unused unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer to be released.
 */
extern void
rjs_uchar_buffer_deinit (RJS_Runtime *rt, RJS_UCharBuffer *ucb);

/**
 * Clear the unicode character buffer.
 * \param rt The current runtime.
 * \param ucb the unicode character buffer to be cleared.
 */
static inline void
rjs_uchar_buffer_clear (RJS_Runtime *rt, RJS_UCharBuffer *ucb)
{
    ucb->item_num = 0;
}

/**
 * Append an unicode to the unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer.
 * \param uc The unicode.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_uchar_buffer_append_uc (RJS_Runtime *rt, RJS_UCharBuffer *ucb,
        int uc);

/**
 * Append an unicode character to the unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer.
 * \param uchar The unicode character.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_uchar_buffer_append_uchar (RJS_Runtime *rt, RJS_UCharBuffer *ucb,
        RJS_UChar uchar);

/**
 * Append unicode characters to the unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer.
 * \param uchars The unicode characters' pointer.
 * \param len Unicode characters' number in the buffer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_uchar_buffer_append_uchars (RJS_Runtime *rt, RJS_UCharBuffer *ucb,
        const RJS_UChar *uchars, size_t len);

/**
 * Append unicode characters in a string to the unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer.
 * \param str The string to be pushed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_uchar_buffer_append_string (RJS_Runtime *rt, RJS_UCharBuffer *ucb,
        RJS_Value *str);

/**
 * Append characters to the unicode character buffer.
 * \param rt The current runtime.
 * \param ucb The unicode character buffer.
 * \param chars The characters to be added.
 * \param len Characters' number to be added. -1 means the characters is end with 0.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_uchar_buffer_append_chars (RJS_Runtime *rt, RJS_UCharBuffer *ucb,
        const char *chars, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

