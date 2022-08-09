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
 * Number internal header.
 */

#ifndef _RJS_NUMBER_INTERNAL_H_
#define _RJS_NUMBER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup number Number
 * Number
 * @{
 */

/**
 * Number unary minus operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_unary_minus (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Number n = rjs_value_get_number(rt, v);

    rjs_value_set_number(rt, rv, -n);
}

/**
 * Number bitwise not operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 */
extern void
rjs_number_bitwise_not (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Number increase.
 * \param rt The current runtime.
 * \param v Input number.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_inc (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Number n  = rjs_value_get_number(rt, v);
    RJS_Number rn = n + 1;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number decrease.
 * \param rt The current runtime.
 * \param v Input number.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_dec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Number n  = rjs_value_get_number(rt, v);
    RJS_Number rn = n - 1;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number add operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_add (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);
    RJS_Number rn = n1 + n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number subtract operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_subtract (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);
    RJS_Number rn = n1 - n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number multiply operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_multiply (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);
    RJS_Number rn = n1 * n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number divide operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_divide (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);
    RJS_Number rn = n1 / n2;

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number remainder operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
static inline void
rjs_number_remainder (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);
    RJS_Number rn = fmod(n1, n2);

    rjs_value_set_number(rt, rv, rn);
}

/**
 * Number exponentiate operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_exponentiate (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Number left shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_left_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Number signed right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_signed_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Number unsigned right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_unsigned_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Number bitwise and operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_bitwise_and (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Number bitwise xor operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_bitwise_xor (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Number bitwise or operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 */
extern void
rjs_number_bitwise_or (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Compare 2 numbers.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \retval RJS_COMPARE_LESS v1 < v2.
 * \retval RJS_COMPARE_GREATER v1 > v2.
 * \retval RJS_COMPARE_EQUAL v1 == v2.
 * \retval RJS_COMPARE_UNDEFINED v1 or v2 is NAN.
 */
static inline RJS_Result
rjs_number_compare (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);

    if (isnan(n1) || isnan(n2))
        return RJS_COMPARE_UNDEFINED;

    if (n1 == n2)
        return RJS_COMPARE_EQUAL;

    if (n1 < n2)
        return RJS_COMPARE_LESS;

    return RJS_COMPARE_GREATER;
}

/**
 * Convert the number value to string.
 * \param rt The current runtime.
 * \param n The number value.
 * \param[out] s Return the string value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_number_to_string (RJS_Runtime *rt, RJS_Number n, RJS_Value *s);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

