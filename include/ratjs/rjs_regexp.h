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
 * Regular expression.
 */

#ifndef _RJS_REGEXP_H_
#define _RJS_REGEXP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup regexp Regular expression
 * Regular expression
 * @{
 */

/**
 * Allocate a new regular expression.
 * \param rt The current runtime.
 * \param nt The new target.
 * \param[out] re Return the regular expression object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_regexp_alloc (RJS_Runtime *rt, RJS_Value *nt, RJS_Value *re);

/**
 * Initialize the regular expression.
 * \param rt The current runtime.
 * \param re The regular expression object.
 * \param p The source string.
 * \param f The flags string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_regexp_initialize (RJS_Runtime *rt, RJS_Value *re, RJS_Value *p, RJS_Value *f);

/**
 * Create a new regular expression.
 * \param rt The current runtime.
 * \param p The source string.
 * \param f The flags string.
 * \param[out] re Return the regular expression.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_regexp_create (RJS_Runtime *rt, RJS_Value *p, RJS_Value *f, RJS_Value *re);

/**
 * Clone a regular expression.
 * \param rt The current runtime.
 * \param[out] dst The result regular expression.
 * \param src The source regular expression.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_regexp_clone (RJS_Runtime *rt, RJS_Value *dst, RJS_Value *src);

/**
 * Execute the regular expression.
 * \param rt The current runtime.
 * \param v The regular expression value.
 * \param str The string.
 * \param[out] rv Return the match result.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_regexp_exec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *str, RJS_Value *rv);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

