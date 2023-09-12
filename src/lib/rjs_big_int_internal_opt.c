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

/*Get the big integer data from the value.*/
static RJS_BI*
bi_get (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_BigInt *bi = rjs_value_get_big_int(rt, v);

    return &bi->bi;
}

/*Initialize the big integer data.*/
static void
bi_init (RJS_BI *bi)
{
    bi->n    = NULL;
    bi->size = 0;
    bi->cap  = 0;
}

/*Release the big integer data.*/
static void
bi_deinit (RJS_Runtime *rt, RJS_BI *bi)
{
    if (bi->n)
        RJS_DEL_N(rt, bi->n, bi->cap);
}

/*Get the size of the number buffer of the big integer.*/
static int
bi_size (RJS_BI *bi)
{
    return RJS_ABS(bi->size);
}

/*Get the sign of the big integer.*/
static int
bi_positive (RJS_BI *bi)
{
    if (bi->size > 0)
        return 1;
    if (bi->size < 0)
        return -1;

    return 0;
}

/*Check if big integer is 1 or -1.*/
static RJS_Bool
bi_is_1 (RJS_BI *bi)
{
    if (bi_size(bi) != 1)
        return RJS_FALSE;

    return bi->n[0] == 1;
}

/*Set the big integer data to zero.*/
static void
bi_set_0 (RJS_BI *bi)
{
    bi->size = 0;
}

/*Set the big integer data's number buffer capacity.*/
static RJS_Result
bi_set_cap (RJS_Runtime *rt, RJS_BI *bi, int cap)
{
    uint32_t *n;

    if (cap <= bi->cap)
        return RJS_OK;

    n = rjs_realloc(rt, bi->n, sizeof(uint32_t) * bi->cap, sizeof(uint32_t) * cap);
    if (!n) {
        return rjs_throw_type_error(rt, _("not enough memory"));
    }

    bi->n   = n;
    bi->cap = cap;
    return RJS_OK;
}

/*Set the big integer data's number item.*/
static RJS_Result
bi_set_item (RJS_Runtime *rt, RJS_BI *bi, int idx, uint32_t n)
{
    RJS_Result r;
    int        size;

    size = idx + 1;

    if (idx >= bi->cap) {
        int cap = size;

        cap = RJS_MAX_3(cap, bi->cap * 2, 4);

        if ((r = bi_set_cap(rt, bi, cap)) == RJS_ERR)
            return r;
    }

    bi->n[idx] = n;

    if (size > bi_size(bi)) {
        if (bi_positive(bi) < 0)
            bi->size = -size;
        else
            bi->size = size;
    }

    return RJS_OK;
}

/*Remove the unused 0 of the big integer data.*/
static void
bi_end (RJS_BI *bi)
{
    int size = bi_size(bi);
    int pos;
    int i;

    if (!size)
        return;

    pos = bi_positive(bi);

    for (i = size - 1; i >= 0; i --) {
        if (bi->n[i])
            break;
    }

    if (i < 0)
        bi->size = 0;
    else if (pos < 0)
        bi->size = - (i + 1);
    else
        bi->size = i + 1;
}

/*Duplicate the big integer data.*/
static RJS_Result
bi_dup (RJS_Runtime *rt, RJS_BI *dst, RJS_BI *src)
{
    int        size = bi_size(src);
    RJS_Result r;

    if ((r = bi_set_cap(rt, dst, size)) == RJS_ERR)
        return r;

    RJS_ELEM_CPY(dst->n, src->n, size);

    dst->size = src->size;
    return RJS_OK;
}

/*Set the big integer data to number.*/
static RJS_Result
bi_set_number (RJS_Runtime *rt, RJS_BI *bi, RJS_Number n)
{
    RJS_Result r;

    if (n != 0) {
        uint64_t *p = (uint64_t*)&n;
        uint64_t  m;
        uint32_t  v;
        int       e, i;

        m = (*p & 0xfffffffffffffull) | 0x10000000000000ull;
        e = ((*p >> 52) & 0x7ff) - 1023;

        bi_set_cap(rt, bi, RJS_ALIGN_UP(e + 1, 32) / 32);

        i = 0;
        while (e >= 84) {
            if ((r = bi_set_item(rt, bi, i, 0)) == RJS_ERR)
                return r;

            i ++;
            e -= 32;
        }

        if (e > 52) {
            v = m << (e - 52);
            m >>= 32 - (e - 52);

            if ((r = bi_set_item(rt, bi, i, v)) == RJS_ERR)
                return r;

            i ++;
            e -= 32;
        } else if (e < 52) {
            m >>= 52 - e;
        }

        while (e >= 0) {
            v = m & 0xffffffff;
            m >>= 32;

            if ((r = bi_set_item(rt, bi, i, v)) == RJS_ERR)
                return r;

            i ++;
            e -= 32;
        }

        if (n < 0)
            bi->size = - bi->size;

        bi_end(bi);
    } else {
        bi_set_0(bi);
    }

    return RJS_OK;
}

/*Compare 2 bit integer data's absolute value.*/
static RJS_Result
bi_compare_abs (RJS_BI *bi1, RJS_BI *bi2)
{
    int        size1 = bi_size(bi1);
    int        size2 = bi_size(bi2);
    RJS_Result r;

    if (size1 < size2) {
        r = RJS_COMPARE_LESS;
    } else if (size1 > size2) {
        r = RJS_COMPARE_GREATER;
    } else {
        int i;

        r = RJS_COMPARE_EQUAL;

        for (i = size1 - 1; i >= 0; i --) {
            if (bi1->n[i] > bi2->n[i]) {
                r = RJS_COMPARE_GREATER;
                break;
            } else if (bi1->n[i] < bi2->n[i]) {
                r = RJS_COMPARE_LESS;
                break;
            }
        }
    }

    return r;
}

/*Compare 2 bit integer data.*/
static RJS_Result
bi_compare (RJS_BI *bi1, RJS_BI *bi2)
{
    int        pos1  = bi_positive(bi1);
    int        pos2  = bi_positive(bi2);
    RJS_Result r;

    if (pos1 < 0) {
        if (pos2 >= 0)
            return RJS_COMPARE_LESS;
    } else if (pos1 == 0) {
        if (pos2 < 0)
            return RJS_COMPARE_GREATER;
        if (pos2 == 0)
            return RJS_COMPARE_EQUAL;

        return RJS_COMPARE_LESS;
    } else {
        if (pos2 <= 0)
            return RJS_COMPARE_GREATER;
    }

    r = bi_compare_abs(bi1, bi2);

    if (pos1 < 0) {
        if (r == RJS_COMPARE_LESS)
            r = RJS_COMPARE_GREATER;
        else if (r == RJS_COMPARE_GREATER)
            r = RJS_COMPARE_LESS;
    }

    return r;
}

/*Big integer data multiply an integer.*/
static RJS_Result
bi_mul_int (RJS_Runtime *rt, RJS_BI *src, uint32_t n, RJS_BI *dst, int shift)
{
    RJS_Result r;
    int        i, j;
    int        size = bi_size(src);
    uint64_t   left = 0;

    if (n == 0) {
        bi_set_0(dst);
        return RJS_OK;
    } else if ((n == 1) && (shift == 0)) {
        return bi_dup(rt, dst, src);
    }

    bi_set_0(dst);

    for (j = 0; j < shift; j ++) {
        if ((r = bi_set_item(rt, dst, j, 0)) == RJS_ERR)
            return r;
    }

    for (i = 0; i < size; i ++, j ++) {
        uint64_t res;

        res = n * ((uint64_t)src->n[i]) + left;

        if ((r = bi_set_item(rt, dst, j, res & 0xffffffff)) == RJS_ERR)
            return r;

        left = res >> 32;
    }

    if (left) {
        if ((r = bi_set_item(rt, dst, j, left)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/*Big integer data add an integer.*/
static RJS_Result
bi_self_add_int (RJS_Runtime *rt, RJS_BI *bi, uint32_t n)
{
    int        i;
    RJS_Result r;
    int        size = bi_size(bi);
    uint64_t   left = n;

    for (i = 0; i < size; i ++) {
        uint64_t res;

        res = bi->n[i] + left;

        bi->n[i] = res & 0xffffffff;

        left = res >> 32;

        if (!left)
            break;
    }

    if (left) {
        if ((r = bi_set_item(rt, bi, i, left)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/*Big integer data add an integer.*/
static RJS_Result
bi_add_int (RJS_Runtime *rt, RJS_BI *src, uint32_t n, RJS_BI *dst)
{
    int        size = bi_size(src);
    uint64_t   left = n;
    int        i;
    RJS_Result r;

    bi_set_0(dst);

    if ((r = bi_set_cap(rt, dst, size)) == RJS_ERR)
        return r;

    for (i = 0; i < size; i ++) {
        uint64_t sum;

        sum = src->n[i] + left;

        if ((r = bi_set_item(rt, dst, i, sum & 0xffffffff)) == RJS_ERR)
            return r;

        left = sum >> 32;
    }

    if (left) {
        if ((r = bi_set_item(rt, dst, i, left)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/*Big integer data substract an integer.*/
static RJS_Result
bi_sub_int (RJS_Runtime *rt, RJS_BI *src, uint32_t n, RJS_BI *dst)
{
    int        size = bi_size(src);
    int64_t    left = n;
    int        i;
    RJS_Result r;

    bi_set_0(dst);

    if ((r = bi_set_cap(rt, dst, size)) == RJS_ERR)
        return r;

    for (i = 0; i < size; i ++) {
        int64_t  sum;
        uint32_t v;

        sum = src->n[i] - left;

        if (sum < 0) {
            v    = 0x100000000ull + sum;
            left = 1;
        } else {
            v    = sum;
            left = 0;
        }

        if ((r = bi_set_item(rt, dst, i, v)) == RJS_ERR)
            return r;
    }

    bi_end(dst);
    return RJS_OK;
}

/*Big integer data divide a integer number.*/
static RJS_Result
bi_div_int (RJS_Runtime *rt, RJS_BI *src, int n, RJS_BI *dst, int *rem)
{
    RJS_Result r;
    int        i;
    int        size = bi_size(src);
    uint64_t   left = 0;

    if (n == 1) {
        *rem = 0;
        return bi_dup(rt, src, dst);
    }

    bi_set_0(dst);

    for (i = size - 1; i >= 0; i --) {
        uint64_t sum, div;

        left <<= 32;

        sum  = src->n[i] + left;
        div  = sum / n;
        left = sum % n;

        if ((r = bi_set_item(rt, dst, i, div)) == RJS_ERR)
            return r;
    }

    *rem = left;

    bi_end(dst);
    return RJS_OK;
}

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

    bi_deinit(rt, &bi->bi);

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
static RJS_BI*
big_int_new (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_BigInt *bi;

    RJS_NEW(rt, bi);

    bi_init(&bi->bi);

    rjs_value_set_big_int(rt, v, bi);
    rjs_gc_add(rt, bi, &big_int_ops);

    return &bi->bi;
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
    RJS_BI *bi;

    if (!rjs_is_integral_number(n)) {
        return rjs_throw_range_error(rt, _("the value is not an integer"));
    }

    bi = big_int_new(rt, v);

    return bi_set_number(rt, bi, n);
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
    RJS_BI     *bi;
    RJS_BI      tmp1, tmp2;
    RJS_BI     *obi, *nbi;
    const char *c;
    RJS_Result  r;

    bi = big_int_new(rt, v);

    bi_init(&tmp1);
    bi_init(&tmp2);

    obi = &tmp1;
    nbi = &tmp2;

    c = chars;
    while (*c) {
        int     v = rjs_hex_char_to_number(*c);
        RJS_BI *t;

        if ((r = bi_mul_int(rt, obi, base, nbi, 0)) == RJS_ERR)
            goto end;
        if ((r = bi_self_add_int(rt, nbi, v)) == RJS_ERR)
            goto end;

        t   = obi;
        obi = nbi;
        nbi = t;

        c ++;
    }

    if ((r = bi_dup(rt, bi, obi)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    bi_deinit(rt, &tmp1);
    bi_deinit(rt, &tmp2);
    return r;
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
    RJS_BI     *bi;
    RJS_Result  r;

    bi = big_int_new(rt, v);

    if ((r = bi_set_item(rt, bi, 0, RJS_ABS(i))) == RJS_ERR)
        return r;

    if (i < 0)
        bi->size = - bi->size;

    bi_end(bi);
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
    RJS_BI     *bi;
    int64_t     a;
    RJS_Result  r;

    bi = big_int_new(rt, v);

    a = RJS_ABS(i);

    if ((r = bi_set_item(rt, bi, 0, a & 0xffffffff)) == RJS_ERR)
        return r;

    if ((r = bi_set_item(rt, bi, 1, a >> 32)) == RJS_ERR)
        return r;

    if (i < 0)
        bi->size = - bi->size;

    bi_end(bi);
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
    RJS_BI     *bi;
    RJS_Result  r;

    bi = big_int_new(rt, v);

    if ((r = bi_set_item(rt, bi, 0, i & 0xffffffff)) == RJS_ERR)
        return r;

    if ((r = bi_set_item(rt, bi, 1, i >> 32)) == RJS_ERR)
        return r;

    bi_end(bi);
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
    RJS_BI *bi  = bi_get(rt, v);
    int     pos = bi_positive(bi);

    if (pos == 0) {
        *pi = 0;
    } else {
        int64_t i;

        i = bi->n[0];

        if (RJS_ABS(bi->size) > 1)
            i |= ((uint64_t)bi->n[1]) << 32;

        if (pos < 0)
            i = -i;

        *pi = i;
    }

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
    RJS_BI *bi  = bi_get(rt, v);
    int     pos = bi_positive(bi);

    if (pos == 0) {
        *pi = 0;
    } else {
        int64_t i;

        i = bi->n[0];

        if (RJS_ABS(bi->size) > 1)
            i |= ((uint64_t)bi->n[1]) << 32;

        if (pos < 0)
            i = -i;

        *pi = i;
    }

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
    RJS_BI *bi1 = bi_get(rt, v1);
    RJS_BI *bi2 = bi_get(rt, v2);
    int     i, size;

    if (bi1->size != bi2->size)
        return RJS_FALSE;

    size = bi_size(bi1);

    for (i = 0; i < size; i ++) {
        if (bi1->n[i] != bi2->n[i])
            return RJS_FALSE;
    }

    return RJS_TRUE;
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
    RJS_BI *bi = bi_get(rt, v);

    return bi_positive(bi) == 0;
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
    RJS_BI         *bi  = bi_get(rt, v);
    int             pos = bi_positive(bi);
    RJS_BI          tmp1, tmp2;
    RJS_BI         *curr, *res;
    RJS_CharBuffer  cb;
    ssize_t         i, len;
    RJS_UChar      *c;
    RJS_Result      r;

    rjs_char_buffer_init(rt, &cb);

    bi_init(&tmp1);
    bi_init(&tmp2);

    if (pos == 0) {
        r = rjs_string_from_chars(rt, s, "0", -1);
        goto end;
    }

    curr = &tmp1;
    res  = &tmp2;

    bi_dup(rt, curr, bi);

    while (bi_positive(curr) != 0) {
        int     rem;
        int     c;
        RJS_BI *t;

        if ((r = bi_div_int(rt, curr, radix, res, &rem)) == RJS_ERR)
            goto end;

        if (rem < 10)
            c = rem + '0';
        else
            c = rem - 10 + 'a';

        rjs_char_buffer_append_char(rt, &cb, c);

        t    = res;
        res  = curr;
        curr = t;
    }

    len = cb.item_num;

    if (pos < 0)
        len ++;

    if ((r = rjs_string_from_chars(rt, s, NULL, len)) == RJS_ERR)
        goto end;

    c = (RJS_UChar*)rjs_string_get_uchars(rt, s);

    if (pos < 0)
        *c ++ = '-';

    for (i = cb.item_num - 1; i >= 0; i --) {
        *c ++ = cb.items[i];
    }

    r = RJS_OK;
end:
    bi_deinit(rt, &tmp1);
    bi_deinit(rt, &tmp2);
    rjs_char_buffer_deinit(rt, &cb);
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
    RJS_BI     *src = bi_get(rt, v);
    RJS_BI     *dst = big_int_new(rt, rv);
    RJS_Result  r;

    if (bi_positive(src) != 0) {
        if ((r = bi_dup(rt, dst, src)) == RJS_ERR)
            return r;

        dst->size = - dst->size;
    }

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
    RJS_BI     *src = bi_get(rt, v);
    RJS_BI     *dst = big_int_new(rt, rv);
    int         pos = bi_positive(src);
    RJS_Result  r;

    if (pos >= 0) {
        if ((r = bi_add_int(rt, src, 1, dst)) == RJS_ERR)
            goto end;

        dst->size = - dst->size;
    } else {
        if ((r = bi_sub_int(rt, src, 1, dst)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    return r;
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
    RJS_BI     *src = bi_get(rt, v);
    RJS_BI     *dst = big_int_new(rt, rv);
    int         pos = bi_positive(src);
    RJS_Result  r;

    if (pos >= 0) {
        if ((r = bi_add_int(rt, src, 1, dst)) == RJS_ERR)
            goto end;
    } else {
        if ((r = bi_sub_int(rt, src, 1, dst)) == RJS_ERR)
            goto end;

        if (bi_positive(dst) != 0)
            dst->size = - dst->size;
    }

    r = RJS_OK;
end:
    return r;
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
    RJS_BI     *src = bi_get(rt, v);
    RJS_BI     *dst = big_int_new(rt, rv);
    int         pos = bi_positive(src);
    RJS_Result  r;

    if (pos > 0) {
        if ((r = bi_sub_int(rt, src, 1, dst)) == RJS_ERR)
            goto end;
    } else if (pos == 0) {
        if ((r = bi_set_item(rt, dst, 0, 1)) == RJS_ERR)
            goto end;

        dst->size = -1;
    } else {
        if ((r = bi_add_int(rt, src, 1, dst)) == RJS_ERR)
            goto end;

        dst->size = - dst->size;
    }

    r = RJS_OK;
end:
    return r;
}

/* Self add */
static RJS_Result
bi_self_add (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2)
{
    int        size1 = bi_size(src1);
    int        size2 = bi_size(src2);
    int        size  = RJS_MAX(size1, size2);
    uint64_t   left  = 0;
    int        i;
    RJS_Result r;

    if ((r = bi_set_cap(rt, src1, size)) == RJS_ERR)
        return r;

    for (i = 0; i < size; i ++) {
        uint64_t n1, n2, sum;

        if (i < size1)
            n1 = src1->n[i];
        else
            n1 = 0;

        if (i < size2)
            n2 = src2->n[i];
        else
            n2 = 0;

        sum = n1 + n2 + left;

        if ((r = bi_set_item(rt, src1, i, sum & 0xffffffff)) == RJS_ERR)
            return r;

        left = sum >> 32;
    }

    if (left) {
        if ((r = bi_set_item(rt, src1, i, left)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/* + */
static RJS_Result
bi_add (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2, RJS_BI *dst)
{
    int        size1 = bi_size(src1);
    int        size2 = bi_size(src2);
    int        size  = RJS_MAX(size1, size2);
    uint64_t   left  = 0;
    int        i;
    RJS_Result r;

    bi_set_0(dst);

    if ((r = bi_set_cap(rt, dst, size)) == RJS_ERR)
        return r;

    for (i = 0; i < size; i ++) {
        uint64_t n1, n2, sum;

        if (i < size1)
            n1 = src1->n[i];
        else
            n1 = 0;

        if (i < size2)
            n2 = src2->n[i];
        else
            n2 = 0;

        sum = n1 + n2 + left;

        if ((r = bi_set_item(rt, dst, i, sum & 0xffffffff)) == RJS_ERR)
            return r;

        left = sum >> 32;
    }

    if (left) {
        if ((r = bi_set_item(rt, dst, i, left)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/* a - b (a must  > b)*/
static RJS_Result
bi_self_sub_greater (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2)
{
    int        size1;
    int        size2;
    int        size;
    int64_t    left;
    int        i;
    RJS_Result r;

    size1 = bi_size(src1);
    size2 = bi_size(src2);
    size  = RJS_MAX(size1, size2);
    left  = 0;

    for (i = 0; i < size; i ++) {
        int64_t  n1, n2, res;
        uint32_t v;

        if (i < size1)
            n1 = src1->n[i];
        else
            n1 = 0;

        if (i < size2)
            n2 = src2->n[i];
        else
            n2 = 0;

        res = n1 - n2 - left;

        if (res < 0) {
            v    = res + 0x100000000ull;
            left = 1;
        } else {
            v = res;
        }

        if ((r = bi_set_item(rt, src1, i, v)) == RJS_ERR)
            return r;

        if ((left == 0) && (i >= size2))
            break;
    }

    bi_end(src1);
    return RJS_OK;
}

/* a - b (a must  > b)*/
static RJS_Result
bi_sub_greater (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2, RJS_BI *dst)
{
    int        size1;
    int        size2;
    int        size;
    int64_t    left;
    int        i;
    RJS_Result r;

    bi_set_0(dst);

    size1 = bi_size(src1);
    size2 = bi_size(src2);
    size  = RJS_MAX(size1, size2);
    left  = 0;

    for (i = 0; i < size; i ++) {
        int64_t  n1, n2, res;
        uint32_t v;

        if (i < size1)
            n1 = src1->n[i];
        else
            n1 = 0;

        if (i < size2)
            n2 = src2->n[i];
        else
            n2 = 0;

        res = n1 - n2 - left;

        if (res < 0) {
            v    = res + 0x100000000ull;
            left = 1;
        } else {
            v = res;
        }

        if ((r = bi_set_item(rt, dst, i, v)) == RJS_ERR)
            return r;
    }

    bi_end(dst);
    return RJS_OK;
}

/* - */
static RJS_Result
bi_sub (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2, RJS_BI *dst)
{
    int        pos;
    RJS_Result r;

    r = bi_compare_abs(src1, src2);
    if (r == RJS_COMPARE_EQUAL) {
        bi_set_0(dst);
        return RJS_OK;
    }

    if (r == RJS_COMPARE_LESS) {
        RJS_BI *t = src1;

        src1 = src2;
        src2 = t;
        pos  = -1;
    } else {
        pos  = 1;
    }

    if ((r = bi_sub_greater(rt, src1, src2, dst)) == RJS_ERR)
        return r;

    if (pos == -1)
        dst->size = - dst->size;

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
    RJS_BI     *src1 = bi_get(rt, v1);
    RJS_BI     *src2 = bi_get(rt, v2);
    int         pos1 = bi_positive(src1);
    int         pos2 = bi_positive(src2);
    RJS_BI     *dst  = big_int_new(rt, rv);
    RJS_Result  r;

    if (pos1 >= 0)
        pos1 = 1;
    if (pos2 >= 0)
        pos2 = 1;

    if (pos1 == pos2) {
        if ((r = bi_add(rt, src1, src2, dst)) == RJS_ERR)
            goto end;
    } else {
        if ((r = bi_sub(rt, src1, src2, dst)) == RJS_ERR)
            goto end;
    }

    if (bi_positive(dst)) {
        if (pos1 < 0)
            dst->size = - dst->size;
    }

    r = RJS_OK;
end:
    return r;
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
    RJS_BI     *src1 = bi_get(rt, v1);
    RJS_BI     *src2 = bi_get(rt, v2);
    int         pos1 = bi_positive(src1);
    int         pos2 = bi_positive(src2);
    RJS_BI     *dst  = big_int_new(rt, rv);
    RJS_Result  r;

    if (pos1 >= 0)
        pos1 = 1;
    if (pos2 >= 0)
        pos2 = 1;

    if (pos1 == pos2) {
        if ((r = bi_sub(rt, src1, src2, dst)) == RJS_ERR)
            goto end;
    } else {
        if ((r = bi_add(rt, src1, src2, dst)) == RJS_ERR)
            goto end;
    }

    if (pos1 < 0)
        dst->size = - dst->size;

    r = RJS_OK;
end:
    return r;
}

/*Multiplay.*/
static RJS_Result
bi_mul (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2, RJS_BI *dst)
{
    int         size2 = bi_size(src2);
    int         pos1  = bi_positive(src1);
    int         pos2  = bi_positive(src2);
    int         i;
    RJS_BI      tmp1, tmp2;
    RJS_BI     *sum   = &tmp1;
    RJS_BI     *mres  = &tmp2;
    RJS_Result  r;

    bi_init(&tmp1);
    bi_init(&tmp2);

    if (!pos1 || !pos2) {
        r = RJS_OK;
        goto end;
    }

    if (bi_is_1(src1)) {
        if ((r = bi_dup(rt, dst, src2)) == RJS_ERR)
            goto end;

        if (pos1 < 0)
            dst->size = - dst->size;

        r = RJS_OK;
        goto end;
    }

    if (bi_is_1(src2)) {
        if ((r = bi_dup(rt, dst, src1)) == RJS_ERR)
            goto end;

        if (pos2 < 0)
            dst->size = - dst->size;

        r = RJS_OK;
        goto end;
    }

    for (i = 0; i < size2; i ++) {
        uint32_t n = src2->n[i];

        if ((r = bi_mul_int(rt, src1, n, mres, i)) == RJS_ERR)
            goto end;

        if (i == 0) {
            if ((r = bi_dup(rt, sum, mres)) == RJS_ERR)
                goto end;
        } else {
            if ((r = bi_self_add(rt, sum, mres)) == RJS_ERR)
                goto end;
        }
    }

    if ((r = bi_dup(rt, dst, sum)) == RJS_ERR)
        goto end;

    if (pos1 != pos2)
        dst->size = - dst->size;

    r = RJS_OK;
end:
    bi_deinit(rt, &tmp1);
    bi_deinit(rt, &tmp2);
    return r;
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
    RJS_BI *src1 = bi_get(rt, v1);
    RJS_BI *src2 = bi_get(rt, v2);
    RJS_BI *dst  = big_int_new(rt, rv);

    return bi_mul(rt, src1, src2, dst);
}

/*Divide.*/
static RJS_Result
bi_div (RJS_Runtime *rt, RJS_BI *op1, RJS_BI *op2, RJS_BI *res, RJS_BI *rem)
{
    int        pos1  = bi_positive(op1);
    int        pos2  = bi_positive(op2);
    int        size1 = bi_size(op1);
    int        size2 = bi_size(op2);
    int        size;
    RJS_BI     qr, left, cmp, add, tmp;
    RJS_Result r;

    bi_init(&qr);
    bi_init(&left);
    bi_init(&cmp);
    bi_init(&add);
    bi_init(&tmp);

    if (pos2 == 0) {
        r = rjs_throw_range_error(rt, _("cannot be divided by 0"));
        goto end;
    }

    if (pos1 == 0) {
        r = RJS_OK;
        goto end;
    }

    if (bi_is_1(op2)) {
        if (res) {
            if ((r = bi_dup(rt, res, op1)) == RJS_ERR)
                goto end;

            if (pos2 < 0)
                res->size = - res->size;
        }

        r = RJS_OK;
        goto end;
    }

    if ((r = bi_dup(rt, &left, op1)) == RJS_ERR)
        goto end;

    left.size = size1;

    for (size = size1; size >= size2; size --) {
        int      shift = size - size2;
        uint64_t n;
        int      i, j;
        uint64_t v1, v2;

        bi_set_0(&cmp);

        for (i = 0; i < shift; i ++) {
            if ((r = bi_set_item(rt, &cmp, i, 0)) == RJS_ERR)
                goto end;
        }

        for (j = 0; i < size; j ++, i ++) {
            uint32_t v = op2->n[j];

            if ((r = bi_set_item(rt, &cmp, i, v)) == RJS_ERR)
                goto end;
        }

        v2 = cmp.n[size - 1];
        if (size - 1)
            v2 ++;

        while (1) {
            v1 = left.n[size - 1];
            if (size < bi_size(&left))
                v1 |= ((uint64_t)left.n[size]) << 32;

            n = v1 / v2;
            if (n == 0) {
                if (size == size2) {
                    RJS_Result r = bi_compare(&left, &cmp);

                    if ((r == RJS_COMPARE_EQUAL) || (r == RJS_COMPARE_GREATER))
                        n = 1;
                }

                if (n == 0)
                    break;
            }

            if (n > 0xffffffff)
                n = 0xffffffff;

            bi_set_0(&add);

            for (i = 0; i < shift; i ++) {
                if ((r = bi_set_item(rt, &add, i, 0)) == RJS_ERR)
                    goto end;
            }

            if ((r = bi_set_item(rt, &add, i, n)) == RJS_ERR)
                goto end;

            if ((r = bi_self_add(rt, &qr, &add)) == RJS_ERR)
                goto end;

            if ((r = bi_mul_int(rt, &cmp, n, &tmp, 0)) == RJS_ERR)
                goto end;

            if ((r = bi_self_sub_greater(rt, &left, &tmp)) == RJS_ERR)
                goto end;
        }
    }

    if (pos1 != pos2) {
        qr.size = - qr.size;
    }

    if (pos1 < 0) {
        left.size = - left.size;
    }

    if (res) {
        if ((r = bi_dup(rt, res, &qr)) == RJS_ERR)
            goto end;
    }

    if (rem) {
        if ((r = bi_dup(rt, rem, &left)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    bi_deinit(rt, &qr);
    bi_deinit(rt, &left);
    bi_deinit(rt, &cmp);
    bi_deinit(rt, &add);
    bi_deinit(rt, &tmp);
    return r;
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
    RJS_BI *src1 = bi_get(rt, v1);
    RJS_BI *src2 = bi_get(rt, v2);
    RJS_BI *dst  = big_int_new(rt, rv);

    return bi_div(rt, src1, src2, dst, NULL);
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
    RJS_BI *src1 = bi_get(rt, v1);
    RJS_BI *src2 = bi_get(rt, v2);
    RJS_BI *dst  = big_int_new(rt, rv);

    return bi_div(rt, src1, src2, NULL, dst);
}

/**Big integer map.*/
typedef RJS_VECTOR_DECL(RJS_BI) RJS_BIMap;

/*Resize the big integer map.*/
static RJS_Result
bi_map_resize (RJS_Runtime *rt, RJS_BIMap *map, size_t size)
{
    size_t i;

    if (map->item_num >= size)
        return RJS_OK;

    rjs_vector_set_capacity(map, size, rt);

    for (i = map->item_num; i < size; i ++) {
        RJS_BI *bi = &map->items[i];

        bi_init(bi);
    }

    map->item_num = size;

    return RJS_OK;
}

/*Lookup the big integer map.*/
static RJS_BI*
bi_map_lookup (RJS_BIMap *map, int shift)
{
    RJS_BI *bi;

    if (shift >= map->item_num)
        return NULL;

    bi = &map->items[shift];

    if (bi_positive(bi) == 0)
        return NULL;

    return bi;
}

/*Get the big integer from the map.*/
static RJS_BI*
bi_map_get (RJS_Runtime *rt, RJS_BIMap *map, int shift)
{
    RJS_BI    *src, *dst;
    RJS_Result r;

    dst = bi_map_lookup(map, shift);
    if (dst)
        return dst;

    bi_map_resize(rt, map, shift + 1);

    dst = &map->items[shift];
    src = bi_map_get(rt, map, shift - 1);

    if ((r = bi_mul(rt, src, src, dst)) == RJS_ERR)
        return NULL;

    return dst;
}

/*Exponentiate*/
static RJS_Result
bi_exp (RJS_Runtime *rt, RJS_BIMap *map, uint32_t e, RJS_BI *res)
{
    uint64_t   v     = 1;
    int        shift = 0;
    RJS_BI    *bi, tmp;
    RJS_Result r;

    while (1) {
        if ((v << 1) > e)
            break;

        v <<= 1;
        shift ++;

        if (v == e)
            break;
    }

    if (!(bi = bi_map_get(rt, map, shift)))
        return RJS_ERR;

    bi_init(&tmp);

    if ((r = bi_mul(rt, bi, res, &tmp)) == RJS_OK) {
        RJS_BI t = tmp;

        tmp  = *res;
        *res = t;
    }

    bi_deinit(rt, &tmp);
    if (r == RJS_ERR)
        return r;

    if (e > v) {
        if ((r = bi_exp(rt, map, e - v, res)) == RJS_ERR)
            return r;
    }

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
    RJS_BI     *src1 = bi_get(rt, v1);
    RJS_BI     *src2 = bi_get(rt, v2);
    RJS_BI     *dst  = big_int_new(rt, rv);
    int         pos1 = bi_positive(src1);
    int         pos2 = bi_positive(src2);
    RJS_BIMap   emap;
    RJS_BI     *pbi;
    size_t      i;
    uint32_t    e;
    RJS_Result  r;

    rjs_vector_init(&emap);

    if (pos2 < 0) {
        r = rjs_throw_range_error(rt, _("exponent cannot < 0"));
        goto end;
    }

    if (pos2 == 0) {
        r = bi_set_item(rt, dst, 0, 1);
        goto end;
    }

    if (pos1 == 0) {
        r = RJS_OK;
        goto end;
    }

    if (bi_size(src2) > 1) {
        r = rjs_throw_range_error(rt, _("exponent is too big"));
        goto end;
    }

    e = src2->n[0];

    if ((bi_size(src1) == 1) && (src1->n[0] == 1)) {
        if ((r = bi_set_item(rt, dst, 0, 1)) == RJS_ERR)
            goto end;
    } else if (e == 1) {
        if ((r = bi_dup(rt, dst, src1)) == RJS_ERR)
            goto end;
    } else {
        bi_map_resize(rt, &emap, 1);
        pbi = emap.items;
        if ((r = bi_dup(rt, pbi, src1)) == RJS_ERR)
            goto end;
        pbi->size = RJS_ABS(pbi->size);

        if ((r = bi_set_item(rt, dst, 0, 1)) == RJS_ERR)
            goto end;

        if ((r = bi_exp(rt, &emap, e, dst)) == RJS_ERR)
            goto end;
    }

    if ((pos1 < 0) && (e & 1))
        dst->size = - dst->size;
end:
    rjs_vector_foreach(&emap, i, pbi) {
        bi_deinit(rt, pbi);
    }
    rjs_vector_deinit(&emap, rt);
    return r;
}

/*Complement*/
static RJS_Result
bi_complement (RJS_Runtime *rt, RJS_BI *src, RJS_BI *dst)
{
    int        size = bi_size(src);
    int        i;
    RJS_Result r;

    for (i = 0; i < size; i ++) {
        if ((r = bi_set_item(rt, dst, i, ~src->n[i])) == RJS_ERR)
            return r;
    }

    if ((r = bi_self_add_int(rt, dst, 1)) == RJS_ERR)
        return r;

    bi_end(dst);
    return RJS_OK;
}

/*Shift.*/
static RJS_Result
bi_shift (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2, RJS_BI *dst, RJS_Bool rev)
{
    int         size1 = bi_size(src1);
    int         size2 = bi_size(src2);
    int         pos1  = bi_positive(src1);
    int         pos2  = bi_positive(src2);
    int         size, i, j, off, bits;
    RJS_BI      inp, res;
    RJS_BI     *s, *d;
    uint32_t    y;
    RJS_Result  r;

    bi_init(&inp);
    bi_init(&res);

    if (pos2 == 0) {
        r = bi_dup(rt, dst, src1);
        goto end;
    }

    if (rev)
        pos2 = - pos2;

    if (size2 > 1) {
        if (pos2 < 0) {
            if (pos1 < 0) {
                if ((r = bi_set_item(rt, dst, 0, 1)) == RJS_ERR)
                    goto end;

                dst->size = - dst->size;
            }
            r = RJS_OK;
            goto end;
        } else {
            r = rjs_throw_range_error(rt, _("shift value is too big"));
            goto end;
        }
    }

    if (pos1 < 0) {
        if ((r = bi_complement(rt, src1, &inp)) == RJS_ERR)
            goto end;

        s = &inp;
        d = &res;
    } else {
        s = src1;
        d = dst;
    }

    y    = src2->n[0];
    off  = y / 32;
    bits = y % 32;

    if (pos2 < 0) {
        size = size1 - off;

        for (i = 0; i < size; i ++) {
            uint64_t v;

            j = i + off;
            v = s->n[j];

            if (j + 1 < size1) {
                v |= ((uint64_t)s->n[j + 1]) << 32;
            } else if (pos1 < 0) {
                v |= 0xffffffff00000000ull;
            }

            v >>= bits;

            if ((r = bi_set_item(rt, d, i, v & 0xffffffff)) == RJS_ERR)
                goto end;
        }
    } else {
        uint32_t left = 0;

        for (i = 0; i < off; i ++) {
            if ((r = bi_set_item(rt, d, i, 0)) == RJS_ERR)
                goto end;
        }

        for (j = 0; j < size1; j ++, i ++) {
            uint64_t v;

            v = s->n[j];
            v <<= bits;
            v |= left;

            if ((r = bi_set_item(rt, d, i, v & 0xffffffff)) == RJS_ERR)
                goto end;

            left = v >> 32;
        }

        if (left) {
            if (pos1 < 0)
                left |= 0xffffffff << bits;

            if ((r = bi_set_item(rt, d, i, left)) == RJS_ERR)
                goto end;
        }
    }

    bi_end(d);

    if (pos1 < 0) {
        if ((r = bi_complement(rt, d, dst)) == RJS_ERR)
            goto end;

        dst->size = - dst->size;
    }

    r = RJS_OK;
end:
    bi_deinit(rt, &inp);
    bi_deinit(rt, &res);
    return r;
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
    RJS_BI *src1  = bi_get(rt, v1);
    RJS_BI *src2  = bi_get(rt, v2);
    RJS_BI *dst   = big_int_new(rt, rv);

    return bi_shift(rt, src1, src2, dst, RJS_FALSE);
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
    RJS_BI *src1  = bi_get(rt, v1);
    RJS_BI *src2  = bi_get(rt, v2);
    RJS_BI *dst   = big_int_new(rt, rv);

    return bi_shift(rt, src1, src2, dst, RJS_TRUE);
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

#define BIT_AND 0
#define BIT_OR  1
#define BIT_XOR 2

/*Bitwise operation.*/
static RJS_Result
bi_bit_op (RJS_Runtime *rt, RJS_BI *src1, RJS_BI *src2, RJS_BI *dst, int op)
{
    RJS_BI     t1, t2, res;
    int        pos1, pos2, pos, size1, size2, size, i;
    RJS_Result r;

    pos1  = bi_positive(src1);
    pos2  = bi_positive(src2);
    size1 = bi_size(src1);
    size2 = bi_size(src2);

    bi_init(&t1);
    bi_init(&t2);
    bi_init(&res);

    if (pos1 < 0) {
        if ((r = bi_complement(rt, src1, &t1)) == RJS_ERR)
            goto end;

        if ((r = bi_set_item(rt, &t1, size1, 0xffffffff)) == RJS_ERR)
            goto end;
    } else {
        if ((r = bi_dup(rt, &t1, src1)) == RJS_ERR)
            goto end;

        if ((r = bi_set_item(rt, &t1, size1, 0)) == RJS_ERR)
            goto end;
    }

    if (pos2 < 0) {
        if ((r = bi_complement(rt, src2, &t2)) == RJS_ERR)
            goto end;

        if ((r = bi_set_item(rt, &t2, size2, 0xffffffff)) == RJS_ERR)
            goto end;
    } else {
        if ((r = bi_dup(rt, &t2, src2)) == RJS_ERR)
            goto end;

        if ((r = bi_set_item(rt, &t2, size2, 0)) == RJS_ERR)
            goto end;
    }

    size1 ++;
    size2 ++;
    size = RJS_MAX(size1, size2);

    for (i = 0; i < size; i ++) {
        uint32_t v1, v2, vr;

        if (i < size1)
            v1 = t1.n[i];
        else
            v1 = (pos1 < 0) ? 0xffffffff : 0;

        if (i < size2)
            v2 = t2.n[i];
        else
            v2 = (pos2 < 0) ? 0xffffffff : 0;

        switch (op) {
        case BIT_AND:
            vr = v1 & v2;
            break;
        case BIT_OR:
            vr = v1 | v2;
            break;
        default:
            vr = v1 ^ v2;
            break;
        }

        if ((r = bi_set_item(rt, &res, i, vr)) == RJS_ERR)
            goto end;
    }

    pos = 1;

    if (size && (res.n[size - 1] & 0x80000000))
        pos = -1;

    bi_end(&res);

    if (pos < 0) {
        if ((r = bi_complement(rt, &res, dst)) == RJS_ERR)
            goto end;

        dst->size = - dst->size;
    } else {
        if ((r = bi_dup(rt, dst, &res)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    bi_deinit(rt, &t1);
    bi_deinit(rt, &t2);
    bi_deinit(rt, &res);
    return r;
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
    RJS_BI *src1 = bi_get(rt, v1);
    RJS_BI *src2 = bi_get(rt, v2);
    RJS_BI *dst  = big_int_new(rt, rv);

    return bi_bit_op(rt, src1, src2, dst, BIT_AND);
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
    RJS_BI *src1 = bi_get(rt, v1);
    RJS_BI *src2 = bi_get(rt, v2);
    RJS_BI *dst  = big_int_new(rt, rv);

    return bi_bit_op(rt, src1, src2, dst, BIT_XOR);
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
    RJS_BI *src1 = bi_get(rt, v1);
    RJS_BI *src2 = bi_get(rt, v2);
    RJS_BI *dst  = big_int_new(rt, rv);

    return bi_bit_op(rt, src1, src2, dst, BIT_OR);
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
    RJS_BI *bi1 = bi_get(rt, v1);
    RJS_BI *bi2 = bi_get(rt, v2);

    return bi_compare(bi1, bi2);
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
    RJS_BI    *bi1 = bi_get(rt, v);
    RJS_BI     tmp;
    RJS_BI    *bi2 = &tmp;
    RJS_Number i;
    RJS_Result r;

    if (isnan(n))
        return RJS_COMPARE_UNDEFINED;

    if (isinf(n)) {
        if (n < 0)
            return RJS_COMPARE_GREATER;
        else
            return RJS_COMPARE_LESS;
    }

    i = trunc(n);

    bi_init(bi2);

    if ((r = bi_set_number(rt, bi2, i)) == RJS_OK) {
        r = bi_compare(bi1, bi2);
    }

    bi_deinit(rt, bi2);

    if (r == RJS_COMPARE_EQUAL) {
        if (i < n)
            r = RJS_COMPARE_LESS;
        else if (i > n)
            r = RJS_COMPARE_GREATER;
    }

    return r;
}

/*Get the bits from another big integer.*/
static RJS_Result
bi_bits (RJS_Runtime *rt, RJS_BI *src, RJS_BI *dst, int64_t bits)
{
    RJS_BI     tmp;
    RJS_BI    *inp;
    int        size;
    int64_t    off, b, i;
    uint32_t   n;
    RJS_Result r;

    bi_init(&tmp);

    if (bi_positive(src) < 0) {
        inp = &tmp;

        if ((r = bi_complement(rt, src, inp)) == RJS_ERR)
            goto end;
    } else {
        inp = src;
    }

    size = bi_size(inp);

    off = bits / 32;
    b   = bits % 32;

    for (i = 0; i < off; i ++) {
        if (i < size)
            n = inp->n[i];
        else
            n = 0;

        if ((r = bi_set_item(rt, dst, i, n)) == RJS_ERR)
            goto end;
    }

    if (b) {
        if (i < size)
            n = inp->n[i];
        else
            n = 0;

        n &= ~(0xffffffff << b);

        if ((r = bi_set_item(rt, dst, i, n)) == RJS_ERR)
            goto end;
    }

    bi_end(dst);

    r = RJS_OK;
end:
    bi_deinit(rt, &tmp);
    return r;
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
    RJS_BI    *src  = bi_get(rt, v);
    RJS_BI    *dst  = big_int_new(rt, rv);
    int        pos  = 1;
    int        size;
    int64_t    off, b;
    uint32_t   n;
    RJS_BI     res;
    RJS_Result r;

    bi_init(&res);

    if ((r = bi_bits(rt, src, &res, bits)) == RJS_ERR)
        goto end;

    off  = (bits - 1) / 32;
    b    = (bits - 1) % 32;
    size = bi_size(&res);

    if (size > off) {
        n = res.n[off];
        if (n & (1 << b)) {
            n |= (0xffffffff << b);
            bi_set_item(rt, &res, off, n);
            pos = -1;
        }
    }

    if (pos < 0) {
        if ((r = bi_complement(rt, &res, dst)) == RJS_ERR)
            goto end;

        dst->size = - dst->size;
    } else {
        if ((r = bi_dup(rt, dst, &res)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    bi_deinit(rt, &res);
    return r;
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
    RJS_BI *src = bi_get(rt, v);
    RJS_BI *dst = big_int_new(rt, rv);

    return bi_bits(rt, src, dst, bits);
}
