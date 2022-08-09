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
 * Convert the string to integer.
 * \param str The string.
 * \param[out] end Return the end pointer which character cannot be parsed as integer.
 * \param base Base of the number.
 * \return The double precision number.
 */
double
rjs_strtoi (const char *str, char **end, int base)
{
    const char *p    = str;
    int         sign = 1;
    double      d    = 0;

    assert((base == 0) || ((base >= 2) && (base <= 36)));

    while (*p) {
        if (!isspace(*p))
            break;

        p ++;
    }

    if (*p == '+') {
        sign = 1;
        p ++;
    } else if (*p == '-') {
        sign = -1;
        p ++;
    }

    if ((base == 0) || (base == 16)) {
        if ((p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X'))) {
            p   += 2;
            base = 16;
        }
    }

    if (base == 0)
        base = 10;

    if (base == 10)
        return rjs_strtod(str, end);

    while (*p) {
        int v;

        if (base <= 10) {
            if ((*p < '0') || (*p > '0' + base - 1))
                break;

            v = *p - '0';
        } else {
            if (isdigit(*p)) {
                v = *p - '0';
            } else {
                int lower = 'a' + base - 11;
                int upper = 'A' + base - 11;

                if ((*p >= 'a') && (*p <= lower))
                    v = *p - 'a' + 10;
                else if ((*p >= 'A' && (*p <= upper)))
                    v = *p - 'A' + 10;
                else
                    break;
            }
        }

        d *= base;
        d += v;
        p ++;
    }

    if (end)
        *end = (char*)p;

    if (sign == -1)
        d = -d;

    return d;
}
