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
 * String internal header.
 */

#ifndef _RJS_STRING_INTERNAL_H_
#define _RJS_STRING_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**The string is a property key.*/
#define RJS_STRING_FL_PROP_KEY   1
/**The string use a static characters buffer.*/
#define RJS_STRING_FL_STATIC     2
/**The string is not an index.*/
#define RJS_STRING_FL_NOT_INDEX  4
/**The string is not a number.*/
#define RJS_STRING_FL_NOT_NUMBER 8

/**
 * Convert the string to array index.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[out] pi Return the index value.
 * \retval RJS_TRUE The string is an array index.
 * \retval RJS_FALSE The string is not an array index.
 */
extern RJS_Bool
rjs_string_to_index_internal (RJS_Runtime *rt, RJS_Value *v, int64_t *pi);

/**
 * Convert the string to array index.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[out] pi Return the index value.
 * \retval RJS_TRUE The string is an array index.
 * \retval RJS_FALSE The string is not an array index.
 */
static inline RJS_Bool
rjs_string_to_index (RJS_Runtime *rt, RJS_Value *v, int64_t *pi)
{
    if (rjs_value_is_index_string(rt, v)) {
        *pi = rjs_value_get_index_string(rt, v);
        return RJS_TRUE;
    } else {
        RJS_String *s = rjs_value_get_string(rt, v);

        if (s->flags & RJS_STRING_FL_NOT_INDEX)
            return RJS_FALSE;
    }

    return rjs_string_to_index_internal(rt, v, pi);
}

/**
 * Check if the string is a canonical numeric index string.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[pn] pn Return the number value.
 * \retval RJS_TRUE The string is a numeric index string.
 * \retval RJS_FALSE The string is not a numeric index string.
 */
extern RJS_Bool
rjs_canonical_numeric_index_string_internal (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn);

/**
 * Check if the string is a canonical numeric index string.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[pn] pn Return the number value.
 * \retval RJS_TRUE The string is a numeric index string.
 * \retval RJS_FALSE The string is not a numeric index string.
 */
static inline RJS_Bool
rjs_canonical_numeric_index_string (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn)
{
    if (rjs_value_is_index_string(rt, v)) {
        *pn = rjs_value_get_index_string(rt, v);
        return RJS_TRUE;
    } else {
        RJS_String *s = rjs_value_get_string(rt, v);

        if (s->flags & RJS_STRING_FL_NOT_NUMBER)
            return RJS_FALSE;
    }

    return rjs_canonical_numeric_index_string_internal(rt, v, pn);
}

#if ENABLE_BIG_INT

/**
 * Convert a string to big integer.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[out] bi Return the big integer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_to_big_int (RJS_Runtime *rt, RJS_Value *v, RJS_Value *bi);

#endif /*ENABLE_BIG_INT*/

/**
 * Get the hash key of a string value.
 * \param rt The current runtime.
 * \param v The string value.
 * \return The hash key code of the string.
 */
extern size_t
rjs_string_hash_key (RJS_Runtime *rt, RJS_Value *v);

/**
 * Get the substitution.
 * \param rt The current runtime.
 * \param str The origin string.
 * \param pos The position of the matched string.
 * \param captures The captured substrings.
 * \param rep_templ The replace template string.
 * \retval rv The result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_substitution (RJS_Runtime *rt, RJS_Value *str, size_t pos,
        RJS_Value *captures, RJS_Value *rep_templ, RJS_Value *rv);

/**
 * Load a string from a file.
 * \param rt The current runtime.
 * \param[out] str The result string.
 * \param fn The filename.
 * \param enc The character encoding of the file.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_from_file (RJS_Runtime *rt, RJS_Value *str, const char *fn, const char *enc);

/**
 * Initialize the string resource in the rt.
 * \param rt The rt to be initialized.
 */
extern void
rjs_runtime_string_init (RJS_Runtime *rt);

/**
 * Release the string resource in the rt.
 * \param rt The rt to be released.
 */
extern void
rjs_runtime_string_deinit (RJS_Runtime *rt);

/**
 * Scan the internal strings in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_gc_scan_internal_strings (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

