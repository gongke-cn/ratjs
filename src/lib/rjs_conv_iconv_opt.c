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
 * Initialize a character encoding convertor.
 * \param rt The current runtime.
 * \param conv The convertor to be initlized.
 * \param enc_in The input encoding.
 * \param enc_out The output encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_conv_init (RJS_Runtime *rt, RJS_Conv *conv, const char *enc_in, const char *enc_out)
{
    conv->cd = iconv_open(enc_out, enc_in);
    if (conv->cd == (iconv_t)-1) {
        RJS_LOGE("\"iconv_open\" from %s to %s failed", enc_in, enc_out);
        return RJS_ERR;
    }

    return RJS_OK;
}

/**
 * Convert encoding.
 * \param rt The current runtime.
 * \param conv The convertor.
 * \param in The input characters' pointer.
 * \param in_left The left characters number in the input buffer.
 * \param out The output characters' pointer.
 * \param out_left The left space of the output buffer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE The output buffer is not big enough.
 */
RJS_Result
rjs_conv_run (RJS_Runtime *rt, RJS_Conv *conv, const char **in,
        size_t *in_left, char **out, size_t *out_left)
{
    size_t r;

    r = iconv(conv->cd, (char**)in, in_left, out, out_left);
    if (r == (size_t)-1) {
        if (errno == E2BIG)
            return RJS_FALSE;

        if (errno != EINVAL) {
            RJS_LOGE("iconv failed");
            return RJS_ERR;
        }
    }

    return RJS_OK;
}

/**
 * Release the character convertor.
 * \param rt The current runtime.
 * \param conv The convertor to be released.
 */
void
rjs_conv_deinit (RJS_Runtime *rt, RJS_Conv *conv)
{
    if (conv->cd != (iconv_t)-1)
        iconv_close(conv->cd);
}
