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

/*Symbol*/
static RJS_NF(Symbol_constructor)
{
    RJS_Value *desc = rjs_argument_get(rt, args, argc, 0);
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *dstr = rjs_value_stack_push(rt);
    RJS_Result r;

    if (nt) {
        r = rjs_throw_type_error(rt, _("\"Symbol\" cannot be used as a constructor"));
        goto end;
    }

    if (rjs_value_is_undefined(rt, desc))
        rjs_value_set_undefined(rt, dstr);
    else if ((r = rjs_to_string(rt, desc, dstr)) == RJS_ERR)
        goto end;

    r = rjs_symbol_new(rt, rv, dstr);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
symbol_constructor_desc = {
    "Symbol",
    0,
    Symbol_constructor
};

static const RJS_BuiltinFieldDesc
symbol_field_descs[] = {
    {
        "asyncIterator",
        RJS_VALUE_SYMBOL,
        0,
        "@@asyncIterator"
    },
    {
        "hasInstance",
        RJS_VALUE_SYMBOL,
        0,
        "@@hasInstance"
    },
    {
        "isConcatSpreadable",
        RJS_VALUE_SYMBOL,
        0,
        "@@isConcatSpreadable"
    },
    {
        "iterator",
        RJS_VALUE_SYMBOL,
        0,
        "@@iterator"
    },
    {
        "match",
        RJS_VALUE_SYMBOL,
        0,
        "@@match"
    },
    {
        "matchAll",
        RJS_VALUE_SYMBOL,
        0,
        "@@matchAll"
    },
    {
        "replace",
        RJS_VALUE_SYMBOL,
        0,
        "@@replace"
    },
    {
        "search",
        RJS_VALUE_SYMBOL,
        0,
        "@@search"
    },
    {
        "species",
        RJS_VALUE_SYMBOL,
        0,
        "@@species"
    },
    {
        "split",
        RJS_VALUE_SYMBOL,
        0,
        "@@split"
    },
    {
        "toPrimitive",
        RJS_VALUE_SYMBOL,
        0,
        "@@toPrimitive"
    },
    {
        "toStringTag",
        RJS_VALUE_SYMBOL,
        0,
        "@@toStringTag"
    },
    {
        "unscopables",
        RJS_VALUE_SYMBOL,
        0,
        "@@unscopables"
    },
    {NULL}
};

/*Symbol.for*/
static RJS_NF(Symbol_for)
{
    RJS_Value          *key  = rjs_argument_get(rt, args, argc, 0);
    size_t              top  = rjs_value_stack_save(rt);
    RJS_Value          *kstr = rjs_value_stack_push(rt);
    RJS_String         *s;
    RJS_SymbolRegistry *sr;
    RJS_HashEntry      *he, **phe;
    RJS_Result          r;

    if ((r = rjs_to_string(rt, key, kstr)) == RJS_ERR)
        goto end;

    rjs_string_to_property_key(rt, kstr);
    s = rjs_value_get_string(rt, kstr);

    r = rjs_hash_lookup(&rt->sym_reg_key_hash, s, &he, &phe, &rjs_hash_size_ops, rt);
    if (r) {
        sr = RJS_CONTAINER_OF(he, RJS_SymbolRegistry, key_he);
        rjs_value_copy(rt, rv, &sr->symbol);
    } else {
        RJS_Symbol *sym;

        if ((r = rjs_symbol_new(rt, rv, kstr)) == RJS_ERR)
            goto end;

        RJS_NEW(rt, sr);

        rjs_value_copy(rt, &sr->symbol, rv);
        rjs_value_copy(rt, &sr->key, kstr);

        rjs_hash_insert(&rt->sym_reg_key_hash, s, &sr->key_he, phe, &rjs_hash_size_ops, rt);

        sym = rjs_value_get_symbol(rt, rv);

        r = rjs_hash_lookup(&rt->sym_reg_sym_hash, sym, &he, &phe, &rjs_hash_size_ops, rt);
        assert(!r);
        rjs_hash_insert(&rt->sym_reg_sym_hash, sym, &sr->sym_he, phe, &rjs_hash_size_ops, rt);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Symbol.keyFor*/
static RJS_NF(Symbol_keyFor)
{
    RJS_Value     *sym = rjs_argument_get(rt, args, argc, 0);
    RJS_Symbol    *s;
    RJS_HashEntry *he;
    RJS_Result     r;

    if (!rjs_value_is_symbol(rt, sym))
        return rjs_throw_type_error(rt, _("the value is not a symbol"));

    s = rjs_value_get_symbol(rt, sym);
    r = rjs_hash_lookup(&rt->sym_reg_sym_hash, s, &he, NULL, &rjs_hash_size_ops, rt);
    if (r) {
        RJS_SymbolRegistry *sr;

        sr = RJS_CONTAINER_OF(he, RJS_SymbolRegistry, sym_he);

        rjs_value_copy(rt, rv, &sr->key);
    } else {
        rjs_value_set_undefined(rt, rv);
    }

    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
symbol_function_descs[] = {
    {
        "for",
        1,
        Symbol_for
    },
    {
        "keyFor",
        1,
        Symbol_keyFor
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
symbol_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Symbol",
        RJS_PROP_FL_CONFIGURABLE
    },
    {NULL}
};

/*Get this symbol value.*/
static RJS_Symbol*
this_symbol_value (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Symbol *s = NULL;

    if (rjs_value_is_symbol(rt, v)) {
        s = rjs_value_get_symbol(rt, v);
    } else if (rjs_value_is_object(rt, v)
            && (rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_PRIMITIVE)) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, v);

        if (!rjs_value_is_symbol(rt, &po->value))
            rjs_throw_type_error(rt, _("this is not a symbol value"));
        else
            s = rjs_value_get_symbol(rt, &po->value);
    } else {
        rjs_throw_type_error(rt, _("this is not a symbol value"));
    }

    return s;
}

/*Get the symbol descriptive string.*/
static RJS_Result
symbol_descriptive_string (RJS_Runtime *rt, RJS_Symbol *s, RJS_Value *str)
{
    RJS_UCharBuffer ucb;

    rjs_uchar_buffer_init(rt, &ucb);

    rjs_uchar_buffer_append_chars(rt, &ucb, "Symbol(", -1);
    if (!rjs_value_is_undefined(rt, &s->description)) {
        rjs_uchar_buffer_append_string(rt, &ucb, &s->description);
    }
    rjs_uchar_buffer_append_uchar(rt, &ucb, ')');

    rjs_string_from_uchars(rt, str, ucb.items, ucb.item_num);
    rjs_uchar_buffer_deinit(rt, &ucb);

    return RJS_OK;
}

/*Symbol.prototype.toString*/
static RJS_NF(Symbol_prototype_toString)
{
    RJS_Symbol *s;

    if (!(s = this_symbol_value(rt, thiz)))
        return RJS_ERR;

    return symbol_descriptive_string(rt, s, rv);
}

/*Symbol.prototype.valueOf*/
static RJS_NF(Symbol_prototype_valueOf)
{
    RJS_Symbol *s;

    if (!(s = this_symbol_value(rt, thiz)))
        return RJS_ERR;

    rjs_value_set_symbol(rt, rv, s);
    return RJS_OK;
}

/*Symbol.prototype[@@toPrimitive]*/
static RJS_NF(Symbol_prototype_toPrimitive)
{
    RJS_Symbol *s;

    if (!(s = this_symbol_value(rt, thiz)))
        return RJS_ERR;

    rjs_value_set_symbol(rt, rv, s);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
symbol_prototype_function_descs[] = {
    {
        "toString",
        0,
        Symbol_prototype_toString
    },
    {
        "valueOf",
        0,
        Symbol_prototype_valueOf
    },
    {
        "@@toPrimitive",
        1,
        Symbol_prototype_toPrimitive
    },
    {NULL}
};

/*get Symbol.prototype.description*/
static RJS_NF(Symbol_prototype_description_get)
{
    RJS_Symbol *s;

    if (!(s = this_symbol_value(rt, thiz)))
        return RJS_ERR;

    rjs_value_copy(rt, rv, &s->description);
    return RJS_OK;
}

static const RJS_BuiltinAccessorDesc
symbol_prototype_accessor_descs[] = {
    {
        "description",
        Symbol_prototype_description_get,
        NULL
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
symbol_prototype_desc = {
    "Symbol",
    NULL,
    NULL,
    NULL,
    symbol_prototype_field_descs,
    symbol_prototype_function_descs,
    symbol_prototype_accessor_descs,
    NULL,
    "Symbol_prototype"
};