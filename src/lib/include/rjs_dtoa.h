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
 * Convert double to alpha internal header.
 */

#ifndef _RJS_DTOA_INTERNAL_H_
#define _RJS_DTOA_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the dtoa library.
 */
extern void
rjs_dtoa_init (void);

/**
 * Release the dtoa library.
 */
extern void
rjs_dtoa_deinit (void);

/**
 * Convert the string to double.
 */
extern double
rjs_strtod (const char *s00, char **se);

/**
 * Convert double to string.
 */
extern char*
rjs_dtoa (double d, int mode, int ndigits, int *decpt, int *sign, char **rve);

/**
 * Free the string returned by rjs_dtoa.
 */
extern void
rjs_freedtoa (char *s);

#ifdef __cplusplus
}
#endif

#endif
