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
 * Number bitwise not operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 */
void
rjs_number_bitwise_not (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    int32_t i = 0;

    rjs_to_int32(rt, v, &i);

    rjs_value_set_number(rt, rv, ~i);
}

/**
 * Number left shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_left_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    int32_t  n1 = 0;
    uint32_t n2 = 0;
    int32_t  rn;

    rjs_to_int32(rt, v1, &n1);
    rjs_to_uint32(rt, v2, &n2);

    rn = n1 << (n2 & 31);

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number signed right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_signed_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    int32_t  n1 = 0;
    uint32_t n2 = 0;
    int32_t  rn;

    rjs_to_int32(rt, v1, &n1);
    rjs_to_uint32(rt, v2, &n2);

    rn = n1 >> (n2 & 31);

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number unsigned right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_unsigned_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    uint32_t n1 = rjs_value_get_number(rt, v1);
    uint32_t n2 = rjs_value_get_number(rt, v2);
    uint32_t rn = n1 >> n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number bitwise and operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_bitwise_and (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    int32_t n1 = 0, n2 = 0, rn;

    rjs_to_int32(rt, v1, &n1);
    rjs_to_int32(rt, v2, &n2);

    rn = n1 & n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number bitwise xor operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_bitwise_xor (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    int32_t n1 = 0, n2 = 0, rn;

    rjs_to_int32(rt, v1, &n1);
    rjs_to_int32(rt, v2, &n2);

    rn = n1 ^ n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number bitwise or operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_bitwise_or (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    int32_t n1 = 0, n2 = 0, rn;

    rjs_to_int32(rt, v1, &n1);
    rjs_to_int32(rt, v2, &n2);

    rn = n1 | n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number exponentiate operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
void
rjs_number_exponentiate (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);
    RJS_Number rn;
    RJS_Bool   is_odd;

    if (isnan(n2)) {
        rn = NAN;
    } else if (n2 == 0) {
        rn = 1;
    } else if (isnan(n1)) {
        rn = NAN;
    } else if (n1 == INFINITY) {
        rn = (n2 > 0) ? INFINITY : 0;
    } else if (n1 == -INFINITY) {
        is_odd = rjs_is_integral_number(n2) && (((int64_t)n2) & 1);

        if (n2 > 0)
            rn = is_odd ? -INFINITY : INFINITY;
        else
            rn = is_odd ? -0. : 0.;
    } else if (n1 == 0) {
        if (signbit(n1)) {
            is_odd = rjs_is_integral_number(n2) && (((int64_t)n2) & 1);

            if (n2 > 0)
                rn = is_odd ? -0. : 0.;
            else
                rn = is_odd ? -INFINITY : INFINITY;
        } else {
            rn = (n2 > 0) ? 0 : INFINITY;
        }
    } else if (n2 == INFINITY) {
        RJS_Number a = fabs(n1);

        if (a > 1)
            rn = INFINITY;
        else if (a == 1)
            rn = NAN;
        else
            rn = 0;
    } else if (n2 == -INFINITY) {
        RJS_Number a = fabs(n1);

        if (a > 1)
            rn = 0;
        else if (a == 1)
            rn = NAN;
        else
            rn = INFINITY;
    } else {
        rn = pow(n1, n2);
    }

    rjs_value_set_number(rt, rv, rn);
}

/*Check if the number is an array index.*/
static RJS_Bool
is_array_index (RJS_Number n)
{
    RJS_Number f;

    if (isnan(n) || isinf(n) || signbit(n))
        return RJS_FALSE;

    f = floor(n);
    if (f != n)
        return RJS_FALSE;

    return f < 0xffffffff;
}

/**
 * Convert the number value to string.
 * \param rt The current runtime.
 * \param n The number value.
 * \param[out] s Return the string value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_number_to_string (RJS_Runtime *rt, RJS_Number n, RJS_Value *s)
{
    RJS_Result r = RJS_OK;

    if (signbit(n) && (n == 0.)) {
        rjs_value_set_index_string(rt, s, 0);
    } else if (is_array_index(n)) {
        rjs_value_set_index_string(rt, s, n);
    } else if (isnan(n)) {
        rjs_value_copy(rt, s, rjs_s_NaN(rt));
    } else if (isinf(n)) {
        if (signbit(n))
            rjs_value_copy(rt, s, rjs_s_negative_Infinity(rt));
        else
            rjs_value_copy(rt, s, rjs_s_Infinity(rt));
    } else {
        char *sb, *se, *sc, *dc;
        int   sign, decpt, nv, kv, i;
        char  sbuf[64];

        sb = rjs_dtoa(n, 0, 0, &decpt, &sign, &se);
        sc = sb;
        dc = sbuf;

        if (sign)
            *dc ++ = '-';

        kv = se - sb;
        nv = decpt;

        if ((kv <= nv) && (nv <= 21)) {
            for (i = 0; i < kv; i ++)
                *dc ++ = *sc ++;
            for (; i < nv; i ++)
                *dc ++ = '0';
        } else if ((0 < nv) && (nv <= 21)) {
            for (i = 0; i < nv; i ++)
                *dc ++ = *sc ++;
            *dc ++ = '.';
            for (; i < kv; i ++)
                *dc ++ = *sc ++;
        } else if ((-6 < nv) && (nv <= 0)) {
            *dc ++ = '0';
            *dc ++ = '.';
            for (i = 0; i < -nv; i ++)
                *dc ++ = '0';
            for (i = 0; i < kv; i ++)
                *dc ++ = *sc ++;
        } else {
            int v, left;

            *dc ++ = *sc ++;
            if (kv != 1) {
                *dc ++ = '.';
                for (i = 0; i < kv - 1; i ++)
                    *dc ++ = *sc ++;
            }
            *dc ++ = 'e';
            if (nv - 1 < 0) {
                *dc ++ = '-';
                left   = 1 - nv;
            } else {
                *dc ++ = '+';
                left   = nv - 1;
            }

            for (v = 10; v <= left; v *= 10);
            for (v /= 10; v > 0; v /= 10) {
                int v1 = left / v;

                *dc ++ = v1 + '0';
                left %= v;
            }
        }

        *dc = 0;

        rjs_string_from_chars(rt, s, sbuf, -1);

        rjs_freedtoa(sb);
    }

    return r;
}
