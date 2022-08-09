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
 * JSON.
 */

#ifndef _RJS_JSON_H_
#define _RJS_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup json JSON
 * JSON parser and generator
 * @{
 */

/**
 * Create a JSON from a file.
 * \param rt The current runtime.
 * \param[out] res Store the output value.
 * \param filename The filename.
 * \param enc The file's character encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_json_from_file (RJS_Runtime *rt, RJS_Value *res, const char *filename, const char *enc);

/**
 * Parse the JSON from a string.
 * \param rt The current runtime.
 * \param[out] res Store the output value.
 * \param str The string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_json_from_string (RJS_Runtime *rt, RJS_Value *res, RJS_Value *str);

/**
 * Convert the value to JSON string.
 * \param rt The current runtime.
 * \param v The input value.
 * \param replacer The replacer function.
 * \param space The space value.
 * \param[out] str The result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_json_stringify (RJS_Runtime *rt, RJS_Value *v, RJS_Value *replacer,
        RJS_Value *space, RJS_Value *str);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

