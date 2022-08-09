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
 * String.
 */

#ifndef _RJS_STRING_H_
#define _RJS_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup string String
 * String
 * @{
 */

/**Trim the prefixed space characters.*/
#define RJS_STRING_TRIM_START   1
/**Trim the postfixed space characters.*/
#define RJS_STRING_TRIM_END     2

/**Pad position.*/
typedef enum {
    RJS_STRING_PAD_START, /**< Pad at the beginning of the string.*/
    RJS_STRING_PAD_END    /**< Pad at the end of the string.*/
} RJS_StringPadPosition;

/**
 * Get the length of the string.
 * \param rt The current runtime.
 * \param v The string value.
 * \return The length of the string.
 */
static inline size_t
rjs_string_get_length (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_String *s = rjs_value_get_string(rt, v);

    return s->length;
}

/**
 * Get a character in the string.
 * \param rt The current runtime.
 * \param v The string value.
 * \param idx The character's index.
 * \return The unicode character.
 */
static inline int
rjs_string_get_uchar (RJS_Runtime *rt, RJS_Value *v, size_t idx)
{
    RJS_String *s = rjs_value_get_string(rt, v);

    assert(idx < s->length);

    return s->uchars[idx];
}

/**
 * Get the unicode character buffer of the string.
 * \param rt The current runtime.
 * \param v The string value.
 * \return The unicode character buffer.
 */
static inline const RJS_UChar*
rjs_string_get_uchars (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_String *s = rjs_value_get_string(rt, v);

    return s->uchars;
}

/**
 * Get the unicode code point at the position in the string.
 * \param rt The current runtime.
 * \param v The string value.
 * \param idx The character's index.
 * \return The unicode code point.
 */
extern int
rjs_string_get_uc (RJS_Runtime *rt, RJS_Value *v, size_t idx);

/**
 * Create a string from characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param chars The characters buffer.
 * \param len Characters in the buffer.
 * -1 means the characters are 0 terminated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_from_chars (RJS_Runtime *rt, RJS_Value *v,
        const char *chars, size_t len);

/**
 * Create a string from encoded characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param chars The characters buffer.
 * \param len Characters in the buffer.
 * -1 means the characters are 0 terminated.
 * \param enc The encoding of the character.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_from_enc_chars (RJS_Runtime *rt, RJS_Value *v,
        const char *chars, size_t len, const char *enc);

/**
 * Create a string from unicode characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param uchars The unicode characters buffer.
 * \param len Unicode characters in the buffer.
 * -1 means the unicode characters are 0 terminated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_from_uchars (RJS_Runtime *rt, RJS_Value *v,
        const RJS_UChar *uchars, size_t len);

/**
 * Create a string from static unicode characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param uchars The unicode characters buffer.
 * \param len Unicode characters in the buffer.
 * -1 means the unicode characters are 0 terminated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_from_static_uchars (RJS_Runtime *rt, RJS_Value *v,
        const RJS_UChar *uchars, size_t len);

/**
 * Convert the string to encoded characters.
 * \param rt The current runtime.
 * \param v The value of the string.
 * \param cb The output character buffer.
 * \param enc The encoding.
 * \return The 0 terminated characters.
 */
extern const char*
rjs_string_to_enc_chars (RJS_Runtime *rt, RJS_Value *v,
        RJS_CharBuffer *cb, const char *enc);

/**
 * Convert a string to property key.
 * The property key is unique if the string values are same.
 * \param rt The rt.
 * \param v The string value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_to_property_key (RJS_Runtime *rt, RJS_Value *v);

/**
 * Convert a string to number.
 * \param rt The current runtime.
 * \param v The string value.
 * \return The number.
 */
extern RJS_Number
rjs_string_to_number (RJS_Runtime *rt, RJS_Value *v);

/**
 * Check if 2 strings are equal.
 * \param rt The current runtime.
 * \param s1 String value 1.
 * \param s2 String value 2.
 * \retval RJS_TRUE s1 == s1.
 * \retval RJS_FALSE s1 != s2.
 */
extern RJS_Bool
rjs_string_equal (RJS_Runtime *rt, RJS_Value *s1, RJS_Value *s2);

/**
 * Get the substring.
 * \param rt The current runtime.
 * \param orig The origin string.
 * \param start The substring's start position.
 * \param end The substring's last character's position + 1.
 * \param[out] sub Return the substring.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_substr (RJS_Runtime *rt, RJS_Value *orig, size_t start, size_t end, RJS_Value *sub);

/**
 * Concatenate 2 strings.
 * \param rt The current runtime.
 * \param s1 String 1.
 * \param s2 String 2.
 * \param[out] sr Return the new string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_concat (RJS_Runtime *rt, RJS_Value *s1, RJS_Value *s2, RJS_Value *sr);

/**
 * Compare 2 strings.
 * \param rt The current runtime.
 * \param v1 String value 1.
 * \param v2 String value 2.
 * \retval RJS_COMPARE_EQUAL v1 == v2.
 * \retval RJS_COMPARE_LESS v1 < v2.
 * \retval RJS_COMPARE_GREATER v1 > v2.
 * \retval RJS_ERR On error.
 */
extern RJS_CompareResult
rjs_string_compare (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2);

/**
 * Trim the space characters of the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param flags The trim flags.
 * \param[out] rstr Return the result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_trim (RJS_Runtime *rt, RJS_Value *str, int flags, RJS_Value *rstr);

/**
 * Pad the substring at the beginning or end of the string.
 * \param rt The current runtime.
 * \param o The string object.
 * \param max_len The maximum length of the result.
 * \param fill_str The pattern string.
 * \param pos The pad position.
 * \param[out] rs The result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_string_pad (RJS_Runtime *rt, RJS_Value *o, RJS_Value *max_len, RJS_Value *fill_str,
        RJS_StringPadPosition pos, RJS_Value *rs);

/**
 * Search the unicode character in the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param uc The unicode character to be searched.
 * \param pos Searching start position.
 * \return The character's index in the string.
 * \retval -1 Cannot find the character.
 */
extern ssize_t
rjs_string_index_of_uchar (RJS_Runtime *rt, RJS_Value *str, RJS_UChar uc, size_t pos);

/**
 * Search the substring in the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param sub The substring to be searched.
 * \param pos Searching start position.
 * \return The substring's first character's index.
 * \retval -1 Cannot find the substring.
 */
extern ssize_t
rjs_string_index_of (RJS_Runtime *rt, RJS_Value *str, RJS_Value *sub, size_t pos);

/**
 * Search the last substring in the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param sub The substring to be searched.
 * \param pos Searching start position.
 * \return The substring's first character's index.
 * \retval -1 Cannot find the substring.
 */
extern ssize_t
rjs_string_last_index_of (RJS_Runtime *rt, RJS_Value *str, RJS_Value *sub, size_t pos);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

