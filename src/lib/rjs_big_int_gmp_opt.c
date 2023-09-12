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

#ifdef __GMP_SHORT_LIMB
    #define SIZEOF_LIMB __SIZEOF_SHORT__
#else
#ifdef _LONG_LONG_LIMB
    #define SIZEOF_LIMB __SIZEOF_LONG_LONG__
#else
    #define SIZEOF_LIMB __SIZEOF_LONG__
#endif
#endif

/*Scan the reference things in the big integer.*/
static void
big_int_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
}

/*Free the big integer.*/
static void
big_int_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_BigInt *bi = ptr;

    mpz_clear(&bi->mpz);

    RJS_DEL(rt, bi);
}

/*Big integer operation functions.*/
static const RJS_GcThingOps
big_int_ops = {
    RJS_GC_THING_BIG_INT,
    big_int_op_gc_scan,
    big_int_op_gc_free
};

/*Allocate a new big integer.*/
static RJS_BigInt*
big_int_new (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_BigInt *bi;

    RJS_NEW(rt, bi);

    rjs_value_set_big_int(rt, v, bi);
    rjs_gc_add(rt, bi, &big_int_ops);

    return bi;
}

/*Allocate a new big integer and initialize it.*/
static RJS_BigInt*
big_int_new_init (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_BigInt *bi;

    bi = big_int_new(rt, v);

    mpz_init(&bi->mpz);

    return bi;
}

/**
 * Convert the number to big integer.
 * \param rt The current runtime.
 * \param n The number.
 * \param[out] v Return the big number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On errpr.
 */
RJS_Result
rjs_number_to_big_int (RJS_Runtime *rt, RJS_Number n, RJS_Value *v)
{
    RJS_BigInt *bi;

    if (!rjs_is_integral_number(n)) {
        return rjs_throw_range_error(rt, _("the value is not an integer"));
    }

    bi = big_int_new(rt, v);

    mpz_init_set_d(&bi->mpz, n);

    return RJS_OK;
}

/**
 * Create the big integer from the NULL terminated characters.
 * \param rt The current runtime.
 * \param[out] v Return the new big integer.
 * \param chars NULL terminated characters.
 * \param base Base of the number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On errpr.
 */
RJS_Result
rjs_big_int_from_chars (RJS_Runtime *rt, RJS_Value *v, const char *chars, int base)
{
    RJS_BigInt *bi;
    int         r;

    bi = big_int_new(rt, v);

    r = mpz_init_set_str(&bi->mpz, chars, base);
    if (r == -1) {
        mpz_init(&bi->mpz);
        return RJS_ERR;
    }

    return RJS_OK;
}

/**
 * Convert the value to big intwger.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] bi The big integer.
 */
RJS_Result
rjs_to_big_int (RJS_Runtime *rt, RJS_Value *v, RJS_Value *bi)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *prim = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_primitive(rt, v, prim, RJS_VALUE_NUMBER)) == RJS_ERR)
        goto end;

    switch (rjs_value_get_type(rt, prim)) {
    case RJS_VALUE_NULL:
    case RJS_VALUE_UNDEFINED:
    case RJS_VALUE_NUMBER:
    case RJS_VALUE_SYMBOL:
        r = rjs_throw_type_error(rt, _("the value cannot be converted to big integer"));
        goto end;
    case RJS_VALUE_BOOLEAN:
        if (rjs_value_get_boolean(rt, prim))
            rjs_big_int_from_int(rt, bi, 1);
        else
            rjs_big_int_from_int(rt, bi, 0);
        break;
    case RJS_VALUE_BIG_INT:
        rjs_value_copy(rt, bi, prim);
        break;
    case RJS_VALUE_STRING:
        rjs_string_to_big_int(rt, prim, bi);
        if (rjs_value_is_undefined(rt, bi)) {
            r = rjs_throw_syntax_error(rt, _("the string cannot be converted to big integer"));
            goto end;
        }
        break;
    default:
        assert(0);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a big integer from an integer.
 * \param rt The current runtime.
 * \param[out] v Return the big integer.
 * \param i The integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_from_int (RJS_Runtime *rt, RJS_Value *v, int i)
{
    RJS_BigInt *bi;

    bi = big_int_new(rt, v);

    mpz_init_set_si(&bi->mpz, i);

    return RJS_OK;
}

/**
 * Create a big integer from 64 bits signed integer number.
 * \param rt The current runtime.
 * \param[out] v Return the big integer.
 * \param i The integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_from_int64 (RJS_Runtime *rt, RJS_Value *v, int64_t i)
{
    RJS_BigInt *bi;

    bi = big_int_new(rt, v);

#if SIZEOF_LIMB == 8
    {
        mp_limb_t *limb;
        uint64_t   u;

        mpz_init(&bi->mpz);

        u = RJS_ABS(i);

        limb = mpz_limbs_modify(&bi->mpz, 1);
        limb[0] = u;

        mpz_limbs_finish(&bi->mpz, (i < 0) ? -1 : 1);
    }
#else
    {
        mp_limb_t *limb;
        uint64_t   u;

        mpz_init(&bi->mpz);

        u = RJS_ABS(i);

        limb = mpz_limbs_modify(&bi->mpz, 2);
        limb[0] = u & 0xffffffff;
        limb[1] = u >> 32;

        mpz_limbs_finish(&bi->mpz, (i < 0) ? -2 : 2);
    }
#endif /*SIZEOF_LIMB == 8*/

    return RJS_OK;
}

/**
 * Create a big integer from 64 bits unsigned integer number.
 * \param rt The current runtime.
 * \param[out] v Return the big integer.
 * \param i The integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_from_uint64 (RJS_Runtime *rt, RJS_Value *v, uint64_t i)
{
    RJS_BigInt *bi;

    bi = big_int_new(rt, v);

#if SIZEOF_LIMB == 8
    {
        mp_limb_t *limb;
        
        mpz_init(&bi->mpz);

        limb = mpz_limbs_modify(&bi->mpz, 1);
        limb[0] = i;

        mpz_limbs_finish(&bi->mpz, 1);
    }
#else
    {
        mp_limb_t *limb;
        
        mpz_init(&bi->mpz);

        limb = mpz_limbs_modify(&bi->mpz, 2);
        limb[0] = i & 0xffffffff;
        limb[1] = i >> 32;

        mpz_limbs_finish(&bi->mpz, 2);
    }
#endif /*SIZEOF_LIMB == 8*/

    return RJS_OK;
}

/**
 * Convert the big integer to 64 bits signed integer number.
 * \param rt The current runtime.
 * \param v The big integer.
 * \param[out] pi Return the 64 bits signed integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_to_int64 (RJS_Runtime *rt, RJS_Value *v, int64_t *pi)
{
    RJS_BigInt *bi;

    bi = rjs_value_get_big_int(rt, v);

#if SIZEOF_LIMB == 8
    {
        size_t  size = mpz_size(&bi->mpz);
        int64_t i;

        if (size == 0) {
            i = 0;
        } else {
            const mp_limb_t *limb;

            limb = mpz_limbs_read(&bi->mpz);

            i = limb[0];

            if (mpz_sgn(&bi->mpz) < 0)
                i = -i;
        }

        *pi = i;
    }
#else
    {
        size_t  size = mpz_size(&bi->mpz);
        int64_t i;

        if (size == 0) {
            i = 0;
        } else if (size == 1) {
            i = mpz_get_ui(&bi->mpz);
        } else {
            const mp_limb_t *limb;

            limb = mpz_limbs_read(&bi->mpz);

            i = ((int64_t)limb[0]) + (((int64_t)limb[1]) << 32);
        }

        if (mpz_sgn(&bi->mpz) < 0) {
            i = -i;
        }

        *pi = i;
    }
#endif /*SIZEOF_LIMB == 8*/

    return RJS_OK;
}

/**
 * Convert the big integer to 64 bits unsigned integer number.
 * \param rt The current runtime.
 * \param v The big integer.
 * \param[out] pi Return the 64 bits unsigned integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_to_uint64 (RJS_Runtime *rt, RJS_Value *v, uint64_t *pi)
{
    RJS_BigInt *bi;

    bi = rjs_value_get_big_int(rt, v);

#if SIZEOF_LIMB == 8
    {
        size_t  size = mpz_size(&bi->mpz);
        int64_t i;

        if (size == 0) {
            i = 0;
        } else {
            const mp_limb_t *limb;

            limb = mpz_limbs_read(&bi->mpz);

            i = limb[0];

            if (mpz_sgn(&bi->mpz) < 0)
                i = -i;
        }

        *pi = i;
    }
#else
    {
        size_t  size = mpz_size(&bi->mpz);
        int64_t i;

        if (size == 0) {
            i = 0;
        } else if (size == 1) {
            i = mpz_get_ui(&bi->mpz);
        } else {
            const mp_limb_t *limb;

            limb = mpz_limbs_read(&bi->mpz);

            i = ((int64_t)limb[0]) + (((int64_t)limb[1]) << 32);
        }

        if (mpz_sgn(&bi->mpz) < 0) {
            i = -i;
        }

        *pi = i;
    }
#endif /*SIZEOF_LIMB == 8*/

    return RJS_OK;
}

/**
 * Check if 2 big integer values are equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2.
 */
RJS_Bool
rjs_big_int_same_value (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_BigInt *bi1, *bi2;
    int         r;

    bi1 = rjs_value_get_big_int(rt, v1);
    bi2 = rjs_value_get_big_int(rt, v2);

    r = mpz_cmp(&bi1->mpz, &bi2->mpz);

    return (r == 0);
}

/**
 * Check if the big integer is zero.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE v is 0.
 * \retval RJS_FALSE v is not 0.
 */
RJS_Bool
rjs_big_int_is_0 (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_BigInt *bi = rjs_value_get_big_int(rt, v);

    return mpz_sgn(&bi->mpz) == 0;
}

/**
 * Convert the big integer value to string.
 * \param rt The current runtime.
 * \param v The big integer value.
 * \param radix The radix.
 * \param[out] s Return the string value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_to_string (RJS_Runtime *rt, RJS_Value *v, int radix, RJS_Value *s)
{
    RJS_BigInt *bi;
    char       *cstr;
    RJS_Result  r;
    void (*free_func) (void *, size_t);

    bi = rjs_value_get_big_int(rt, v);

    cstr = mpz_get_str(NULL, radix, &bi->mpz);

    r = rjs_string_from_chars(rt, s, cstr, -1);

    mp_get_memory_functions(NULL, NULL, &free_func);

    free_func(cstr, strlen(cstr) + 1);

    return r;
}

/**
 * Big integer unary minus operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_unary_minus (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_BigInt *src, *dst;

    src = rjs_value_get_big_int(rt, v);
    dst = big_int_new_init(rt, rv);

    mpz_neg(&dst->mpz, &src->mpz);

    return RJS_OK;
}

/**
 * Big integer bitwise not operation.
 * \param rt The current runtime.
 * \param v Input value.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_bitwise_not (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_BigInt *src, *dst;

    src = rjs_value_get_big_int(rt, v);
    dst = big_int_new_init(rt, rv);

    mpz_com(&dst->mpz, &src->mpz);

    return RJS_OK;
}

/**
 * Big integer increase.
 * \param rt The current runtime.
 * \param v Input number.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_inc (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_BigInt *src, *dst;

    src = rjs_value_get_big_int(rt, v);
    dst = big_int_new_init(rt, rv);

    mpz_add_ui(&dst->mpz, &src->mpz, 1);

    return RJS_OK;
}

/**
 * Big integer increase.
 * \param rt The current runtime.
 * \param v Input number.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_dec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_BigInt *src, *dst;

    src = rjs_value_get_big_int(rt, v);
    dst = big_int_new_init(rt, rv);

    mpz_sub_ui(&dst->mpz, &src->mpz, 1);

    return RJS_OK;
}

/**
 * Big integer add operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_add (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    mpz_add(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer subtract operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_subtract (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    mpz_sub(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer multiply operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_multiply (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    mpz_mul(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer divide operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_divide (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);

    if (mpz_sgn(&src2->mpz) == 0)
        return rjs_throw_range_error(rt, _("cannot be divided by 0"));

    dst = big_int_new_init(rt, rv);

    mpz_tdiv_q(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer remainder operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_remainder (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);

    if (mpz_sgn(&src2->mpz) == 0)
        return rjs_throw_range_error(rt, _("cannot be divided by 0"));

    dst = big_int_new_init(rt, rv);

    mpz_tdiv_r(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer exponentiate operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_exponentiate (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;
    unsigned long int exp;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);

    if (mpz_sgn(&src2->mpz) < 0)
        return rjs_throw_range_error(rt, _("exponent cannot < 0"));

    dst = big_int_new_init(rt, rv);

    exp = mpz_get_ui(&src2->mpz);
    mpz_pow_ui(&dst->mpz, &src1->mpz, exp);

    return RJS_OK;
}

/**
 * Big integer left shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_left_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;
    long int bits;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    bits = mpz_get_si(&src2->mpz);

    if (bits < 0)
        mpz_div_2exp(&dst->mpz, &src1->mpz, -bits);
    else
        mpz_mul_2exp(&dst->mpz, &src1->mpz, bits);

    return RJS_OK;
}

/**
 * Big integer signed right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_signed_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;
    long int bits;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    bits = mpz_get_si(&src2->mpz);

    if (bits < 0)
        mpz_mul_2exp(&dst->mpz, &src1->mpz, -bits);
    else
        mpz_div_2exp(&dst->mpz, &src1->mpz, bits);

    return RJS_OK;
}

/**
 * Big integer unsigned right shift operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_unsigned_right_shift (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    return rjs_throw_type_error(rt, _("cannot unsigned right shift to a big integer"));
}

/**
 * Big integer bitwise and operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_bitwise_and (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    mpz_and(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer bitwise xor operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_bitwise_xor (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    mpz_xor(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Big integer bitwise or operation.
 * \param rt The current runtime.
 * \param v1 Number 1.
 * \param v2 Number 2.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_bitwise_or (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    RJS_BigInt *src1, *src2, *dst;

    src1 = rjs_value_get_big_int(rt, v1);
    src2 = rjs_value_get_big_int(rt, v2);
    dst  = big_int_new_init(rt, rv);

    mpz_ior(&dst->mpz, &src1->mpz, &src2->mpz);

    return RJS_OK;
}

/**
 * Compare 2 big integers.
 * \param rt The current runtime.
 * \param v1 Big integer 1.
 * \param v2 big integer 2.
 * \retval RJS_COMPARE_LESS v1 < v2.
 * \retval RJS_COMPARE_GREATER v1 > v2.
 * \retval RJS_COMPARE_EQUAL v1 == v2.
 */
RJS_Result
rjs_big_int_compare (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_BigInt *bi1, *bi2;
    int         r;

    bi1 = rjs_value_get_big_int(rt, v1);
    bi2 = rjs_value_get_big_int(rt, v2);

    r = mpz_cmp(&bi1->mpz, &bi2->mpz);

    if (r == 0)
        return RJS_COMPARE_EQUAL;
    if (r < 0)
        return RJS_COMPARE_LESS;

    return RJS_COMPARE_GREATER;
}

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
RJS_Result
rjs_big_int_compare_number (RJS_Runtime *rt, RJS_Value *v, RJS_Number n)
{
    RJS_BigInt *bi;
    int         r;

    if (isnan(n))
        return RJS_COMPARE_UNDEFINED;
        
    if (isinf(n)) {
        if (n < 0)
            return RJS_COMPARE_GREATER;
        else
            return RJS_COMPARE_LESS;
    }

    bi = rjs_value_get_big_int(rt, v);
    r  = mpz_cmp_d(&bi->mpz, n);

    if (r == 0)
        return RJS_COMPARE_EQUAL;
    if (r < 0)
        return RJS_COMPARE_LESS;

    return RJS_COMPARE_GREATER;
}

/**
 * Create the signed big interger use the last bits of another big integer.
 * \param rt The current runtime.
 * \param bits Bits number.
 * \param v The input big integer.
 * \param[out] rv The result big integer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_as_int_n (RJS_Runtime *rt, int64_t bits, RJS_Value *v, RJS_Value *rv)
{
    RJS_BigInt  *src, *dst;
    mpz_t        tmp;

    src = rjs_value_get_big_int(rt, v);
    dst = big_int_new_init(rt, rv);

    mpz_init(tmp);
    mpz_fdiv_r_2exp(tmp, &src->mpz, bits);

    if (bits && mpz_tstbit(tmp, bits - 1)) {
        mpz_t t1, t2;

        mpz_init_set_ui(t1, 1);
        mpz_init(t2);
        mpz_mul_2exp(t2, t1, bits);
        mpz_sub(&dst->mpz, tmp, t2);

        mpz_clear(t1);
        mpz_clear(t2);
    } else {
        mpz_set(&dst->mpz, tmp);
    }

    mpz_clear(tmp);

    return RJS_OK;
}

/**
 * Create the unsigned big interger use the last bits of another big integer.
 * \param rt The current runtime.
 * \param bits Bits number.
 * \param v The input big integer.
 * \param[out] rv The result big integer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_big_int_as_uint_n (RJS_Runtime *rt, int64_t bits, RJS_Value *v, RJS_Value *rv)
{
    RJS_BigInt *src, *dst;

    src = rjs_value_get_big_int(rt, v);
    dst = big_int_new_init(rt, rv);

    mpz_fdiv_r_2exp(&dst->mpz, &src->mpz, bits);

    return RJS_OK;
}
