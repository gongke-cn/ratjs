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
    if (dlen >= slen) {
        const RJS_UChar *sc, *se;
        RJS_UChar       *dc;

        sc = s;
        se = sc + slen;
        dc = dst;

        while (sc < se) {
            if ((*sc >= 'a') && (*sc <= 'z'))
                *dc = *sc - 'a' + 'A';
            else
                *dc = *sc;

            sc ++;
            dc ++;
        }
    }

    return slen;
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
    if (dlen >= slen) {
        const RJS_UChar *sc, *se;
        RJS_UChar       *dc;

        sc = s;
        se = sc + slen;
        dc = dst;

        while (sc < se) {
            if ((*sc >= 'A') && (*sc <= 'Z'))
                *dc = *sc - 'A' + 'a';
            else
                *dc = *sc;
                
            sc ++;
            dc ++;
        }
    }

    return slen;
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
    while ((l1 > 0) && (l2 > 0)) {
        int r = *s1 - *s2;

        if (r != 0)
            return r;

        s1 ++;
        s2 ++;
        l1 --;
        l2 --;
    }

    return l1 - l2;
}
