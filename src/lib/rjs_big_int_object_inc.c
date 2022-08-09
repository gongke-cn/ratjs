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

/*BigInt*/
static RJS_NF(BigInt_constructor)
{
    RJS_Value *v    = rjs_argument_get(rt, args, argc, 0);
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *prim = rjs_value_stack_push(rt);
    RJS_Result r;

    if (nt) {
        r = rjs_throw_type_error(rt, _("\"BigInt\" cannot be used as a constructor"));
        goto end;
    }

    if ((r = rjs_to_primitive(rt, v, prim, RJS_VALUE_NUMBER)) == RJS_ERR)
        goto end;

    if (rjs_value_is_number(rt, prim)) {
        RJS_Number n = rjs_value_get_number(rt, prim);

        r = rjs_number_to_big_int(rt, n, rv);
    } else {
        r = rjs_to_big_int(rt, prim, rv);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
big_int_constructor_desc = {
    "BigInt",
    1,
    BigInt_constructor
};

/*BigInt.asIntN*/
static RJS_NF(BigInt_asIntN)
{
    RJS_Value   *bitsv = rjs_argument_get(rt, args, argc, 0);
    RJS_Value   *nv    = rjs_argument_get(rt, args, argc, 1);
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *bi    = rjs_value_stack_push(rt);
    RJS_Result   r;
    int64_t      bits;

    if ((r = rjs_to_index(rt, bitsv, &bits)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_big_int(rt, nv, bi)) == RJS_ERR)
        goto end;

    r = rjs_big_int_as_int_n(rt, bits, bi, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*BigInt.asUintN*/
static RJS_NF(BigInt_asUintN)
{
    RJS_Value   *bitsv = rjs_argument_get(rt, args, argc, 0);
    RJS_Value   *nv    = rjs_argument_get(rt, args, argc, 1);
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *bi    = rjs_value_stack_push(rt);
    RJS_Result   r;
    int64_t      bits;

    if ((r = rjs_to_index(rt, bitsv, &bits)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_big_int(rt, nv, bi)) == RJS_ERR)
        goto end;

    r = rjs_big_int_as_uint_n(rt, bits, bi, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
big_int_function_descs[] = {
    {
        "asIntN",
        2,
        BigInt_asIntN
    },
    {
        "asUintN",
        2,
        BigInt_asUintN
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
big_int_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "BigInt",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Get this big integer value pointer.*/
static RJS_Value*
this_big_int_value (RJS_Runtime *rt, RJS_Value *v)
{
    if (rjs_value_is_big_int(rt, v))
        return v;

    if (rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_PRIMITIVE) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, v);

        if (rjs_value_is_big_int(rt, &po->value))
            return &po->value;
    }

    rjs_throw_type_error(rt, _("the value is not a big integer"));
    return NULL;
}

/*BigInt.prototype.toString*/
static RJS_NF(BigInt_prototype_toString)
{
    RJS_Value *radixv = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *bi;
    int        radix;
    RJS_Result r;

    if (!(bi = this_big_int_value(rt, thiz))) {
        r = RJS_ERR;
        goto end;
    }

    if (rjs_value_is_undefined(rt, radixv)) {
        radix = 10;
    } else {
        RJS_Number n;

        if ((r = rjs_to_integer_or_infinity(rt, radixv, &n)) == RJS_ERR)
            goto end;

        if ((n < 2) || (n > 36)) {
            r = rjs_throw_range_error(rt, _("radix must in range 2 ~ 36"));
            goto end;
        }

        radix = n;
    }

    r = rjs_big_int_to_string(rt, bi, radix, rv);
end:
    return r;
}

/*BigInt.prototype.valueOf*/
static RJS_NF(BigInt_prototype_valueOf)
{
    RJS_Value *bi;

    if (!(bi = this_big_int_value(rt, thiz)))
        return RJS_ERR;

    rjs_value_copy(rt, rv, bi);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
big_int_prototype_function_descs[] = {
    {
        "toLocaleString",
        0,
        BigInt_prototype_toString
    },
    {
        "toString",
        0,
        BigInt_prototype_toString
    },
    {
        "valueOf",
        0,
        BigInt_prototype_valueOf
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
big_int_prototype_desc = {
    "BigInt",
    NULL,
    NULL,
    NULL,
    big_int_prototype_field_descs,
    big_int_prototype_function_descs,
    NULL,
    NULL,
    "BigInt_prototype"
};
