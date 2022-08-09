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
 * Character encoding convertor internal header.
 */

#ifndef _RJS_CONV_INTERNAL_H_
#define _RJS_CONV_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#if ENC_CONV == ENC_CONV_ICU
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define RJS_ENC_UCHAR  "UTF-16LE"
        #define RJS_ENC_UC     "UTF-32LE"
    #else
        #define RJS_ENC_UCHAR  "UTF-16BE"
        #define RJS_ENC_UC     "UTF-32BE"
    #endif
#else
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define RJS_ENC_UCHAR  "UCS-2LE"
        #define RJS_ENC_UC     "UCS-4LE"
    #else
        #define RJS_ENC_UCHAR  "UCS-2BE"
        #define RJS_ENC_UC     "UCS-4BE"
    #endif
#endif

#define RJS_ENC_UTF8 "UTF-8"

/**Encoding operation functions.*/
typedef struct {
    char *name; /**< Name of the encoding.*/
    /**Convert to unicode.*/
    RJS_Result (*to_uc)   (uint8_t *c, size_t len, uint32_t *puc);
    /**Convert from unicode.*/
    RJS_Result (*from_uc) (uint32_t uc, uint8_t *c, size_t len);
} RJS_EncOps;

/**Encoding convertor.*/
typedef struct {
#if ENC_CONV == ENC_CONV_ICU
    UConverter *source;       /**< The target converter.*/
    UConverter *target;       /**< The source converter.*/
    UChar       pivot[16];    /**< Pivot buffer.*/
    UChar      *pivot_source; /**< Pivot source pointer.*/
    UChar      *pivot_target; /**< Pivot target pointer.*/
#elif ENC_CONV == ENC_CONV_ICONV
    iconv_t      cd;   /**< The iconv device.*/
#else /*ENC_CONV == ENC_CONV_INTERNAL*/
    const RJS_EncOps *enc_in;  /**< Input encoding.*/
    const RJS_EncOps *enc_out; /**< Output encoding.*/
#endif
} RJS_Conv;

/**
 * Initialize a character encoding convertor.
 * \param rt The current runtime.
 * \param conv The convertor to be initlized.
 * \param enc_in The input encoding.
 * \param enc_out The output encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_conv_init (RJS_Runtime *rt, RJS_Conv *conv, const char *enc_in, const char *enc_out);

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
extern RJS_Result
rjs_conv_run (RJS_Runtime *rt, RJS_Conv *conv, const char **in,
        size_t *in_left, char **out, size_t *out_left);

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
extern RJS_Result
rjs_conv_to_buffer (RJS_Runtime *rt, RJS_Conv *conv, const char *in,
        size_t in_len, RJS_CharBuffer *cb);

/**
 * Release the character convertor.
 * \param rt The current runtime.
 * \param conv The convertor to be released.
 */
extern void
rjs_conv_deinit (RJS_Runtime *rt, RJS_Conv *conv);

#ifdef __cplusplus
}
#endif

#endif

