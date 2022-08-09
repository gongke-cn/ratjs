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

#define MAX_SAFE_INTEGER 9007199254740991
#define MIN_SAFE_INTEGER -9007199254740991

/*Number*/
static RJS_NF(Number_constructor)
{
    RJS_Value *v     = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *prim  = rjs_value_stack_push(rt);
    RJS_Value *nv    = rjs_value_stack_push(rt);
    RJS_Number n;
    RJS_Result r;

    if (argc > 0) {
        if ((r = rjs_to_numeric(rt, v, prim)) == RJS_ERR)
            goto end;

#if ENABLE_BIG_INT
        if (rjs_value_is_big_int(rt, prim)) {
            int64_t i;

            if ((r = rjs_big_int_to_int64(rt, prim, &i)) == RJS_ERR)
                goto end;

            n = i;
        } else
#endif /*ENABLE_BIG_INT*/
        {
            n = rjs_value_get_number(rt, prim);
        }
    } else {
        n = 0;
    }

    if (!nt) {
        rjs_value_set_number(rt, rv, n);
    } else {
        rjs_value_set_number(rt, nv, n);

        if ((r = rjs_primitive_object_new(rt, rv, nt, RJS_O_Number_prototype, nv)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
number_constructor_desc = {
    "Number",
    1,
    Number_constructor
};

static const RJS_BuiltinFieldDesc
number_field_descs[] = {
    {
        "EPSILON",
        RJS_VALUE_NUMBER,
        2.2204460492503130808472633361816e-16
    },
    {
        "MAX_SAFE_INTEGER",
        RJS_VALUE_NUMBER,
        MAX_SAFE_INTEGER
    },
    {
        "MIN_SAFE_INTEGER",
        RJS_VALUE_NUMBER,
        MIN_SAFE_INTEGER
    },
    {
        "MAX_VALUE",
        RJS_VALUE_NUMBER,
        1.7976931348623157e+308
    },
    {
        "MIN_VALUE",
        RJS_VALUE_NUMBER,
        5e-324
    },
    {
        "NaN",
        RJS_VALUE_NUMBER,
        NAN
    },
    {
        "NEGATIVE_INFINITY",
        RJS_VALUE_NUMBER,
        -INFINITY
    },
    {
        "POSITIVE_INFINITY",
        RJS_VALUE_NUMBER,
        INFINITY
    },
    {NULL}
};

/*Number.isFinite*/
static RJS_NF(Number_isFinite)
{
    RJS_Value *nv = rjs_argument_get(rt, args, argc, 0);
    RJS_Bool   b;

    if (!rjs_value_is_number(rt, nv)) {
        b = RJS_FALSE;
    } else {
        RJS_Number n = rjs_value_get_number(rt, nv);

        b = !(isinf(n) || isnan(n));
    }

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

/*Number.isInteger*/
static RJS_NF(Number_isInteger)
{
    RJS_Value *nv = rjs_argument_get(rt, args, argc, 0);
    RJS_Bool   b;

    if (!rjs_value_is_number(rt, nv))
        b = RJS_FALSE;
    else
        b = rjs_is_integral_number(rjs_value_get_number(rt, nv));

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

/*Number.isNaN*/
static RJS_NF(Number_isNaN)
{
    RJS_Value *nv = rjs_argument_get(rt, args, argc, 0);
    RJS_Bool   b;

    if (!rjs_value_is_number(rt, nv)) {
        b = RJS_FALSE;
    } else {
        RJS_Number n = rjs_value_get_number(rt, nv);

        b = isnan(n);
    }

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

/*Number.isSafeInteger*/
static RJS_NF(Number_isSafeInteger)
{
    RJS_Value *nv = rjs_argument_get(rt, args, argc, 0);
    RJS_Bool   b;

    if (!rjs_value_is_number(rt, nv)) {
        b = RJS_FALSE;
    } else {
        RJS_Number n = rjs_value_get_number(rt, nv);

        if (isinf(n) || isnan(n))
            b = RJS_FALSE;
        else if (trunc(n) != n)
            b = RJS_FALSE;
        else
            b = (fabs(n) <= MAX_SAFE_INTEGER);
    }

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
number_function_descs[] = {
    {
        "isFinite",
        1,
        Number_isFinite
    },
    {
        "isInteger",
        1,
        Number_isInteger
    },
    {
        "isNaN",
        1,
        Number_isNaN
    },
    {
        "isSafeInteger",
        1,
        Number_isSafeInteger
    },
    {
        "parseFloat",
        1,
        NULL,
        "parseFloat"
    },
    {
        "parseInt",
        2,
        NULL,
        "parseInt"
    },
    {NULL}
};

/*Get this number value.*/
static RJS_Result
this_number_value (RJS_Runtime *rt, RJS_Value *thiz, RJS_Number *pn)
{
    RJS_Number n;

    if (rjs_value_is_number(rt, thiz)) {
        n = rjs_value_get_number(rt, thiz);
    } else if (rjs_value_is_object(rt, thiz)
            && (rjs_value_get_gc_thing_type(rt, thiz) == RJS_GC_THING_PRIMITIVE)) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, thiz);

        if (!rjs_value_is_number(rt, &po->value))
            return rjs_throw_type_error(rt, _("this is not a number value"));

        n = rjs_value_get_number(rt, &po->value);
    } else {
        return rjs_throw_type_error(rt, _("this is not a number value"));
    }

    *pn = n;
    return RJS_OK;
}

/*Number.prototype.toExponential*/
static RJS_NF(Number_prototype_toExponential)
{
    RJS_Value *fv = rjs_argument_get(rt, args, argc, 0);
    char      *sb = NULL;
    RJS_Number n  = 0, fn;
    int        frac;
    RJS_Result r;
    char      *se, *sc, *dc;
    int        sign, decpt, nv, i, v, left, ndigits, mode;
    char       sbuf[256];

    if ((r = this_number_value(rt, thiz, &n)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, fv, &fn)) == RJS_ERR)
        goto end;

    if (isinf(n) || isnan(n)) {
        r = rjs_number_to_string(rt, n, rv);
        goto end;
    }

    if ((fn < 0) || (fn > 100)) {
        r = rjs_throw_range_error(rt, _("fraction must >= 0 and <= 100"));
        goto end;
    }

    if (!rjs_value_is_undefined(rt, fv)) {
        mode    = 2;
        ndigits = fn + 1;
    } else {
        mode    = 0;
        ndigits = 0;
    }

    sb = rjs_dtoa(n, mode, ndigits, &decpt, &sign, &se);

    sc = sb;
    dc = sbuf;
    nv = decpt;

    if (!rjs_value_is_undefined(rt, fv))
        frac = fn;
    else
        frac = (se - sb) - 1;

    if (sign && (n != 0))
        *dc ++ = '-';

    *dc ++ = *sc ++;

    if (frac > 0) {
        *dc ++ = '.';
        for (i = 0; i < frac; i ++)
            *dc ++ = (sc >= se) ? '0' : *sc ++;
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

    *dc = 0;

    rjs_string_from_chars(rt, rv, sbuf, -1);

    r = RJS_OK;
end:
    if (sb)
        rjs_freedtoa(sb);

    return r;
}

/*Number.prototype.toFixed*/
static RJS_NF(Number_prototype_toFixed)
{
    RJS_Value *fv = rjs_argument_get(rt, args, argc, 0);
    char      *sb = NULL;
    RJS_Number n  = 0, fn;
    int        frac;
    RJS_Result r;
    char      *se, *sc, *dc;
    int        sign, decpt, nv, kv, i;
    char       sbuf[256];

    if ((r = this_number_value(rt, thiz, &n)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, fv, &fn)) == RJS_ERR)
        goto end;

    if (isinf(fn)) {
        r = rjs_throw_range_error(rt, _("fraction is infinite"));
        goto end;
    }

    if ((fn < 0) || (fn > 100)) {
        r = rjs_throw_range_error(rt, _("fraction must >= 0 and <= 100"));
        goto end;
    }

    if (isinf(n) || isnan(n) || (fabs(n) >= 1e21)) {
        r = rjs_number_to_string(rt, n, rv);
        goto end;
    }

    frac = fn;

    sb = rjs_dtoa(n, 3, frac, &decpt, &sign, &se);
    sc = sb;
    dc = sbuf;

    kv = se - sb;
    nv = decpt;

    if (sign)
        *dc ++ = '-';

    if (kv <= nv) {
        if (kv) {
            for (i = 0; i < kv; i ++)
                *dc ++ = *sc ++;
        } else {
            *dc ++ = '0';
            i = 1;
        }

        for (; i < nv; i ++)
            *dc ++ = '0';
        if (frac > 0)
            *dc ++ = '.';
    } else if (nv > 0) {
        for (i = 0; i < nv; i ++)
            *dc ++ = *sc ++;

        if (frac > 0) {
            *dc ++ = '.';

            for (; i < kv; i ++) {
                if (frac <= 0)
                    break;
                *dc ++ = *sc ++;
                frac --;
            }
        }
    } else {
        *dc ++ = '0';

        if (frac > 0) {
            *dc ++ = '.';
            for (i = 0; i < -nv; i ++) {
                if (frac <= 0)
                    break;
                *dc ++ = '0';
                frac --;
            }
            for (i = 0; i < kv; i ++) {
                if (frac <= 0)
                    break;
                *dc ++ = *sc ++;
                frac --;
            }
        }
    }

    while (frac > 0) {
        *dc ++ = '0';
        frac --;
    }

    *dc = 0;

    rjs_string_from_chars(rt, rv, sbuf, -1);

    r = RJS_OK;
end:
    if (sb)
        rjs_freedtoa(sb);

    return r;
}

/*Number.prototype.toLocaleString*/
static RJS_NF(Number_prototype_toLocaleString)
{
    RJS_Number n;
    RJS_Result r;

    if ((r = this_number_value(rt, thiz, &n)) == RJS_ERR)
        goto end;

    r = rjs_number_to_string(rt, n, rv);
end:
    return r;
}

/*Number.prototype.toPrecision*/
static RJS_NF(Number_prototype_toPrecision)
{
    RJS_Value *precv = rjs_argument_get(rt, args, argc, 0);
    char      *sb    = NULL;
    RJS_Number n, pn;
    int        prec;
    RJS_Result r;
    char      *se, *sc, *dc;
    int        sign, decpt, nv, ev, i;
    char       sbuf[256];

    if ((r = this_number_value(rt, thiz, &n)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, precv)) {
        r = rjs_number_to_string(rt, n, rv);
        goto end;
    }

    if ((r = rjs_to_integer_or_infinity(rt, precv, &pn)) == RJS_ERR)
        goto end;

    if (isinf(n) || isnan(n)) {
        r = rjs_number_to_string(rt, n, rv);
        goto end;
    }

    if ((pn < 1) || (pn > 100)) {
        r = rjs_throw_range_error(rt, _("precision must >= 1 and <= 100"));
        goto end;
    }

    prec = pn;

    sb = rjs_dtoa(n, 2, prec, &decpt, &sign, &se);
    sc = sb;
    dc = sbuf;

    if (sign && (n != 0))
        *dc ++ = '-';

    nv = decpt;
    ev = nv - 1;

    if ((ev < -6) || (ev >= prec)) {
        int v, left;

        *dc ++ = *sc ++;

        if (prec != 1) {
            *dc ++ = '.';
            for (i = 0; i < prec - 1; i ++)
                *dc ++ = (sc >= se) ? '0' : *sc ++;
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
    } else if (ev == prec - 1) {
        for (i = 0; i < prec; i ++)
            *dc ++ = (sc >= se) ? '0' : *sc ++;
    } else if (ev >= 0) {
        for (i = 0; i < ev + 1; i ++)
            *dc ++ = (sc >= se) ? '0' : *sc ++;

        *dc ++ = '.';

        for (i = 0; i < prec - ev - 1; i ++)
            *dc ++ = (sc >= se) ? '0' : *sc ++;
    } else {
        *dc ++ = '0';
        *dc ++ = '.';
        for (i = 0; i < - (ev + 1); i ++)
            *dc ++ = '0';
        for (i = 0; i < prec; i ++)
            *dc ++ = (sc >= se) ? '0' : *sc ++;
    }

    *dc = 0;

    rjs_string_from_chars(rt, rv, sbuf, -1);

    r = RJS_OK;
end:
    if (sb)
        rjs_freedtoa(sb);

    return r;
}

/*Number.prototype.toString*/
static RJS_NF(Number_prototype_toString)
{
    RJS_Value *radixv = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n, rn;
    int64_t    i;
    int        radix;
    RJS_Result r;

    if ((r = this_number_value(rt, thiz, &n)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, radixv)) {
        radix = 10;
    } else {
        if ((r = rjs_to_integer_or_infinity(rt, radixv, &rn)) == RJS_ERR)
            goto end;

        if ((rn < 2) || (rn > 36)) {
            r = rjs_throw_range_error(rt, _("radix must >= 2 and <= 36"));
            goto end;
        }

        radix = rn;
    }

    if (radix == 10) {
        r = rjs_number_to_string(rt, n, rv);
        goto end;
    }

    i = n;
    if (n == (RJS_Number)i) {
        char    buf[256];
        char   *c;
        int     sign;
        int64_t left;

        if (i < 0) {
            sign = 1;
            i    = -i;
        } else {
            sign = 0;
        }

        c = buf + RJS_N_ELEM(buf) - 1;
        *c -- = 0;

        left = i;

        while (1) {
            int n;
            int end;

            if (left < radix) {
                n   = left;
                end = 1;
            } else {
                n     = left % radix;
                left /= radix;
                end   = 0;
            }

            if (n < 10)
                *c = n + '0';
            else
                *c = n - 10 + 'a';

            c --;

            if (end)
                break;
        }

        if (sign)
            *c -- = '-';

        r = rjs_string_from_chars(rt, rv, c + 1, -1);
            goto end;
    }

    r = rjs_number_to_string(rt, n, rv);
end:
    return r;
}

/*Number.prototype.valueOf*/
static RJS_NF(Number_prototype_valueOf)
{
    RJS_Number n;
    RJS_Result r;

    if ((r = this_number_value(rt, thiz, &n)) == RJS_ERR)
        return r;

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
number_prototype_function_descs[] = {
    {
        "toExponential",
        1,
        Number_prototype_toExponential
    },
    {
        "toFixed",
        1,
        Number_prototype_toFixed
    },
    {
        "toLocaleString",
        0,
        Number_prototype_toLocaleString
    },
    {
        "toPrecision",
        1,
        Number_prototype_toPrecision
    },
    {
        "toString",
        1,
        Number_prototype_toString
    },
    {
        "valueOf",
        0,
        Number_prototype_valueOf
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
number_prototype_desc = {
    "Number",
    NULL,
    NULL,
    NULL,
    NULL,
    number_prototype_function_descs,
    NULL,
    NULL,
    "Number_prototype"
};
