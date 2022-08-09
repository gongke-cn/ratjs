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
    UErrorCode err = U_ZERO_ERROR;

    conv->source = ucnv_open(enc_in, &err);
    if (U_FAILURE(err)) {
        RJS_LOGE("ucnv_open(\"%s\") failed: %s", enc_in, u_errorName(err));
        return RJS_ERR;
    }

    conv->target = ucnv_open(enc_out, &err);
    if (U_FAILURE(err)) {
        ucnv_close(conv->source);
        RJS_LOGE("ucnv_open(\"%s\") failed: %s", enc_out, u_errorName(err));
        return RJS_ERR;
    }

    conv->pivot_source = conv->pivot;
    conv->pivot_target = conv->pivot;

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
    char        *t, *tlim;
    const char  *s, *slim;
    const UChar *plim;
    UErrorCode   err = U_ZERO_ERROR;

    s    = *in;
    t    = *out;
    slim = s + *in_left;
    tlim = t + *out_left;
    plim = conv->pivot + RJS_N_ELEM(conv->pivot);

    ucnv_convertEx(conv->target, conv->source, &t, tlim, &s, slim,
            conv->pivot, &conv->pivot_source, &conv->pivot_target, plim,
            0, 0, &err);

    *in       = s;
    *out      = t;
    *in_left  = slim - s;
    *out_left = tlim - t;

    if (err == U_BUFFER_OVERFLOW_ERROR)
        return RJS_FALSE;

    if (U_FAILURE(err)) {
        RJS_LOGE("ucnv_convertEx failed: %s", u_errorName(err));
        return RJS_ERR;
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
    if (conv->source)
        ucnv_close(conv->source);
    if (conv->target)
        ucnv_close(conv->target);
}
