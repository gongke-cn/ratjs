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
 * Error.
 */

#ifndef _RJS_ERROR_H_
#define _RJS_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup error Error
 * Error object
 * @{
 */

/**
 * Create a new type error.
 * \param rt The current runtime.
 * \param[out] err Return the type error object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_type_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...);

/**
 * Create a new range error.
 * \param rt The current runtime.
 * \param[out] err Return the range object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_range_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...);

/**
 * Create a new reference error.
 * \param rt The current runtime.
 * \param[out] err Return the reference error object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_reference_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...);

/**
 * Create a new syntax error.
 * \param rt The current runtime.
 * \param[out] err Return the syntax error object.
 * \param msg The error message.
 * \param ... Arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_syntax_error_new (RJS_Runtime *rt, RJS_Value *err, const char *msg, ...);

/**
 * Throw an error.
 * \param rt The current runtime.
 * \param err The error value.
 * \retval RJS_ERR.
 */
extern RJS_Result
rjs_throw (RJS_Runtime *rt, RJS_Value *err);

/**
 * Dump the error stack.
 * \param rt The current runtime.
 * \param fp The output file.
 */
extern void
rjs_dump_error_stack (RJS_Runtime *rt, FILE *fp);

/**
 * Catch the current error.
 * \param rt The current runtime.
 * \param[out] err Return the error value.
 * \retval RJS_TRUE Catch the error successfully.
 * \retval RJS_FALSE There is no error.
 */
extern RJS_Result
rjs_catch (RJS_Runtime *rt, RJS_Value *err);

/**
 * Throw a type error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR.
 */
extern RJS_Result
rjs_throw_type_error (RJS_Runtime *rt, const char *msg, ...);

/**
 * Throw a range error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR.
 */
extern RJS_Result
rjs_throw_range_error (RJS_Runtime *rt, const char *msg, ...);

/**
 * Throw a reference error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR.
 */
extern RJS_Result
rjs_throw_reference_error (RJS_Runtime *rt, const char *msg, ...);

/**
 * Throw a syntax error.
 * \param rt The current runtime.
 * \param msg The error message.
 * \param ... Arguments.
 * \return RJS_ERR.
 */
extern RJS_Result
rjs_throw_syntax_error (RJS_Runtime *rt, const char *msg, ...);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

