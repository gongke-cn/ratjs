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

static const RJS_BuiltinFieldDesc
math_field_descs[] = {
    {
        "E",
        RJS_VALUE_NUMBER,
        2.7182818284590452354
    },
    {
        "LN10",
        RJS_VALUE_NUMBER,
        2.302585092994046
    },
    {
        "LN2",
        RJS_VALUE_NUMBER,
        0.6931471805599453
    },
    {
        "LOG10E",
        RJS_VALUE_NUMBER,
        0.4342944819032518
    },
    {
        "LOG2E",
        RJS_VALUE_NUMBER,
        1.4426950408889634
    },
    {
        "PI",
        RJS_VALUE_NUMBER,
        3.1415926535897932
    },
    {
        "SQRT1_2",
        RJS_VALUE_NUMBER,
        0.7071067811865476
    },
    {
        "SQRT2",
        RJS_VALUE_NUMBER,
        1.4142135623730951
    },
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Math",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Number vector.*/
typedef RJS_VECTOR_DECL(RJS_Number) RJS_NumberVector;

/*Math.abs*/
static RJS_NF(Math_abs)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = fabs(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.acos*/
static RJS_NF(Math_acos)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = acos(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.acosh*/
static RJS_NF(Math_acosh)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = acosh(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.asin*/
static RJS_NF(Math_asin)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = asin(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.asinh*/
static RJS_NF(Math_asinh)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = asinh(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.atan*/
static RJS_NF(Math_atan)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = atan(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.atanh*/
static RJS_NF(Math_atanh)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = atanh(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.atan2*/
static RJS_NF(Math_atan2)
{
    RJS_Value *y = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *x = rjs_argument_get(rt, args, argc, 1);
    RJS_Number ny, nx, n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, y, &ny)) == RJS_ERR)
        return r;

    if ((r = rjs_to_number(rt, x, &nx)) == RJS_ERR)
        return r;

    n = atan2(ny, nx);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.cbrt*/
static RJS_NF(Math_cbrt)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = cbrt(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.ceil*/
static RJS_NF(Math_ceil)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = ceil(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.clz32*/
static RJS_NF(Math_clz32)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    int        v = 0;
    uint32_t   n, mask;
    RJS_Result r;

    if ((r = rjs_to_uint32(rt, x, &n)) == RJS_ERR)
        return r;

    mask = 0x80000000;

    while (mask) {
        if (n & mask)
            break;

        v ++;
        mask >>= 1;
    }

    rjs_value_set_number(rt, rv, v);
    return RJS_OK;
}

/*Math.cos*/
static RJS_NF(Math_cos)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = cos(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.cosh*/
static RJS_NF(Math_cosh)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = cosh(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.exp*/
static RJS_NF(Math_exp)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = exp(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.expm1*/
static RJS_NF(Math_expm1)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = expm1(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.floor*/
static RJS_NF(Math_floor)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = floor(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.fround*/
static RJS_NF(Math_fround)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = (float)n;

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.hypot*/
static RJS_NF(Math_hypot)
{
    RJS_Bool         only_0 = RJS_TRUE;
    RJS_Number       rn     = 0;
    RJS_NumberVector nv;
    size_t           i;
    RJS_Result       r;

    rjs_vector_init(&nv);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);
        RJS_Number n;

        if ((r = rjs_to_number(rt, arg, &n)) == RJS_ERR)
            return r;

        rjs_vector_append(&nv, n, rt);
    }

    for (i = 0; i < nv.item_num; i ++) {
        RJS_Number n = nv.items[i];

        if (isinf(n)) {
            rjs_value_set_number(rt, rv, INFINITY);
            r = RJS_OK;
            goto end;
        }

        if (n != 0)
            only_0 = RJS_FALSE;

        rn += n * n;
    }

    if (only_0) {
        rn = 0;
    } else {
        rn = sqrt(rn);
    }

    rjs_value_set_number(rt, rv, rn);
    r = RJS_OK;
end:
    rjs_vector_deinit(&nv, rt);
    return r;
}

/*Math.imul*/
static RJS_NF(Math_imul)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *y = rjs_argument_get(rt, args, argc, 1);
    uint32_t   a, b;
    int32_t    c;
    RJS_Result r;

    if ((r = rjs_to_uint32(rt, x, &a)) == RJS_ERR)
        return r;

    if ((r = rjs_to_uint32(rt, y, &b)) == RJS_ERR)
        return r;

    c = a * b;

    rjs_value_set_number(rt, rv, c);
    return RJS_OK;
}

/*Math.log*/
static RJS_NF(Math_log)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = log(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.log1p*/
static RJS_NF(Math_log1p)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = log1p(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.log10*/
static RJS_NF(Math_log10)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = log10(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.log2*/
static RJS_NF(Math_log2)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = log2(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.max*/
static RJS_NF(Math_max)
{
    RJS_Result       r;
    size_t           i;
    RJS_NumberVector nv;
    RJS_Number       max = -INFINITY;

    rjs_vector_init(&nv);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);
        RJS_Number n;

        if ((r = rjs_to_number(rt, arg, &n)) == RJS_ERR)
            goto end;

        rjs_vector_append(&nv, n, rt);
    }

    for (i = 0; i < nv.item_num; i ++) {
        RJS_Number n = nv.items[i];

        if (isnan(n)) {
            rjs_value_set_number(rt, rv, NAN);
            r = RJS_OK;
            goto end;
        }

        if ((n == 0) && (max == 0) && !signbit(n) && signbit(max))
            max = 0.;
        else if (n > max)
            max = n;
    }

    rjs_value_set_number(rt, rv, max);
    r = RJS_OK;
end:
    rjs_vector_deinit(&nv, rt);
    return r;
}

/*Math.min*/
static RJS_NF(Math_min)
{
    RJS_Result       r;
    size_t           i;
    RJS_NumberVector nv;
    RJS_Number       min = INFINITY;

    rjs_vector_init(&nv);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);
        RJS_Number n;

        if ((r = rjs_to_number(rt, arg, &n)) == RJS_ERR)
            goto end;

        rjs_vector_append(&nv, n, rt);
    }

    for (i = 0; i < nv.item_num; i ++) {
        RJS_Number n = nv.items[i];

        if (isnan(n)) {
            rjs_value_set_number(rt, rv, NAN);
            r = RJS_OK;
            goto end;
        }

        if ((n == 0) && (min == 0) && signbit(n) && !signbit(min))
            min = -0.;
        else if (n < min)
            min = n;
    }

    rjs_value_set_number(rt, rv, min);
    r = RJS_OK;
end:
    rjs_vector_deinit(&nv, rt);
    return r;
}

/*Math.pow*/
static RJS_NF(Math_pow)
{
    RJS_Value *base = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *exp  = rjs_argument_get(rt, args, argc, 1);
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *bv   = rjs_value_stack_push(rt);
    RJS_Value *ev   = rjs_value_stack_push(rt);
    RJS_Number bn, en;
    RJS_Result r;

    if ((r = rjs_to_number(rt, base, &bn)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, bv, bn);

    if ((r = rjs_to_number(rt, exp, &en)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, ev, en);

    rjs_number_exponentiate(rt, bv, ev, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Math.random*/
static RJS_NF(Math_random)
{
    RJS_Number n;

    n = ((RJS_Number)rand()) / RAND_MAX;

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.round*/
static RJS_NF(Math_round)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n, i;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    i = floor(n);

    if (!isinf(n) && !isnan(n) && (i != n)) {
        if (!signbit(n) && (n < 0.5) && (n >= 0)) {
            n = +0.;
        } else if (signbit(n) && (n <= 0) && (n >= -0.5)) {
            n = -0.;
        } else {
            if (n - i < 0.5) {
                n = i;
            } else {
                n = i + 1;
            }
        }
    }

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.sign*/
static RJS_NF(Math_sign)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || (n == 0)) {
    } else if (n < 0) {
        n = -1;
    } else {
        n = 1;
    }

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.sin*/
static RJS_NF(Math_sin)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = sin(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.sinh*/
static RJS_NF(Math_sinh)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = sinh(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.sqrt*/
static RJS_NF(Math_sqrt)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = sqrt(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.tan*/
static RJS_NF(Math_tan)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = tan(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.tanh*/
static RJS_NF(Math_tanh)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = tanh(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/*Math.trunc*/
static RJS_NF(Math_trunc)
{
    RJS_Value *x = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, x, &n)) == RJS_ERR)
        return r;

    n = trunc(n);

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
math_function_descs[] = {
    {
        "abs",
        1,
        Math_abs
    },
    {
        "acos",
        1,
        Math_acos
    },
    {
        "acosh",
        1,
        Math_acosh
    },
    {
        "asin",
        1,
        Math_asin
    },
    {
        "asinh",
        1,
        Math_asinh
    },
    {
        "atan",
        1,
        Math_atan
    },
    {
        "atanh",
        1,
        Math_atanh
    },
    {
        "atan2",
        2,
        Math_atan2
    },
    {
        "cbrt",
        1,
        Math_cbrt
    },
    {
        "ceil",
        1,
        Math_ceil
    },
    {
        "clz32",
        1,
        Math_clz32
    },
    {
        "cos",
        1,
        Math_cos
    },
    {
        "cosh",
        1,
        Math_cosh
    },
    {
        "exp",
        1,
        Math_exp
    },
    {
        "expm1",
        1,
        Math_expm1
    },
    {
        "floor",
        1,
        Math_floor
    },
    {
        "fround",
        1,
        Math_fround
    },
    {
        "hypot",
        2,
        Math_hypot
    },
    {
        "imul",
        2,
        Math_imul
    },
    {
        "log",
        1,
        Math_log
    },
    {
        "log1p",
        1,
        Math_log1p
    },
    {
        "log10",
        1,
        Math_log10
    },
    {
        "log2",
        1,
        Math_log2
    },
    {
        "max",
        2,
        Math_max
    },
    {
        "min",
        2,
        Math_min
    },
    {
        "pow",
        2,
        Math_pow
    },
    {
        "random",
        0,
        Math_random
    },
    {
        "round",
        1,
        Math_round
    },
    {
        "sign",
        1,
        Math_sign
    },
    {
        "sin",
        1,
        Math_sin
    },
    {
        "sinh",
        1,
        Math_sinh
    },
    {
        "sqrt",
        1,
        Math_sqrt
    },
    {
        "tan",
        1,
        Math_tan
    },
    {
        "tanh",
        1,
        Math_tanh
    },
    {
        "trunc",
        1,
        Math_trunc
    },
    {NULL}
};
