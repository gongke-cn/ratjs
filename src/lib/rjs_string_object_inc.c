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

/*String*/
static RJS_NF(String_constructor)
{
    RJS_Value *v     = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    RJS_Result r;

    if (argc < 1) {
        rjs_value_copy(rt, str, rjs_s_empty(rt));
    } else {
        if (!nt && rjs_value_is_symbol(rt, v)) {
            RJS_Symbol *s = rjs_value_get_symbol(rt, v);

            return symbol_descriptive_string(rt, s, rv);
        }

        if ((r = rjs_to_string(rt, v, str)) == RJS_ERR)
            goto end;
    }

    if (!nt) {
        rjs_value_copy(rt, rv, str);
    } else {
        if ((r = rjs_primitive_object_new(rt, rv, nt, RJS_O_String_prototype, str)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
string_constructor_desc = {
    "String",
    1,
    String_constructor
};

/*String.fromCharCode*/
static RJS_NF(String_fromCharCode)
{
    RJS_Result r;
    RJS_UChar *uc;
    size_t     i;

    if ((r = rjs_string_from_uchars(rt, rv, NULL, argc)) == RJS_ERR)
        return r;

    uc = (RJS_UChar*)rjs_string_get_uchars(rt, rv);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);
        uint16_t   c;

        if ((r = rjs_to_uint16(rt, arg, &c)) == RJS_ERR)
            return r;

        *uc = c;
        uc ++;
    }

    return RJS_OK;
}

/*String.fromCodePoint*/
static RJS_NF(String_fromCodePoint)
{
    RJS_UCharBuffer ucb;
    size_t          i;
    RJS_Result      r;

    rjs_uchar_buffer_init(rt, &ucb);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);
        RJS_Number n;

        if ((r = rjs_to_number(rt, arg, &n)) == RJS_ERR)
            goto end;

        if (!rjs_is_integral_number(n)) {
            r = rjs_throw_range_error(rt, _("code point must be an integer number"));
            goto end;
        }

        if ((n < 0) || (n > 0x10ffff)) {
            r = rjs_throw_range_error(rt, _("code point must >= 0 and <= 0x10ffff"));
            goto end;
        }

        rjs_uchar_buffer_append_uc(rt, &ucb, n);
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    return r;
}

/*String.raw*/
static RJS_NF(String_raw)
{
    RJS_Value      *templ  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value      *subs   = rjs_argument_get(rt, args, argc, 1);
    size_t          top    = rjs_value_stack_save(rt);
    RJS_Value      *cooked = rjs_value_stack_push(rt);
    RJS_Value      *rawp   = rjs_value_stack_push(rt);
    RJS_Value      *raw    = rjs_value_stack_push(rt);
    RJS_Value      *seg    = rjs_value_stack_push(rt);
    RJS_Value      *sstr   = rjs_value_stack_push(rt);
    RJS_UCharBuffer ucb;
    int64_t         i, len, nsub;
    RJS_Result      r;

    rjs_uchar_buffer_init(rt, &ucb);

    if (argc > 1)
        nsub = argc - 1;
    else
        nsub = 0;

    if ((r = rjs_to_object(rt, templ, cooked)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, cooked, rjs_pn_raw(rt), rawp)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_object(rt, rawp, raw)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, raw, &len)) == RJS_ERR)
        goto end;

    if (len == 0) {
        rjs_value_copy(rt, rv, rjs_s_empty(rt));
        r = RJS_OK;
        goto end;
    }

    i = 0;
    while (1) {
        if ((r = rjs_get_index(rt, raw, i, seg)) == RJS_ERR)
            goto end;

        if ((r = rjs_to_string(rt, seg, sstr)) == RJS_ERR)
            goto end;

        rjs_uchar_buffer_append_string(rt, &ucb, sstr);

        if (i + 1 >= len)
            break;

        if (i < nsub) {
            RJS_Value *sub = rjs_value_buffer_item(rt, subs, i);

            if ((r = rjs_to_string(rt, sub, sstr)) == RJS_ERR)
                goto end;

            rjs_uchar_buffer_append_string(rt, &ucb, sstr);
        }

        i ++;
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
string_function_descs[] = {
    {
        "fromCharCode",
        1,
        String_fromCharCode
    },
    {
        "fromCodePoint",
        1,
        String_fromCodePoint
    },
    {
        "raw",
        1,
        String_raw
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
string_prototype_field_descs[] = {
    {
        "length",
        RJS_VALUE_NUMBER,
        0
    },
    {NULL}
};

/*Get this string value.*/
static RJS_Result
this_string_value (RJS_Runtime *rt, RJS_Value *thiz, RJS_Value *str)
{
    if (rjs_value_is_string(rt, thiz)) {
        rjs_value_copy(rt, str, thiz);
    } else if (rjs_value_is_object(rt, thiz)
            && (rjs_value_get_gc_thing_type(rt, thiz) == RJS_GC_THING_PRIMITIVE)) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, thiz);

        if (!rjs_value_is_string(rt, &po->value))
            return rjs_throw_type_error(rt, _("this is not a string value"));

        rjs_value_copy(rt, str, &po->value);
    } else {
        return rjs_throw_type_error(rt, _("this is not a string value"));
    }

    return RJS_OK;
}

/*String.prototype.at*/
static RJS_NF(String_prototype_at)
{
    RJS_Value *index = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    size_t     len;
    RJS_Number n, k;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    if ((r = rjs_to_integer_or_infinity(rt, index, &n)) == RJS_ERR)
        goto end;

    if (n >= 0) {
        k = n;
    } else {
        k = len + n;
    }

    if ((k < 0) || (k >= len)) {
        rjs_value_set_undefined(rt, rv);
    } else {
        if ((r = rjs_string_substr(rt, str, k, k + 1, rv)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.charAt*/
static RJS_NF(String_prototype_charAt)
{
    RJS_Value *posv  = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    size_t     len;
    RJS_Number pos;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, posv, &pos)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    if ((pos < 0) || (pos >= len)) {
        rjs_value_copy(rt, rv, rjs_s_empty(rt));
    } else {
        if ((r = rjs_string_substr(rt, str, pos, pos + 1, rv)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.charCodeAt*/
static RJS_NF(String_prototype_charCodeAt)
{
    RJS_Value *posv  = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    size_t     len;
    RJS_Number pos;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, posv, &pos)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    if ((pos < 0) || (pos >= len)) {
        rjs_value_set_number(rt, rv, NAN);
    } else {
        int c;

        c = rjs_string_get_uchar(rt, str, pos);

        rjs_value_set_number(rt, rv, c);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.codePointAt*/
static RJS_NF(String_prototype_codePointAt)
{
    RJS_Value *posv  = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    size_t     len;
    RJS_Number pos;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, posv, &pos)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    if ((pos < 0) || (pos >= len)) {
        rjs_value_set_undefined(rt, rv);
    } else {
        int c;

        c = rjs_string_get_uc(rt, str, pos);

        rjs_value_set_number(rt, rv, c);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.concat*/
static RJS_NF(String_prototype_concat)
{
    size_t          top   = rjs_value_stack_save(rt);
    RJS_Value      *str   = rjs_value_stack_push(rt);
    RJS_UCharBuffer ucb;
    size_t          i;
    RJS_Result      r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    rjs_uchar_buffer_append_string(rt, &ucb, str);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, i);

        if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
            goto end;

        rjs_uchar_buffer_append_string(rt, &ucb, str);
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.endsWith*/
static RJS_NF(String_prototype_endsWith)
{
    RJS_Value  *sstr = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *epos = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *str  = rjs_value_stack_push(rt);
    RJS_Value  *ss   = rjs_value_stack_push(rt);
    RJS_Value  *sub  = rjs_value_stack_push(rt);
    size_t      slen, len, start, end;
    RJS_Number  posn;
    RJS_Result  r;
    RJS_Bool    b;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_is_regexp(rt, sstr)) == RJS_ERR)
        goto end;

    if (r) {
        r = rjs_throw_type_error(rt, _("the search string cannot be a regular expression"));
        goto end;
    }

    if ((r = rjs_to_string(rt, sstr, ss)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    if (rjs_value_is_undefined(rt, epos)) {
        posn = len;
    } else {
        if ((r = rjs_to_integer_or_infinity(rt, epos, &posn)) == RJS_ERR)
            goto end;
    }

    end = RJS_CLAMP(posn, 0, len);

    slen = rjs_string_get_length(rt, ss);

    if (slen == 0) {
        b = RJS_TRUE;
    } else if (end < slen) {
        b = RJS_FALSE;
    } else {
        start = end - slen;

        if ((r = rjs_string_substr(rt, str, start, end, sub)) == RJS_ERR)
            goto end;

        b = rjs_same_value_non_numeric(rt, sub, ss);
    }

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.includes*/
static RJS_NF(String_prototype_includes)
{
    RJS_Value  *sstr = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *pos  = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *str  = rjs_value_stack_push(rt);
    RJS_Value  *ss   = rjs_value_stack_push(rt);
    size_t      len, start;
    ssize_t     idx;
    RJS_Number  posn;
    RJS_Result  r;
    RJS_Bool    b;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_is_regexp(rt, sstr)) == RJS_ERR)
        goto end;

    if (r) {
        r = rjs_throw_type_error(rt, _("the search string cannot be a regular expression"));
        goto end;
    }

    if ((r = rjs_to_string(rt, sstr, ss)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, pos, &posn)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    start = RJS_CLAMP(posn, 0, len);

    idx = rjs_string_index_of(rt, str, ss, start);
    b   = (idx != -1);

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.indexOf*/
static RJS_NF(String_prototype_indexOf)
{
    RJS_Value  *sstr = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *pos  = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *str  = rjs_value_stack_push(rt);
    RJS_Value  *ss   = rjs_value_stack_push(rt);
    size_t      len, start;
    ssize_t     idx;
    RJS_Number  posn;
    RJS_Result  r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, sstr, ss)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, pos, &posn)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    start = RJS_CLAMP(posn, 0, len);

    idx = rjs_string_index_of(rt, str, ss, start);

    rjs_value_set_number(rt, rv, idx);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.lastIndexOf*/
static RJS_NF(String_prototype_lastIndexOf)
{
    RJS_Value  *sstr = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *pos  = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *str  = rjs_value_stack_push(rt);
    RJS_Value  *ss   = rjs_value_stack_push(rt);
    RJS_Value  *nv   = rjs_value_stack_push(rt);
    size_t      start;
    ssize_t     idx, len, slen;
    RJS_Number  posn;
    RJS_Result  r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, sstr, ss)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_number(rt, pos, &posn)) == RJS_ERR)
        goto end;

    if (isnan(posn)) {
        posn = INFINITY;
    } else {
        rjs_value_set_number(rt, nv, posn);
        rjs_to_integer_or_infinity(rt, nv, &posn);
    }

    len  = rjs_string_get_length(rt, str);
    slen = rjs_string_get_length(rt, ss);

    start = RJS_CLAMP(posn, 0, len - slen);

    idx = rjs_string_last_index_of(rt, str, ss, start);

    rjs_value_set_number(rt, rv, idx);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.localeCompare*/
static RJS_NF(String_prototype_localeCompare)
{
    RJS_Value        *that = rjs_argument_get(rt, args, argc, 0);
    size_t            top  = rjs_value_stack_save(rt);
    RJS_Value        *s1   = rjs_value_stack_push(rt);
    RJS_Value        *s2   = rjs_value_stack_push(rt);
    const RJS_UChar  *c1, *c2;
    size_t            l1, l2;
    RJS_Result        r;
    int               i;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s1)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, that, s2)) == RJS_ERR)
        goto end;

    c1 = rjs_string_get_uchars(rt, s1);
    c2 = rjs_string_get_uchars(rt, s2);
    l1 = rjs_string_get_length(rt, s1);
    l2 = rjs_string_get_length(rt, s2);

    i = rjs_uchars_compare(c1, l1, c2, l2);

    rjs_value_set_number(rt, rv, i);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.match*/
static RJS_NF(String_prototype_match)
{
    RJS_Value *re    = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *match = rjs_value_stack_push(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    RJS_Value *rx    = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, re) && !rjs_value_is_null(rt, re)) {
        if ((r = rjs_get_method(rt, re, rjs_pn_s_match(rt), match)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, match)) {
            r = rjs_call(rt, match, re, thiz, 1, rv);
            goto end;
        }
    }

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_regexp_create(rt, re, rjs_v_undefined(rt), rx)) == RJS_ERR)
        goto end;

    r = rjs_invoke(rt, rx, rjs_pn_s_match(rt), str, 1, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.matchAll*/
static RJS_NF(String_prototype_matchAll)
{
    RJS_Value *re    = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *flags = rjs_value_stack_push(rt);
    RJS_Value *fstr  = rjs_value_stack_push(rt);
    RJS_Value *match = rjs_value_stack_push(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    RJS_Value *rx    = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, re) && !rjs_value_is_null(rt, re)) {
        if ((r = rjs_is_regexp(rt, re)) == RJS_ERR)
            goto end;

        if (r) {
            if ((r = rjs_get(rt, re, rjs_pn_flags(rt), flags)) == RJS_ERR)
                goto end;

            if ((r = rjs_require_object_coercible(rt, flags)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, flags, fstr)) == RJS_ERR)
                goto end;

            if (rjs_string_index_of_uchar(rt, fstr, 'g', 0) == -1) {
                r = rjs_throw_type_error(rt, _("the regular expression must has \"g\" flag"));
                goto end;
            }
        }

        if ((r = rjs_get_method(rt, re, rjs_pn_s_matchAll(rt), match)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, match)) {
            r = rjs_call(rt, match, re, thiz, 1, rv);
            goto end;
        }
    }

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_regexp_create(rt, re, rjs_s_g(rt), rx)) == RJS_ERR)
        goto end;

    r = rjs_invoke(rt, rx, rjs_pn_s_matchAll(rt), str, 1, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.normalize*/
static RJS_NF(String_prototype_normalize)
{
    char            *mode = "NFC";
    RJS_Value       *form = rjs_argument_get(rt, args, argc, 0);
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *fstr = rjs_value_stack_push(rt);
    RJS_Value       *s    = rjs_value_stack_push(rt);
    const RJS_UChar *c;
    size_t           len;
    int              rlen;
    RJS_Result       r;
    RJS_UCharBuffer  ucb;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, form)) {
        if ((r = rjs_to_string(rt, form, fstr)) == RJS_ERR)
            goto end;

        if (rjs_string_equal(rt, fstr, rjs_s_NFC(rt)))
            mode = "NFC";
        else if (rjs_string_equal(rt, fstr, rjs_s_NFD(rt)))
            mode = "NFD";
        else if (rjs_string_equal(rt, fstr, rjs_s_NFKC(rt)))
            mode = "NFKC";
        else if (rjs_string_equal(rt, fstr, rjs_s_NFKD(rt)))
            mode = "NFKD";
        else {
            r = rjs_throw_range_error(rt, _("illegal normalize form"));
            goto end;
        }
    }

    c   = rjs_string_get_uchars(rt, s);
    len = rjs_string_get_length(rt, s);

    rjs_vector_set_capacity(&ucb, len, rt);
    rlen = rjs_uchars_normalize(c, len, ucb.items, len, mode);
    if (rlen > len) {
        rjs_vector_set_capacity(&ucb, rlen, rt);
        rlen = rjs_uchars_normalize(c, len, ucb.items, rlen, mode);
    }

    rjs_string_from_uchars(rt, rv, ucb.items, rlen);
    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.padEnd*/
static RJS_NF(String_prototype_padEnd)
{
    RJS_Value *max_len  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *fill_str = rjs_argument_get(rt, args, argc, 1);
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        return r;

    return rjs_string_pad(rt, thiz, max_len, fill_str, RJS_STRING_PAD_END, rv);
}

/*String.prototype.padStart*/
static RJS_NF(String_prototype_padStart)
{
    RJS_Value *max_len  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *fill_str = rjs_argument_get(rt, args, argc, 1);
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        return r;

    return rjs_string_pad(rt, thiz, max_len, fill_str, RJS_STRING_PAD_START, rv);
}

/*String.prototype.repeat*/
static RJS_NF(String_prototype_repeat)
{
    RJS_Value *count = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    RJS_Number n;
    size_t     rc;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, count, &n)) == RJS_ERR)
        goto end;

    if ((n < 0) || isinf(n)) {
        r = rjs_throw_range_error(rt, _("repeat count must >= 0 and < infinite"));
        goto end;
    }
    rc = n;

    if (rc == 0) {
        rjs_value_copy(rt, rv, rjs_s_empty(rt));
    } else {
        size_t           slen = rjs_string_get_length(rt, str);
        size_t           rlen = rc * slen;
        size_t           i;
        RJS_UChar       *d;
        const RJS_UChar *s;

        if ((r = rjs_string_from_uchars(rt, rv, NULL, rlen)) == RJS_ERR)
            goto end;

        d = (RJS_UChar*)rjs_string_get_uchars(rt, rv);
        s = rjs_string_get_uchars(rt, str);

        for (i = 0; i < rc; i ++) {
            RJS_ELEM_CPY(d, s, slen);
            d += slen;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.replace*/
static RJS_NF(String_prototype_replace)
{
    RJS_Value *searchv  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *replacev = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *rep_arg0 = rjs_value_stack_push(rt);
    RJS_Value *rep_arg1 = rjs_value_stack_push(rt);
    RJS_Value *replacer = rjs_value_stack_push(rt);
    RJS_Value *sstr     = rjs_value_stack_push(rt);
    RJS_Value *posv     = rjs_value_stack_push(rt);
    RJS_Value *str      = rjs_value_stack_push(rt);
    RJS_Value *rstr     = rjs_value_stack_push(rt);
    RJS_Value *captures = rjs_value_stack_push(rt);
    RJS_Value *fres     = rjs_value_stack_push(rt);
    RJS_Value *rres     = rjs_value_stack_push(rt);
    RJS_Bool   func_replace;
    size_t     len, slen, rlen, nlen, left;
    ssize_t    pos;
    RJS_UChar *d;
    const RJS_UChar *s, *rs;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, searchv) && !rjs_value_is_null(rt, searchv)) {
        if ((r = rjs_get_method(rt, searchv, rjs_pn_s_replace(rt), replacer)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, replacer)) {
            rjs_value_copy(rt, rep_arg0, thiz);
            rjs_value_copy(rt, rep_arg1, replacev);

            r = rjs_call(rt, replacer, searchv, rep_arg0, 2, rv);
            goto end;
        }
    }

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, searchv, sstr)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, replacev)) {
        if ((r = rjs_to_string(rt, replacev, rstr)) == RJS_ERR)
            goto end;

        func_replace = RJS_FALSE;
    } else {
        func_replace = RJS_TRUE;
    }

    slen = rjs_string_get_length(rt, sstr);

    pos = rjs_string_index_of(rt, str, sstr, 0);
    if (pos == -1) {
        rjs_value_copy(rt, rv, str);
        r = RJS_OK;
        goto end;
    }

    if (func_replace) {
        rjs_value_set_number(rt, posv, pos);

        if ((r = rjs_call(rt, replacev, rjs_v_undefined(rt), sstr, 3, fres)) == RJS_ERR)
            goto end;

        if ((r = rjs_to_string(rt, fres, rres)) == RJS_ERR)
            goto end;
    } else {
        if ((r = rjs_array_new(rt, captures, 1, NULL)) == RJS_ERR)
            goto end;

        if ((r = rjs_create_data_property_or_throw_index(rt, captures, 0, sstr)) == RJS_ERR)
            goto end;

        if ((r = rjs_get_substitution(rt, str, pos, captures, rstr, rres)) == RJS_ERR)
            goto end;
    }

    len  = rjs_string_get_length(rt, str);
    rlen = rjs_string_get_length(rt, rres);
    nlen = len - slen + rlen;

    if ((r = rjs_string_from_uchars(rt, rv, NULL, nlen)) == RJS_ERR)
        goto end;

    d = (RJS_UChar*)rjs_string_get_uchars(rt, rv);
    s = rjs_string_get_uchars(rt, str);

    RJS_ELEM_CPY(d, s, pos);
    d += pos;

    rs = rjs_string_get_uchars(rt, rres);
    RJS_ELEM_CPY(d, rs, rlen);
    d += rlen;

    left = len - pos - slen;
    RJS_ELEM_CPY(d, s + pos + slen, left);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.replaceAll*/
static RJS_NF(String_prototype_replaceAll)
{
    RJS_Value *searchv  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *replacev = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *rep_arg0 = rjs_value_stack_push(rt);
    RJS_Value *rep_arg1 = rjs_value_stack_push(rt);
    RJS_Value *replacer = rjs_value_stack_push(rt);
    RJS_Value *sstr     = rjs_value_stack_push(rt);
    RJS_Value *posv     = rjs_value_stack_push(rt);
    RJS_Value *str      = rjs_value_stack_push(rt);
    RJS_Value *rstr     = rjs_value_stack_push(rt);
    RJS_Value *captures = rjs_value_stack_push(rt);
    RJS_Value *fres     = rjs_value_stack_push(rt);
    RJS_Value *rres     = rjs_value_stack_push(rt);
    RJS_Value *flags    = rjs_value_stack_push(rt);
    RJS_Value *fstr     = rjs_value_stack_push(rt);
    RJS_Bool   func_replace;
    size_t     len, slen;
    ssize_t    pos, spos, start;
    const RJS_UChar *s;
    RJS_UCharBuffer  ucb;
    RJS_Result r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, searchv) && !rjs_value_is_null(rt, searchv)) {
        if ((r = rjs_is_regexp(rt, searchv)) == RJS_ERR)
            goto end;
        if (r) {
            if ((r = rjs_get(rt, searchv, rjs_pn_flags(rt), flags)) == RJS_ERR)
                goto end;

            if ((r = rjs_require_object_coercible(rt, flags)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, flags, fstr)) == RJS_ERR)
                goto end;

            if (rjs_string_index_of_uchar(rt, fstr, 'g', 0) == -1) {
                r = rjs_throw_type_error(rt, _("the regular expression must has \"g\" flag"));
                goto end;
            }
        }

        if ((r = rjs_get_method(rt, searchv, rjs_pn_s_replace(rt), replacer)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, replacer)) {
            rjs_value_copy(rt, rep_arg0, thiz);
            rjs_value_copy(rt, rep_arg1, replacev);

            r = rjs_call(rt, replacer, searchv, rep_arg0, 2, rv);
            goto end;
        }
    }

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, searchv, sstr)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, replacev)) {
        if ((r = rjs_to_string(rt, replacev, rstr)) == RJS_ERR)
            goto end;

        func_replace = RJS_FALSE;
    } else {
        func_replace = RJS_TRUE;
    }

    slen = rjs_string_get_length(rt, sstr);
    len  = rjs_string_get_length(rt, str);
    s    = rjs_string_get_uchars(rt, str);

    start = 0;
    spos  = 0;
    pos   = rjs_string_index_of(rt, str, sstr, spos);

    while (pos != -1) {
        rjs_uchar_buffer_append_uchars(rt, &ucb, s + start, pos - start);

        if (func_replace) {
            rjs_value_set_number(rt, posv, pos);

            if ((r = rjs_call(rt, replacev, rjs_v_undefined(rt), sstr, 3, fres)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, fres, rres)) == RJS_ERR)
                goto end;
        } else {
            if ((r = rjs_array_new(rt, captures, 1, NULL)) == RJS_ERR)
                goto end;

            if ((r = rjs_create_data_property_or_throw_index(rt, captures, 0, sstr)) == RJS_ERR)
                goto end;

            if ((r = rjs_get_substitution(rt, str, pos, captures, rstr, rres)) == RJS_ERR)
                goto end;
        }

        rjs_uchar_buffer_append_string(rt, &ucb, rres);

        start = pos + slen;
        spos  = slen ? start : start + 1;
        pos   = rjs_string_index_of(rt, str, sstr, spos);
    }

    if (start < len)
        rjs_uchar_buffer_append_uchars(rt, &ucb, s + start, len - start);

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.search*/
static RJS_NF(String_prototype_search)
{
    RJS_Value *regexp   = rjs_argument_get(rt, args, argc, 0);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *searcher = rjs_value_stack_push(rt);
    RJS_Value *str      = rjs_value_stack_push(rt);
    RJS_Value *rx       = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, regexp) && !rjs_value_is_null(rt, regexp)) {
        if ((r = rjs_get_method(rt, regexp, rjs_pn_s_search(rt), searcher)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, searcher)) {
            r = rjs_call(rt, searcher, regexp, thiz, 1, rv);
            goto end;
        }
    }

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_regexp_create(rt, regexp, rjs_v_undefined(rt), rx)) == RJS_ERR)
        goto end;

    r = rjs_invoke(rt, rx, rjs_pn_s_search(rt), str, 1, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.slice*/
static RJS_NF(String_prototype_slice)
{
    RJS_Value *start = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *end   = rjs_argument_get(rt, args, argc, 1);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *s     = rjs_value_stack_push(rt);
    size_t     len, from, to;
    RJS_Number int_start, int_end;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, s);

    if ((r = rjs_to_integer_or_infinity(rt, start, &int_start)) == RJS_ERR)
        goto end;

    if (int_start == -INFINITY)
        from = 0;
    else if (int_start < 0)
        from = RJS_MAX(len + int_start, 0);
    else
        from = RJS_MIN(int_start, len);

    if (rjs_value_is_undefined(rt, end))
        int_end = len;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &int_end)) == RJS_ERR)
        goto end;

    if (int_end == -INFINITY)
        to = 0;
    else if (int_end < 0)
        to = RJS_MAX(len + int_end, 0);
    else
        to = RJS_MIN(int_end, len);

    if (from >= to) {
        rjs_value_copy(rt, rv, rjs_s_empty(rt));
        r = RJS_OK;
    } else {
        r = rjs_string_substr(rt, s, from, to, rv);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.split*/
static RJS_NF(String_prototype_split)
{
    RJS_Value *separator = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *limit     = rjs_argument_get(rt, args, argc, 1);
    size_t     top       = rjs_value_stack_save(rt);
    RJS_Value *splitter  = rjs_value_stack_push(rt);
    RJS_Value *arg1      = rjs_value_stack_push(rt);
    RJS_Value *arg2      = rjs_value_stack_push(rt);
    RJS_Value *s         = rjs_value_stack_push(rt);
    RJS_Value *ss        = rjs_value_stack_push(rt);
    RJS_Value *sub       = rjs_value_stack_push(rt);
    uint32_t   lim, n;
    size_t     slen, len, start;
    ssize_t    idx;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, separator) && !rjs_value_is_null(rt, separator)) {
        if ((r = rjs_get_method(rt, separator, rjs_pn_s_split(rt), splitter)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, splitter)) {
            rjs_value_copy(rt, arg1, thiz);
            rjs_value_copy(rt, arg2, limit);

            r = rjs_call(rt, splitter, separator, arg1, 2, rv);
            goto end;
        }
    }

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, limit))
        lim = 0xffffffff;
    else if ((r = rjs_to_uint32(rt, limit, &lim)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, separator, ss)) == RJS_ERR)
        goto end;

    if (lim == 0) {
        r = rjs_array_new(rt, rv, 0, NULL);
        goto end;
    }

    if (rjs_value_is_undefined(rt, separator)) {
        r = rjs_create_array_from_elements(rt, rv, s, NULL);
        goto end;
    }

    len  = rjs_string_get_length(rt, s);
    slen = rjs_string_get_length(rt, ss);
    if (slen == 0) {
        uint32_t i;

        n = RJS_MIN(lim, len);

        if ((r = rjs_array_new(rt, rv, n, NULL)) == RJS_ERR)
            goto end;

        for (i = 0; i < n; i ++) {
            if ((r = rjs_string_substr(rt, s, i, i + 1, sub)) == RJS_ERR)
                goto end;

            if ((r = rjs_set_index(rt, rv, i, sub, RJS_TRUE)) == RJS_ERR)
                goto end;
        }

        r = RJS_OK;
        goto end;
    }

    if (len == 0) {
        r = rjs_create_array_from_elements(rt, rv, s, NULL);
        goto end;
    }

    if ((r = rjs_array_new(rt, rv, 0, NULL)) == RJS_ERR)
        goto end;

    idx   = rjs_string_index_of(rt, s, ss, 0);
    start = 0;
    n     = 0;
    while (idx != -1) {
        if ((r = rjs_string_substr(rt, s, start, idx, sub)) == RJS_ERR)
            goto end;

        if ((r = rjs_set_index(rt, rv, n, sub, RJS_TRUE)) == RJS_ERR)
            goto end;

        n ++;

        if (n == lim) {
            r = RJS_OK;
            goto end;
        }

        start = idx + slen;
        idx   = rjs_string_index_of(rt, s, ss, start);
    }

    if ((r = rjs_string_substr(rt, s, start, len, sub)) == RJS_ERR)
        goto end;

    if ((r = rjs_set_index(rt, rv, n, sub, RJS_TRUE)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.startsWith*/
static RJS_NF(String_prototype_startsWith)
{
    RJS_Value  *sstr = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *pos  = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *str  = rjs_value_stack_push(rt);
    RJS_Value  *ss   = rjs_value_stack_push(rt);
    RJS_Value  *sub  = rjs_value_stack_push(rt);
    size_t      slen, len, start, end;
    RJS_Number  posn;
    RJS_Result  r;
    RJS_Bool    b;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_is_regexp(rt, sstr)) == RJS_ERR)
        goto end;

    if (r) {
        r = rjs_throw_type_error(rt, _("the search string cannot be a regular expression"));
        goto end;
    }

    if ((r = rjs_to_string(rt, sstr, ss)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, str);

    if (rjs_value_is_undefined(rt, pos)) {
        posn = 0;
    } else {
        if ((r = rjs_to_integer_or_infinity(rt, pos, &posn)) == RJS_ERR)
            goto end;
    }

    start = RJS_CLAMP(posn, 0, len);

    slen = rjs_string_get_length(rt, ss);

    if (slen == 0) {
        b = RJS_TRUE;
    } else {
        end = start + slen;
        if (end > len) {
            b = RJS_FALSE;
        } else {
            if ((r = rjs_string_substr(rt, str, start, end, sub)) == RJS_ERR)
                goto end;

            b = rjs_same_value_non_numeric(rt, sub, ss);
        }
    }

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.substring*/
static RJS_NF(String_prototype_substring)
{
    RJS_Value  *start = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *end   = rjs_argument_get(rt, args, argc, 1);
    size_t      top   = rjs_value_stack_save(rt);
    RJS_Value  *s     = rjs_value_stack_push(rt);
    size_t      len, final_start, final_end, from, to;
    RJS_Number  int_start, int_end;
    RJS_Result  r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, s);

    if ((r = rjs_to_integer_or_infinity(rt, start, &int_start)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, end))
        int_end = len;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &int_end)) == RJS_ERR)
        goto end;

    final_start = RJS_CLAMP(int_start, 0, len);
    final_end   = RJS_CLAMP(int_end, 0, len);

    from = RJS_MIN(final_start, final_end);
    to   = RJS_MAX(final_start, final_end);

    r = rjs_string_substr(rt, s, from, to, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String toLower.*/
static RJS_Result
string_toLower (RJS_Runtime *rt, RJS_Value *thiz, const char *locale, RJS_Value *rv)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *s   = rjs_value_stack_push(rt);
    size_t           len;
    RJS_UCharBuffer  ucb;
    int              cnt;
    const RJS_UChar *c;
    RJS_Result       r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, s);
    c   = rjs_string_get_uchars(rt, s);

    rjs_vector_set_capacity(&ucb, len, rt);
    cnt = rjs_uchars_to_lower(c, len, ucb.items, ucb.item_cap, locale);
    if (cnt > ucb.item_cap) {
        rjs_vector_set_capacity(&ucb, cnt, rt);
        cnt = rjs_uchars_to_lower(c, len, ucb.items, ucb.item_cap, locale);
    }

    rjs_string_from_uchars(rt, rv, ucb.items, cnt);
    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String toUpper.*/
static RJS_Result
string_toUpper (RJS_Runtime *rt, RJS_Value *thiz, const char *locale, RJS_Value *rv)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *s   = rjs_value_stack_push(rt);
    size_t           len;
    RJS_UCharBuffer  ucb;
    int              cnt;
    const RJS_UChar *c;
    RJS_Result       r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, s);
    c   = rjs_string_get_uchars(rt, s);

    rjs_vector_set_capacity(&ucb, len, rt);
    cnt = rjs_uchars_to_upper(c, len, ucb.items, ucb.item_cap, locale);
    if (cnt > ucb.item_cap) {
        rjs_vector_set_capacity(&ucb, cnt, rt);
        cnt = rjs_uchars_to_upper(c, len, ucb.items, ucb.item_cap, locale);
    }

    rjs_string_from_uchars(rt, rv, ucb.items, cnt);
    r = RJS_OK;
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*String.prototype.toLocaleLowerCase*/
static RJS_NF(String_prototype_toLocaleLowerCase)
{
    return string_toLower(rt, thiz, NULL, rv);
}

/*String.prototype.toLowerCase*/
static RJS_NF(String_prototype_toLowerCase)
{
    return string_toLower(rt, thiz, "", rv);
}

/*String.prototype.toLocaleUpperCase*/
static RJS_NF(String_prototype_toLocaleUpperCase)
{
    return string_toUpper(rt, thiz, NULL, rv);
}

/*String.prototype.toUpperCase*/
static RJS_NF(String_prototype_toUpperCase)
{
    return string_toUpper(rt, thiz, "", rv);
}

/*String.prototype.toString*/
static RJS_NF(String_prototype_toString)
{
    return this_string_value(rt, thiz, rv);
}

/*String.prototype.trim*/
static RJS_NF(String_prototype_trim)
{
    return rjs_string_trim(rt, thiz, RJS_STRING_TRIM_START|RJS_STRING_TRIM_END, rv);
}

/*String.prototype.trimEnd*/
static RJS_NF(String_prototype_trimEnd)
{
    return rjs_string_trim(rt, thiz, RJS_STRING_TRIM_END, rv);
}

/*String.prototype.trimStart*/
static RJS_NF(String_prototype_trimStart)
{
    return rjs_string_trim(rt, thiz, RJS_STRING_TRIM_START, rv);
}

/*String.prototype.valueOf*/
static RJS_NF(String_prototype_valueOf)
{
    return this_string_value(rt, thiz, rv);
}

/*The string iterator.*/
typedef struct {
    RJS_Object object; /**< The base object.*/
    RJS_Value  str;    /**< The string.*/
    size_t     pos;    /**< The current position.*/
} RJS_StringIterator;

/*Scan reference things in the string iterator.*/
static void
string_iterator_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_StringIterator *si = ptr;

    rjs_object_op_gc_scan(rt, &si->object);
    rjs_gc_scan_value(rt, &si->str);
}

/*Free the string iterator.*/
static void
string_iterator_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_StringIterator *si = ptr;

    rjs_object_deinit(rt, &si->object);

    RJS_DEL(rt, si);
}

/*String iterator object operation functions.*/
static const RJS_ObjectOps
string_iterator_ops = {
    {
        RJS_GC_THING_STRING_ITERATOR,
        string_iterator_op_gc_scan,
        string_iterator_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*String.prototype[@@iterator]*/
static RJS_NF(String_prototype_iterator)
{
    RJS_Realm          *realm = rjs_realm_current(rt);
    size_t              top   = rjs_value_stack_save(rt);
    RJS_Value          *s     = rjs_value_stack_push(rt);
    RJS_StringIterator *si;
    RJS_Result          r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, thiz, s)) == RJS_ERR)
        goto end;

    RJS_NEW(rt, si);

    rjs_value_copy(rt, &si->str, s);
    si->pos = 0;

    r = rjs_object_init(rt, rv, &si->object, rjs_o_StringIteratorPrototype(realm), &string_iterator_ops);
    if (r == RJS_ERR)
        RJS_DEL(rt, si);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
string_prototype_function_descs[] = {
    {
        "at",
        1,
        String_prototype_at
    },
    {
        "charAt",
        1,
        String_prototype_charAt
    },
    {
        "charCodeAt",
        1,
        String_prototype_charCodeAt
    },
    {
        "codePointAt",
        1,
        String_prototype_codePointAt
    },
    {
        "concat",
        1,
        String_prototype_concat
    },
    {
        "endsWith",
        1,
        String_prototype_endsWith
    },
    {
        "includes",
        1,
        String_prototype_includes
    },
    {
        "indexOf",
        1,
        String_prototype_indexOf
    },
    {
        "lastIndexOf",
        1,
        String_prototype_lastIndexOf
    },
    {
        "localeCompare",
        1,
        String_prototype_localeCompare
    },
    {
        "match",
        1,
        String_prototype_match
    },
    {
        "matchAll",
        1,
        String_prototype_matchAll
    },
    {
        "normalize",
        0,
        String_prototype_normalize
    },
    {
        "padEnd",
        1,
        String_prototype_padEnd
    },
    {
        "padStart",
        1,
        String_prototype_padStart
    },
    {
        "repeat",
        1,
        String_prototype_repeat
    },
    {
        "replace",
        2,
        String_prototype_replace
    },
    {
        "replaceAll",
        2,
        String_prototype_replaceAll
    },
    {
        "search",
        1,
        String_prototype_search
    },
    {
        "slice",
        2,
        String_prototype_slice
    },
    {
        "split",
        2,
        String_prototype_split
    },
    {
        "startsWith",
        1,
        String_prototype_startsWith
    },
    {
        "substring",
        2,
        String_prototype_substring
    },
    {
        "toLocaleLowerCase",
        0,
        String_prototype_toLocaleLowerCase
    },
    {
        "toLocaleUpperCase",
        0,
        String_prototype_toLocaleUpperCase
    },
    {
        "toLowerCase",
        0,
        String_prototype_toLowerCase
    },
    {
        "toString",
        0,
        String_prototype_toString
    },
    {
        "toUpperCase",
        0,
        String_prototype_toUpperCase
    },
    {
        "trim",
        0,
        String_prototype_trim
    },
    {
        "trimEnd",
        0,
        String_prototype_trimEnd
    },
    {
        "trimStart",
        0,
        String_prototype_trimStart
    },
    {
        "valueOf",
        0,
        String_prototype_valueOf
    },
    {
        "@@iterator",
        0,
        String_prototype_iterator
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
string_prototype_desc = {
    "String",
    NULL,
    NULL,
    NULL,
    string_prototype_field_descs,
    string_prototype_function_descs,
    NULL,
    NULL,
    "String_prototype"
};

static const RJS_BuiltinFieldDesc
string_iterator_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "String Iterator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*%StringIteratorPrototype%.next*/
static RJS_NF(StringIteratorPrototype_next)
{
    RJS_Result          r;
    RJS_StringIterator *si;
    size_t              len;
    RJS_Bool            end;
    size_t              top = rjs_value_stack_save(rt);
    RJS_Value          *sub = rjs_value_stack_push(rt);

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_STRING_ITERATOR) {
        r = rjs_throw_type_error(rt, _("the value is not a string iterator"));
        goto end;
    }

    si = (RJS_StringIterator*)rjs_value_get_object(rt, thiz);

    len = rjs_string_get_length(rt, &si->str);
    if (si->pos >= len) {
        rjs_value_set_undefined(rt, sub);

        end = RJS_TRUE;
    } else {
        const RJS_UChar *c;
        size_t           n = 1;

        c = rjs_string_get_uchars(rt, &si->str);

        if (rjs_uchar_is_leading_surrogate(c[si->pos])) {
            if ((si->pos < len - 1) && rjs_uchar_is_trailing_surrogate(c[si->pos + 1]))
                n ++;
        }

        if ((r = rjs_string_substr(rt, &si->str, si->pos, si->pos + n, sub)) == RJS_ERR)
            goto end;

        si->pos += n;

        end = RJS_FALSE;
    }

    if ((r = rjs_create_iter_result_object(rt, sub, end, rv)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
string_iterator_prototype_function_descs[] = {
    {
        "next",
        0,
        StringIteratorPrototype_next
    },
    {NULL}
};
