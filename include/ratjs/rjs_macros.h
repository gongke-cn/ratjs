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
 * Basic macro definitions.
 */

#ifndef _RJS_MACROS_H_
#define _RJS_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup misc Misc
 * Misc definitions and functions
 * @{
 */

/**Statement like macro begin.*/
#define RJS_STMT_BEGIN do {
/**Statement like macro end.*/
#define RJS_STMT_END   } while (0)

/**Get the minimum value.*/
#define RJS_MIN(a, b) (((a) < (b)) ? (a) : (b))
/**Get the maximum value.*/
#define RJS_MAX(a, b) (((a) > (b)) ? (a) : (b))
/**Get the absolute value.*/
#define RJS_ABS(a)    (((a) >= 0) ? (a) : -(a))
/**Get the minimum value of 3 numbers.*/
#define RJS_MIN_3(a, b, c)      RJS_MIN(a, RJS_MIN(b, c))
/**Get the maximum value of 3 numbers.*/
#define RJS_MAX_3(a, b, c)      RJS_MAX(a, RJS_MAX(b, c))
/**Clamp value in the range.*/
#define RJS_CLAMP(n, min, max)  RJS_MIN(RJS_MAX(n, min), max)

/**Cast pointer to size_t.*/
#define RJS_PTR2SIZE(p)  ((size_t)(p))
/**Cast size_t to pointer.*/
#define RJS_SIZE2PTR(s)  ((void*)(size_t)(s))

/**Get the elements in the array.*/
#define RJS_N_ELEM(a) (sizeof(a)/sizeof((a)[0]))

/**Get the offset of a member.*/
#define RJS_OFFSET_OF(s, m) RJS_PTR2SIZE(&((s*)0)->m)
/**Get the container pointer from the member pointer.*/
#define RJS_CONTAINER_OF(p, s, m)\
    ((s*)(RJS_PTR2SIZE(p) - RJS_OFFSET_OF(s, m)))

/**Set the elements in an array.*/
#define RJS_ELEM_SET(p, v, n)\
    RJS_STMT_BEGIN\
        size_t i;\
        for (i = 0; i < (n); i ++)\
            p[i] = v;\
    RJS_STMT_END

/**Copy array elements.*/
#define RJS_ELEM_CPY(d, s, n)\
    memcpy(d, s, sizeof(*(s)) * (n))

/**Move array elements.*/
#define RJS_ELEM_MOVE(d, s, n)\
    memmove(d, s, sizeof(*(s)) * (n))

/**Compare arrat elements.*/
#define RJS_ELEM_CMP(d, s, n)\
    memcmp(d, s, sizeof(*(s)) * (n))

/**Align down.*/
#define RJS_ALIGN_DOWN(p, s)\
    (RJS_PTR2SIZE(p) & ~((s) - 1))

/**Align up.*/
#define RJS_ALIGN_UP(p, s)\
    ((RJS_PTR2SIZE(p) + (s) - 1) & ~((s) - 1))

/**Get localized string.*/
#define _(s) (s)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

