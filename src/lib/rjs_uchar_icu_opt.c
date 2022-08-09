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
 * Check if the unicode character is a white space.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is a white space.
 * \retval RJS_FALSE The character is not a white space.
 */
RJS_Bool
rjs_uchar_is_white_space (int c)
{
    return u_isUWhiteSpace(c) || (c == 0xfeff);
}

/**
 * Check if the unicode character is a identifier start character.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is a identifier start character.
 * \retval RJS_FALSE The character is not a identifier start character.
 */
RJS_Bool
rjs_uchar_is_id_start (int c)
{
    if (c == -1)
        return RJS_FALSE;

    return (c == '$') || (c == '_')
            || u_hasBinaryProperty(c, UCHAR_ID_START);
}

/**
 * Check if the unicode character is a identifier continue character.
 * \param c The unicode character.
 * \retval RJS_TRUE The character is a identifier continue character.
 * \retval RJS_FALSE The character is not a identifier continue character.
 */
RJS_Bool
rjs_uchar_is_id_continue (int c)
{
    if (c == -1)
        return RJS_FALSE;

    return (c == '$') || (c == 0x200c) || (c == 0x200d)
            || u_hasBinaryProperty(c, UCHAR_ID_START)
            || u_hasBinaryProperty(c, UCHAR_ID_CONTINUE);
}

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
int
rjs_uchars_compare (const RJS_UChar *s1, int l1, const RJS_UChar *s2, int l2)
{
    UErrorCode err = U_ZERO_ERROR;

    return unorm_compare(s1, l1, s2, l2, U_FOLD_CASE_DEFAULT, &err);
}

/**
 * Map the character to its case folding equivalent.
 * \param c The input character.
 * \return The mapped character,
 */
int
rjs_uchar_fold_case (int c)
{
    return u_foldCase(c, U_FOLD_CASE_DEFAULT);
}

/**
 * Convert the characters to uppercase if a is a lowecase character.
 * \param s Input characters.
 * \param slen Input characters' length.
 * \param[out] dst The upper character output buffer.
 * \param dlen Length of the output buffer.
 * \param locale The locale.
 * \return Count of the upper characters.
 */
int
rjs_uchars_to_upper (const RJS_UChar *s, int slen, RJS_UChar *dst, int dlen, const char *locale)
{
    UErrorCode err = U_ZERO_ERROR;

    return u_strToUpper(dst, dlen, s, slen, NULL, &err);
}

/**
 * Convert the characters to lowercase if a is an uppercase character.
 * \param s Input characters.
 * \param slen Input characters' length.
 * \param[out] dst The lower character output buffer.
 * \param dlen Length of the output buffer.
 * \param locale The locale.
 * \return Count of the lower characters.
 */
int
rjs_uchars_to_lower (const RJS_UChar *s, int slen, RJS_UChar *dst, int dlen, const char *locale)
{
    UErrorCode err = U_ZERO_ERROR;

    return u_strToLower(dst, dlen, s, slen, NULL, &err);
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
int
rjs_uchars_normalize (const RJS_UChar *s, int slen, RJS_UChar *d, int dlen, const char *mode)
{
    const UNormalizer2 *inst;
    UErrorCode          err = U_ZERO_ERROR;

    if (!strcmp(mode, "NFC")) {
        inst = unorm2_getNFCInstance(&err);
    } else if (!strcmp(mode, "NFD")) {
        inst = unorm2_getNFDInstance(&err);
    } else if (!strcmp(mode, "NFKC")) {
        inst = unorm2_getNFKCInstance(&err);
    } else if (!strcmp(mode, "NFKD")) {
        inst = unorm2_getNFKDInstance(&err);
    } else {
        return -1;
    }

    return unorm2_normalize(inst, s, slen, d, dlen, &err);
}
