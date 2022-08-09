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
 * Big integer internal header.
 */

#ifndef _RJS_BIG_INT_INTERNAL_H_
#define _RJS_BIG_INT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Big integer.*/
struct RJS_BigInt_s {
    RJS_GcThing  gc_thing; /**< Base GC thing data.*/
    __mpz_struct mpz;      /**< Big integer data.*/
};

/**
 * Convert the number to big integer.
 * \param rt The current runtime.
 * \param n The number.
 * \param[out] v Return the big number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On errpr.
 */
extern RJS_Result
rjs_number_to_big_int (RJS_Runtime *rt, RJS_Number n, RJS_Value *v);

/**
 * Create a new big integer.
 * \param rt The current runtime.
 * \param[out] v Return the big integer value.
 * \return The pointer to the big integer.
 */
extern RJS_BigInt*
rjs_big_int_new (RJS_Runtime *rt, RJS_Value *v);

/**
 * Create the big integer from the NULL terminated characters.
 * \param rt The current runtime.
 * \param[out] v Return the new big integer.
 * \param chars NULL terminated characters.
 * \param base Base of the number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On errpr.
 */
extern RJS_Result
rjs_big_int_from_chars (RJS_Runtime *rt, RJS_Value *v, const char *chars, int base);

/**
 * Create a big integer from an integer.
 * \param rt The current runtime.
 * \param[out] bi Return the big integer.
 * \param i The integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_from_int (RJS_Runtime *rt, RJS_Value *bi, int i);

/**
 * Create a big integer from 64 bits signed integer number.
 * \param rt The current runtime.
 * \param[out] bi Return the big integer.
 * \param i The integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_from_int64 (RJS_Runtime *rt, RJS_Value *bi, int64_t i);

/**
 * Create a big integer from 64 bits unsigned integer number.
 * \param rt The current runtime.
 * \param[out] bi Return the big integer.
 * \param i The integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_from_uint64 (RJS_Runtime *rt, RJS_Value *bi, uint64_t i);

/**
 * Convert the big integer value to string.
 * \param rt The current runtime.
 * \param v The big integer value.
 * \param radix The radix.
 * \param[out] s Return the string value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_to_string (RJS_Runtime *rt, RJS_Value *v, int radix, RJS_Value *s);

/**
 * Big integer unary minus operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_unary_minus (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Big integer bitwise not operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_bitwise_not (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Big integer increase.
 * \param rt The current runtime.
 * \param v Input number.
 * \param[out] rv Return value.
 */
extern void
rjs_big_int_inc (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Big integer increase.
 * \param rt The current runtime.
 * \param v Input number.
 * \param[out] rv Return value.
 */
extern void
rjs_big_int_dec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Big integer add operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_add (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer subtract operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_subtract (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer multiply operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_multiply (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer divide operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_divide (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer remainder operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_remainder (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer exponentiate operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_exponentiate (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer left shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_left_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer signed right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_signed_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer unsigned right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_unsigned_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer bitwise and operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_bitwise_and (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer bitwise xor operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_bitwise_xor (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Big integer bitwise or operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_bitwise_or (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv);

/**
 * Compare 2 big integers.
 * \param rt The current runtime.
 * \param v1 Big integer 1.
 * \param v2 Big integer 2.
 * \retval RJS_COMPARE_LESS v1 < v2.
 * \retval RJS_COMPARE_GREATER v1 > v2.
 * \retval RJS_COMPARE_EQUAL v1 == v2.
 */
extern RJS_Result
rjs_big_int_compare (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2);

/**
 * Compare a big integer to a number.
 * \param rt The current runtime.
 * \param v Big integer.
 * \param n The number.
 * \retval RJS_COMPARE_LESS v < n.
 * \retval RJS_COMPARE_GREATER v > n.
 * \retval RJS_COMPARE_EQUAL v == n.
 * \retval RJS_COMPARE_UNDEFINED n is NAN.
 */
extern RJS_Result
rjs_big_int_compare_number (RJS_Runtime *rt, RJS_Value *v, RJS_Number n);

/**
 * Create the signed big interger use the last bits of another big integer.
 * \param rt The current runtime.
 * \param bits Bits number.
 * \param v The input big integer.
 * \param[out] rv The result big integer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_as_int_n (RJS_Runtime *rt, int64_t bits, RJS_Value *v, RJS_Value *rv);

/**
 * Create the unsigned big interger use the last bits of another big integer.
 * \param rt The current runtime.
 * \param bits Bits number.
 * \param v The input big integer.
 * \param[out] rv The result big integer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_as_uint_n (RJS_Runtime *rt, int64_t bits, RJS_Value *v, RJS_Value *rv);

#ifdef __cplusplus
}
#endif

#endif

