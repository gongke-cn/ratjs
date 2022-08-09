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
 * Unicode character internal header.
 */

#ifndef _RJS_UCHAR_INTERNAL_H_
#define _RJS_UCHAR_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check if the character is an octal number.
 * \param c The character.
 * \retval RJS_TRUE c is an octal number.
 * \retval RJS_FALSE c is not an octal number.
 */
static inline RJS_Bool
rjs_uchar_is_octal (int c)
{
    return (c >= '0') && (c <= '7');
}

/**
 * Check if the character is a digit number.
 * \param c The character.
 * \retval RJS_TRUE c is a digit number.
 * \retval RJS_FALSE c is not a digit number.
 */
static inline RJS_Bool
rjs_uchar_is_digit (int c)
{
    return (c >= '0') && (c <= '9');
}

/**
 * Check if the character is a hexadecimal digit number.
 * \param c The character.
 * \retval RJS_TRUE c is a hexadecimal digit number.
 * \retval RJS_FALSE c is not a hexadecimal digit number.
 */
static inline RJS_Bool
rjs_uchar_is_xdigit (int c)
{
    return ((c >= '0') && (c <= '9'))
            || ((c >= 'a') && (c <= 'f'))
            || ((c >= 'A') && (c <= 'F'));
}

/**
 * Check if the character is an alphabetic character.
 * \param c The character.
 * \retval RJS_TRUE c is an alphabetic character.
 * \retval RJS_FALSE c is not an alphabetic character.
 */
static inline RJS_Bool
rjs_uchar_is_alpha (int c)
{
    return ((c >= 'a') && (c <= 'z'))
            || ((c >= 'A') && (c <= 'Z'));
}

/**
 * Check if the character is an alphabetic character or number.
 * \param c The character.
 * \retval RJS_TRUE c is an alphabetic character or number.
 * \retval RJS_FALSE c is not an alphabetic character or number.
 */
static inline RJS_Bool
rjs_uchar_is_alnum (int c)
{
    return rjs_uchar_is_alpha(c) || rjs_uchar_is_digit(c);
}

/**
 * Convert the hexadecimal character to number.
 * \param c The hexadecimal character.
 * \return The number value of the character.
 */
static inline int
rjs_hex_char_to_number (int c)
{
    if ((c >= 'a') && (c <= 'f'))
        return c - 'a' + 10;
    if ((c >= 'A') && (c <= 'F'))
        return c - 'A' + 10;

    assert((c >= '0') && (c <= '9'));

    return c - '0';
}

/**
 * Convert a number to lower case hexadecimal character.
 * \param n The number.
 * \return The hexadecimal character.
 */
static inline int
rjs_number_to_hex_char_l (int n)
{
    assert((n >= 0) && (n <= 15));

    if (n <= 9)
        return n + '0';

    return n - 10 + 'a';
}

/**
 * Convert a number to upper case hexadecimal character.
 * \param n The number.
 * \return The hexadecimal character.
 */
static inline int
rjs_number_to_hex_char_u (int n)
{
    assert((n >= 0) && (n <= 15));

    if (n <= 9)
        return n + '0';

    return n - 10 + 'A';
}

/**
 * Check if the unicode character is a line terminator.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is line terminator.
 * \retval RJS_FALSE The character is not line terminator.
 */
static inline RJS_Bool
rjs_uchar_is_line_terminator (int c)
{
    return (c == 0x0a) || (c == 0x0d) || (c == 0x2028) || (c == 0x2029);
}

#if ENC_CONV == ENC_CONV_ICU

/**
 * Check if the unicode character is a white space.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is a white space.
 * \retval RJS_FALSE The character is not a white space.
 */
extern RJS_Bool
rjs_uchar_is_white_space (int c);

/**
 * Check if the unicode character is a identifier start character.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is a identifier start character.
 * \retval RJS_FALSE The character is not a identifier start character.
 */
extern RJS_Bool
rjs_uchar_is_id_start (int c);

/**
 * Check if the unicode character is a identifier continue character.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is a identifier continue character.
 * \retval RJS_FALSE The character is not a identifier continue character.
 */
extern RJS_Bool
rjs_uchar_is_id_continue (int c);

/**
 * Normalize the unicoide character string.
 * \param s the source string.
 * \param slen Length of the source string.
 * \param d The output characters buffer.
 * \param dlen Length of the output buffer.
 * \param mode The normalize mode, "NFC/NFD/NFKC/NFKD".
 * \return The length of the output string.
 */
extern int
rjs_uchars_normalize (const RJS_UChar *s, int slen, RJS_UChar *d, int dlen, const char *mode);

/**
 * Map the character to its case folding equivalent.
 * \param c The input character.
 * \return The mapped character,
 */
extern int
rjs_uchar_fold_case (int c);

#else /*ENC_CONV != ENC_CONV_ICU*/

static inline RJS_Bool
rjs_uchar_is_white_space (int c)
{
    return (c == '\n')
            || (c == '\r')
            || (c == '\t')
            || (c == '\v')
            || (c == '\f')
            || (c == ' ')
            || (c == 0xfeff);
}

static inline RJS_Bool
rjs_uchar_is_id_start (int c)
{
    return rjs_uchar_is_alpha(c) || (c == '_') || (c == '$');
}

static inline RJS_Bool
rjs_uchar_is_id_continue (int c)
{
    return rjs_uchar_is_alnum(c) || (c == '_') || (c == '$') || (c == 0x200c) || (c == 0x200d);
}

/**
 * Normalize the unicoide character string.
 * \param s the source string.
 * \param slen Length of the source string.
 * \param d The output characters buffer.
 * \param dlen Length of the output buffer.
 * \param mode The normalize mode, "NFC/NFD/NFKC/NFKD".
 * \return The length of the output string.
 */
static inline int
rjs_uchars_normalize (const RJS_UChar *s, int slen, RJS_UChar *d, int dlen, const char *mode)
{
    if (dlen >= slen) {
        RJS_ELEM_CPY(d, s, slen);
    }

    return slen;
}

/**
 * Map the character to its case folding equivalent.
 * \param c The input character.
 * \return The mapped character,
 */
static inline int
rjs_uchar_fold_case (int c)
{
    return c;
}

#endif /*ENC_CONV == ENC_CONV_ICU*/

/**
 * Compare 2 unicode characters strings.
 * \param s1 String 1.
 * \param l1 Length of string 1.
 * \param s2 String 2.
 * \param l2 Length of string 2.
 * \retval 0 s1 == s2.
 * \retval <0 s1 < s2.
 * \retval >0 s1 > s2.
 */
extern int
rjs_uchars_compare (const RJS_UChar *s1, int l1, const RJS_UChar *s2, int l2);

/**
 * Convert the characters to uppercase if a is a lowecase character.
 * \param s Input characters.
 * \param slen Input characters' length.
 * \param[out] dst The upper character output buffer.
 * \param dlen Length of the output buffer.
 * \param locale The locale.
 * \return Count of the upper characters.
 */
extern int
rjs_uchars_to_upper (const RJS_UChar *s, int slen, RJS_UChar *dst, int dlen, const char *locale);

/**
 * Convert the characters to lowercase if a is an uppercase character.
 * \param s Input characters.
 * \param slen Input characters' length.
 * \param[out] dst The lower character output buffer.
 * \param dlen Length of the output buffer.
 * \param locale The locale.
 * \return Count of the lower characters.
 */
extern int
rjs_uchars_to_lower (const RJS_UChar *s, int slen, RJS_UChar *dst, int dlen, const char *locale);

/**
 * Check if the character is a leading surrogate.
 * \param c The unicode character.
 * \retval RJS_TRUE c is a leading surrogate.
 * \retval RJS_FALSE c is not a leading surrogate.
 */
static inline RJS_Bool
rjs_uchar_is_leading_surrogate (int c)
{
    return (c >= 0xd800) && (c <= 0xdbff);
}

/**
 * Check if the character is a trailing surrogate.
 * \param c The unicode character.
 * \retval RJS_TRUE c is a trailing surrogate.
 * \retval RJS_FALSE c is not a trailing surrogate.
 */
static inline RJS_Bool
rjs_uchar_is_trailing_surrogate (int c)
{
    return (c >= 0xdc00) && (c <= 0xdfff);
}

/**
 * Convert the surrogate pair to unicode.
 * \param l The leading surrogate.
 * \param t The trailing surrograte.
 * \return The unicode value.
 */
static inline uint32_t
rjs_surrogate_pair_to_uc (int l, int t)
{
    return (((l - 0xd800) << 10) | (t - 0xdc00)) + 0x10000;
}

/**
 * Convert the unicode to surrogate pair.
 * \param c The unicode.
 * \param[out] l Return the leading surrogate.
 * \param[out] t Return the tailing surrogate.
 */
static inline void
rjs_uc_to_surrogate_pair (uint32_t c, RJS_UChar *l, RJS_UChar *t)
{
    int v = c - 0x10000;

    *l = (v >> 10) + 0xd800;
    *t = (v & 0x3ff) + 0xdc00;
}

#ifdef __cplusplus
}
#endif

#endif

