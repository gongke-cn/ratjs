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

/*RegExp*/
static RJS_NF(RegExp_constructor)
{
    RJS_Value *pattern    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *flags      = rjs_argument_get(rt, args, argc, 1);
    size_t     top        = rjs_value_stack_save(rt);
    RJS_Value *pat_constr = rjs_value_stack_push(rt);
    RJS_Value *p          = rjs_value_stack_push(rt);
    RJS_Value *fl         = rjs_value_stack_push(rt);
    RJS_Bool   pat_is_re;
    RJS_Result r;

    if ((r = rjs_is_regexp(rt, pattern)) == RJS_ERR)
        goto end;
    pat_is_re = r;

    if (!nt) {
        nt = f;
        if (pat_is_re && rjs_value_is_undefined(rt, flags)) {
            if ((r = rjs_get(rt, pattern, rjs_pn_constructor(rt), pat_constr)) == RJS_ERR)
                goto end;

            if (rjs_same_value(rt, nt, pat_constr)) {
                rjs_value_copy(rt, rv, pattern);
                r = RJS_OK;
                goto end;
            }
        }
    }

    if (pat_is_re) {
        if ((r = rjs_get(rt, pattern, rjs_pn_source(rt), p)) == RJS_ERR)
            goto end;

        if (rjs_value_is_undefined(rt, flags)) {
            if ((r = rjs_get(rt, pattern, rjs_pn_flags(rt), fl)) == RJS_ERR)
                goto end;
        } else {
            rjs_value_copy(rt, fl, flags);
        }
    } else {
        rjs_value_copy(rt, p, pattern);
        rjs_value_copy(rt, fl, flags);
    }

    if ((r = rjs_regexp_alloc(rt, nt, rv)) == RJS_ERR)
        goto end;

    if ((r = rjs_regexp_initialize(rt, rv, p, fl)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
regexp_constructor_desc = {
    "RegExp",
    2,
    RegExp_constructor
};

static const RJS_BuiltinAccessorDesc
regexp_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

/*RegExp.prototype.exec*/
static RJS_NF(RegExp_prototype_exec)
{
    RJS_Value *str = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *s   = rjs_value_stack_push(rt);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_REGEXP) {
        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    if ((r = rjs_regexp_builtin_exec(rt, thiz, s, rv)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the next index value.*/
static int64_t
adv_str_index (RJS_Runtime *rt, RJS_Value *str, int64_t idx, RJS_Bool unicode)
{
    size_t len;
    int    c;

    if (!unicode)
        return idx + 1;

    len = rjs_string_get_length(rt, str);

    if (idx + 1 >= len)
        return idx + 1;

    c = rjs_string_get_uchar(rt, str, idx);
    if (rjs_uchar_is_leading_surrogate(c)) {
        c = rjs_string_get_uchar(rt, str, idx + 1);
        if (rjs_uchar_is_trailing_surrogate(c))
            return idx + 2;
    }

    return idx + 1;
}

/*RegExp.prototype[@@match]*/
static RJS_NF(RegExp_prototype_match)
{
    RJS_Value *str      = rjs_argument_get(rt, args, argc, 0);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *s        = rjs_value_stack_push(rt);
    RJS_Value *flagsv   = rjs_value_stack_push(rt);
    RJS_Value *flags    = rjs_value_stack_push(rt);
    RJS_Value *idxv     = rjs_value_stack_push(rt);
    RJS_Value *res      = rjs_value_stack_push(rt);
    RJS_Value *mv       = rjs_value_stack_push(rt);
    RJS_Value *mstr     = rjs_value_stack_push(rt);
    RJS_Bool   global, unicode;
    size_t     n;
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_flags(rt), flagsv)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, flagsv, flags)) == RJS_ERR)
        goto end;

    global = (rjs_string_index_of_uchar(rt, flags, 'g', 0) != -1);

    if (!global) {
        if ((r = rjs_regexp_exec(rt, thiz, s, rv)) == RJS_ERR)
            goto end;
    } else {
        unicode = (rjs_string_index_of_uchar(rt, flags, 'u', 0) != -1);

        rjs_value_set_number(rt, idxv, 0);

        if ((r = rjs_set(rt, thiz, rjs_pn_lastIndex(rt), idxv, RJS_TRUE)) == RJS_ERR)
            goto end;

        if ((r = rjs_array_new(rt, rv, 0, NULL)) == RJS_ERR)
            goto end;

        n = 0;

        while (1) {
            if ((r = rjs_regexp_exec(rt, thiz, s, res)) == RJS_ERR)
                goto end;

            if (rjs_value_is_null(rt, res)) {
                if (n == 0)
                    rjs_value_set_null(rt, rv);

                r = RJS_OK;
                goto end;
            }

            if ((r = rjs_get_index(rt, res, 0, mv)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, mv, mstr)) == RJS_ERR)
                goto end;

            rjs_create_data_property_or_throw_index(rt, rv, n, mstr);

            if (rjs_string_get_length(rt, mstr) == 0) {
                int64_t idx;

                if ((r = rjs_get(rt, thiz, rjs_pn_lastIndex(rt), idxv)) == RJS_ERR)
                    goto end;

                if ((r = rjs_to_length(rt, idxv, &idx)) == RJS_ERR)
                    goto end;

                idx = adv_str_index(rt, s, idx, unicode);

                rjs_value_set_number(rt, idxv, idx);

                if ((r = rjs_set(rt, thiz, rjs_pn_lastIndex(rt), idxv, RJS_TRUE)) == RJS_ERR)
                    goto end;
            }

            n ++;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**Regular expression string iterator object.*/
typedef struct {
    RJS_Object object;  /**< Base object data.*/
    RJS_Value  re;      /**< The regular expression.*/
    RJS_Value  str;     /**< The string.*/
    RJS_Bool   global;  /**< Global flag.*/
    RJS_Bool   unicode; /**< Unicode flag.*/
    RJS_Bool   done;    /**< Done flag.*/
} RJS_RegExpStringIterator;

/*Scan the referenced things in the regular expression string iterator.*/
static void
regexp_str_iter_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_RegExpStringIterator *resi = ptr;

    rjs_object_op_gc_scan(rt, &resi->object);
    rjs_gc_scan_value(rt, &resi->re);
    rjs_gc_scan_value(rt, &resi->str);
}

/*Free the regular expression string iterator.*/
static void
regexp_str_iter_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_RegExpStringIterator *resi = ptr;

    rjs_object_deinit(rt, &resi->object);

    RJS_DEL(rt, resi);
}

/*Regular expression string iterator operation functions.*/
static const RJS_ObjectOps
regexp_str_iter_ops = {
    {
        RJS_GC_THING_REGEXP_STRING_ITERATOR,
        regexp_str_iter_op_gc_scan,
        regexp_str_iter_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Create a new regular expression string iterator object.*/
static RJS_Result
regexp_str_iter_new (RJS_Runtime *rt, RJS_Value *iter, RJS_Value *re, RJS_Value *str,
        RJS_Bool global, RJS_Bool unicode)
{
    RJS_Realm                *realm = rjs_realm_current(rt);
    RJS_RegExpStringIterator *resi;
    RJS_Result                r;

    RJS_NEW(rt, resi);

    rjs_value_copy(rt, &resi->re, re);
    rjs_value_copy(rt, &resi->str, str);

    resi->global  = global;
    resi->unicode = unicode;
    resi->done    = RJS_FALSE;

    r = rjs_object_init(rt, iter, &resi->object, rjs_o_RegExpStringIteratorPrototype(realm),
            &regexp_str_iter_ops);
    if (r == RJS_ERR)
        RJS_DEL(rt, resi);

    return r;
}

/*RegExp.prototype[@@matchAll]*/
static RJS_NF(RegExp_prototype_matchAll)
{
    RJS_Value *str      = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm *realm    = rjs_realm_current(rt);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *s        = rjs_value_stack_push(rt);
    RJS_Value *c        = rjs_value_stack_push(rt);
    RJS_Value *flagsv   = rjs_value_stack_push(rt);
    RJS_Value *re       = rjs_value_stack_push(rt);
    RJS_Value *flags    = rjs_value_stack_push(rt);
    RJS_Value *matcher  = rjs_value_stack_push(rt);
    RJS_Value *idxv     = rjs_value_stack_push(rt);
    int64_t    idx;
    RJS_Bool   global, unicode;
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    if ((r = rjs_species_constructor(rt, thiz, rjs_o_RegExp(realm), c)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_flags(rt), flagsv)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, flagsv, flags)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, re, thiz);

    if ((r = rjs_construct(rt, c, re, 2, NULL, matcher)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_lastIndex(rt), idxv)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_length(rt, idxv, &idx)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, idxv, idx);

    if ((r = rjs_set(rt, matcher, rjs_pn_lastIndex(rt), idxv, RJS_TRUE)) == RJS_ERR)
        goto end;

    if (rjs_string_index_of_uchar(rt, flags, 'g', 0) != -1)
        global = RJS_TRUE;
    else
        global = RJS_FALSE;

    if (rjs_string_index_of_uchar(rt, flags, 'u', 0) != -1)
        unicode = RJS_TRUE;
    else
        unicode = RJS_FALSE;

    r = regexp_str_iter_new(rt, rv, matcher, str, global, unicode);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Invoke the replace function.*/
static RJS_Result
replace_func (RJS_Runtime *rt, RJS_Value *s, RJS_Value *fn, RJS_Value *mstr, size_t pos, RJS_Value *match, RJS_Value *rstr)
{
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *lenv   = rjs_value_stack_push(rt);
    RJS_Value *groups = rjs_value_stack_push(rt);
    RJS_Value *cap    = rjs_value_stack_push(rt);
    RJS_Value *res    = rjs_value_stack_push(rt);
    RJS_Value *args, *arg;
    size_t     argc;
    int64_t    i, len, aid;
    RJS_Result r;

    argc = 3;

    if ((r = rjs_get(rt, match, rjs_pn_length(rt), lenv)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_length(rt, lenv, &len)) == RJS_ERR)
        goto end;

    if (len > 1)
        argc += len - 1;

    if ((r = rjs_get(rt, match, rjs_pn_groups(rt), groups)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, groups))
        argc ++;

    args = rjs_value_stack_push_n(rt, argc);
    arg  = args;
    aid  = 0;

    rjs_value_copy(rt, arg, mstr);
    aid ++;

    for (i = 1; i < len; i ++) {
        if ((r = rjs_get_index(rt, match, i, cap)) == RJS_ERR)
            goto end;

        arg = rjs_value_buffer_item(rt, args, aid);
        aid ++;

        if ((r = rjs_to_string(rt, cap, arg)) == RJS_ERR)
            goto end;
    }

    arg = rjs_value_buffer_item(rt, args, aid);
    rjs_value_set_number(rt, arg, pos);
    aid ++;

    arg = rjs_value_buffer_item(rt, args, aid);
    rjs_value_copy(rt, arg, s);
    aid ++;

    if (!rjs_value_is_undefined(rt, groups)) {
        arg = rjs_value_buffer_item(rt, args, aid);
        aid ++;

        rjs_value_copy(rt, arg, groups);
    }

    if ((r = rjs_call(rt, fn, rjs_v_undefined(rt), args, argc, res)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, res, rstr)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*RegExp.prototype[@@replace]*/
static RJS_NF(RegExp_prototype_replace)
{
    RJS_Value *str     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *repv    = rjs_argument_get(rt, args, argc, 1);
    size_t     top     = rjs_value_stack_save(rt);
    RJS_Value *s       = rjs_value_stack_push(rt);
    RJS_Value *rep_str = rjs_value_stack_push(rt);
    RJS_Value *flagsv  = rjs_value_stack_push(rt);
    RJS_Value *flags   = rjs_value_stack_push(rt);
    RJS_Value *idxv    = rjs_value_stack_push(rt);
    RJS_Value *match   = rjs_value_stack_push(rt);
    RJS_Value *mstrv   = rjs_value_stack_push(rt);
    RJS_Value *mstr    = rjs_value_stack_push(rt);
    RJS_Value *rstr    = rjs_value_stack_push(rt);
    size_t     len, start;
    RJS_Bool   func_rep, global, unicode = RJS_FALSE;
    RJS_UCharBuffer  ucb;
    const RJS_UChar *chars;
    RJS_Result r;

    rjs_uchar_buffer_init(rt, &ucb);

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    len = rjs_string_get_length(rt, s);

    func_rep = rjs_is_callable(rt, repv);
    if (!func_rep) {
        if ((r = rjs_to_string(rt, repv, rep_str)) == RJS_ERR)
            goto end;
    }

    if ((r = rjs_get(rt, thiz, rjs_pn_flags(rt), flagsv)) == RJS_ERR)
        goto end;
    if ((r = rjs_to_string(rt, flagsv, flags)) == RJS_ERR)
        goto end;
    global = (rjs_string_index_of_uchar(rt, flags, 'g', 0) != -1);

    if (global) {
        unicode = (rjs_string_index_of_uchar(rt, flags, 'u', 0) != -1);

        rjs_value_set_number(rt, idxv, 0);

        if ((r = rjs_set(rt, thiz, rjs_pn_lastIndex(rt), idxv, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    chars = rjs_string_get_uchars(rt, s);
    start = 0;

    while (1) {
        RJS_Number posn;
        size_t     pos;
        int64_t    idx;

        if ((r = rjs_regexp_exec(rt, thiz, s, match)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, match))
            break;

        if ((r = rjs_get(rt, match, rjs_pn_index(rt), idxv)) == RJS_ERR)
            goto end;

        if ((r = rjs_to_integer_or_infinity(rt, idxv, &posn)) == RJS_ERR)
            goto end;

        pos = RJS_CLAMP(posn, 0, len);

        if (pos >= start) {
            if (pos > start) {
                rjs_uchar_buffer_append_uchars(rt, &ucb, chars + start, pos - start);
            }

            if ((r = rjs_get_index(rt, match, 0, mstrv)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, mstrv, mstr)) == RJS_ERR)
                goto end;

            if (func_rep) {
                r = replace_func(rt, s, repv, mstr, pos, match, rstr);
            } else {
                r = rjs_get_substitution(rt, s, pos, match, rep_str, rstr);
            }

            if (r == RJS_ERR)
                goto end;

            rjs_uchar_buffer_append_string(rt, &ucb, rstr);

            if ((r = rjs_get(rt, match, rjs_pn_index(rt), idxv)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_length(rt, idxv, &idx)) == RJS_ERR)
                goto end;

            start = idx + rjs_string_get_length(rt, mstr);

            if (!global)
                break;

            if (rjs_string_get_length(rt, mstr) == 0) {
                if ((r = rjs_get(rt, thiz, rjs_pn_lastIndex(rt), idxv)) == RJS_ERR)
                    goto end;

                if ((r = rjs_to_length(rt, idxv, &idx)) == RJS_ERR)
                    goto end;

                idx = adv_str_index(rt, s, idx, unicode);

                rjs_value_set_number(rt, idxv, idx);

                if ((r = rjs_set(rt, thiz, rjs_pn_lastIndex(rt), idxv, RJS_TRUE)) == RJS_ERR)
                    goto end;
            }
        }
    }

    if (start < len) {
        rjs_uchar_buffer_append_uchars(rt, &ucb, chars + start, len - start);
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*RegExp.prototype[@@search]*/
static RJS_NF(RegExp_prototype_search)
{
    RJS_Value *str           = rjs_argument_get(rt, args, argc, 0);
    size_t     top           = rjs_value_stack_save(rt);
    RJS_Value *s             = rjs_value_stack_push(rt);
    RJS_Value *prev_last_idx = rjs_value_stack_push(rt);
    RJS_Value *curr_last_idx = rjs_value_stack_push(rt);
    RJS_Value *res           = rjs_value_stack_push(rt);
    RJS_Value *zero          = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_lastIndex(rt), prev_last_idx)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, zero, 0);
    if (!rjs_same_value(rt, zero, prev_last_idx)) {
        if ((r = rjs_set(rt, thiz, rjs_pn_lastIndex(rt), zero, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    if ((r = rjs_regexp_exec(rt, thiz, s, res)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_lastIndex(rt), curr_last_idx)) == RJS_ERR)
        goto end;

    if (!rjs_same_value(rt, curr_last_idx, prev_last_idx)) {
        if ((r = rjs_set(rt, thiz, rjs_pn_lastIndex(rt), prev_last_idx, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    if (rjs_value_is_null(rt, res)) {
        rjs_value_set_number(rt, rv, -1);
    } else {
        if ((r = rjs_get(rt, res, rjs_pn_index(rt), rv)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*RegExp.prototype[@@split]*/
static RJS_NF(RegExp_prototype_split)
{
    RJS_Value *str      = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *limit    = rjs_argument_get(rt, args, argc, 1);
    RJS_Realm *realm    = rjs_realm_current(rt);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *s        = rjs_value_stack_push(rt);
    RJS_Value *c        = rjs_value_stack_push(rt);
    RJS_Value *flagsv   = rjs_value_stack_push(rt);
    RJS_Value *flags    = rjs_value_stack_push(rt);
    RJS_Value *re       = rjs_value_stack_push(rt);
    RJS_Value *nflags   = rjs_value_stack_push(rt);
    RJS_Value *tmp      = rjs_value_stack_push(rt);
    RJS_Value *splitter = rjs_value_stack_push(rt);
    RJS_Value *z        = rjs_value_stack_push(rt);
    RJS_Bool   unicode;
    int64_t    p, q;
    uint32_t   lim, size, len;
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    if ((r = rjs_species_constructor(rt, thiz, rjs_o_RegExp(realm), c)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_flags(rt), flagsv)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, flagsv, flags)) == RJS_ERR)
        goto end;

    if (rjs_string_index_of_uchar(rt, flags, 'u', 0) != -1)
        unicode = RJS_TRUE;
    else
        unicode = RJS_FALSE;

    if (rjs_string_index_of_uchar(rt, flags, 'y', 0) != -1) {
        rjs_value_copy(rt, nflags, flags);
    } else {
        rjs_string_from_chars(rt, tmp, "y", 1);
        rjs_string_concat(rt, flags, tmp, nflags);
    }

    rjs_value_copy(rt, re, thiz);
    if ((r = rjs_construct(rt, c, re, 2, NULL, splitter)) == RJS_ERR)
        goto end;

    if ((r = rjs_array_new(rt, rv, 0, NULL)) == RJS_ERR)
        goto end;

    len = 0;

    if (rjs_value_is_undefined(rt, limit)) {
        lim = 0xffffffff;
    } else {
        if ((r = rjs_to_uint32(rt, limit, &lim)) == RJS_ERR)
            goto end;
    }

    if (lim == 0) {
        r = RJS_OK;
        goto end;
    }

    size = rjs_string_get_length(rt, s);

    if (size == 0) {
        if ((r = rjs_regexp_exec(rt, splitter, s, z)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_null(rt, z)) {
            r = RJS_OK;
            goto end;
        }

        rjs_create_data_property_or_throw_index(rt, rv, 0, s);
        r = RJS_OK;
        goto end;
    }

    p = 0;
    q = p;

    while (q < size) {
        rjs_value_set_number(rt, tmp, q);

        if ((r = rjs_set(rt, splitter, rjs_pn_lastIndex(rt), tmp, RJS_TRUE)) == RJS_ERR)
            goto end;

        if ((r = rjs_regexp_exec(rt, splitter, s, z)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, z)) {
            q = adv_str_index(rt, s, q, unicode);
        } else {
            int64_t last_idx;
            size_t  e;

            if ((r = rjs_get(rt, splitter, rjs_pn_lastIndex(rt), tmp)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_length(rt, tmp, &last_idx)) == RJS_ERR)
                goto end;

            e = RJS_MIN(last_idx, size);

            if (e == p) {
                q = adv_str_index(rt, s, q, unicode);
            } else {
                int64_t i, ncap;

                rjs_string_substr(rt, s, p, q, tmp);
                rjs_create_data_property_or_throw_index(rt, rv, len, tmp);

                len ++;
                if (len == lim) {
                    r = RJS_OK;
                    goto end;
                }

                p = e;

                if ((r = rjs_length_of_array_like(rt, z, &ncap)) == RJS_ERR)
                    goto end;

                for (i = 1; i < ncap; i ++) {
                    if ((r = rjs_get_index(rt, z, i, tmp)) == RJS_ERR)
                        goto end;

                    rjs_create_data_property_or_throw_index(rt, rv, len, tmp);
                    len ++;
                    if (len == lim) {
                        r = RJS_OK;
                        goto end;
                    }
                }

                q = p;
            }
        }
    }

    rjs_string_substr(rt, s, p, size, tmp);
    rjs_create_data_property_or_throw_index(rt, rv, len, tmp);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*RegExp.prototype.test*/
static RJS_NF(RegExp_prototype_test)
{
    RJS_Value *s     = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    RJS_Value *match = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_string(rt, s, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_regexp_exec(rt, thiz, str, match)) == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, !rjs_value_is_null(rt, match));
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*RegExp.prototype.toString*/
static RJS_NF(RegExp_prototype_toString)
{
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *pat    = rjs_value_stack_push(rt);
    RJS_Value *flags  = rjs_value_stack_push(rt);
    RJS_UCharBuffer ucb;
    RJS_Result r;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_get(rt, thiz, rjs_pn_source(rt), pat)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, thiz, rjs_pn_flags(rt), flags)) == RJS_ERR)
        goto end;

    rjs_uchar_buffer_init(rt, &ucb);
    rjs_uchar_buffer_append_uchar(rt, &ucb, '/');
    rjs_uchar_buffer_append_string(rt, &ucb, pat);
    rjs_uchar_buffer_append_uchar(rt, &ucb, '/');
    rjs_uchar_buffer_append_string(rt, &ucb, flags);

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    rjs_uchar_buffer_deinit(rt, &ucb);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
regexp_prototype_function_descs[] = {
    {
        "exec",
        1,
        RegExp_prototype_exec
    },
    {
        "@@match",
        1,
        RegExp_prototype_match
    },
    {
        "@@matchAll",
        1,
        RegExp_prototype_matchAll
    },
    {
        "@@replace",
        2,
        RegExp_prototype_replace
    },
    {
        "@@search",
        1,
        RegExp_prototype_search
    },
    {
        "@@split",
        2,
        RegExp_prototype_split
    },
    {
        "test",
        1,
        RegExp_prototype_test
    },
    {
        "toString",
        0,
        RegExp_prototype_toString
    },
    {NULL}
};

/*Check if the regular expression has the flag.*/
static RJS_Result
regexp_has_flag (RJS_Runtime *rt, RJS_Value *thiz, RJS_Value *rv, int flag)
{
    RJS_Realm  *realm = rjs_realm_current(rt);
    RJS_Result  r;
    RJS_RegExp *re;
    RJS_Bool    b;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_REGEXP) {
        if (rjs_same_value(rt, thiz, rjs_o_RegExp_prototype(realm))) {
            rjs_value_set_undefined(rt, rv);
            r = RJS_OK;
            goto end;
        }

        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    re = (RJS_RegExp*)rjs_value_get_object(rt, thiz);
    b  = re->model->flags & flag;

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*get RegExp.prototype.dotAll*/
static RJS_NF(RegExp_prototype_dotAll_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_S);
}

/*get RegExp.prototype.flags*/
static RJS_NF(RegExp_prototype_flags_get)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *fv  = rjs_value_stack_push(rt);
    RJS_Result       r;
    RJS_UChar        chars[16];
    RJS_UChar       *c   = chars;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    if ((r = rjs_get(rt, thiz, rjs_pn_hasIndices(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 'd';

    if ((r = rjs_get(rt, thiz, rjs_pn_global(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 'g';

    if ((r = rjs_get(rt, thiz, rjs_pn_ignoreCase(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 'i';

    if ((r = rjs_get(rt, thiz, rjs_pn_multiline(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 'm';

    if ((r = rjs_get(rt, thiz, rjs_pn_dotAll(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 's';

    if ((r = rjs_get(rt, thiz, rjs_pn_unicode(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 'u';

    if ((r = rjs_get(rt, thiz, rjs_pn_sticky(rt), fv)) == RJS_ERR)
        goto end;
    if (rjs_to_boolean(rt, fv))
        *c ++ = 'y';

    r = rjs_string_from_uchars(rt, rv, chars, c - chars);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*get RegExp.prototype.global*/
static RJS_NF(RegExp_prototype_global_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_G);
}

/*get RegExp.prototype.hasIndices*/
static RJS_NF(RegExp_prototype_hasIndices_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_D);
}

/*get RegExp.prototype.ignoreCase*/
static RJS_NF(RegExp_prototype_ignoreCase_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_I);
}

/*get RegExp.prototype.multiline*/
static RJS_NF(RegExp_prototype_multiline_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_M);
}

/*get RegExp.prototype.source*/
static RJS_NF(RegExp_prototype_source_get)
{
    RJS_Realm       *realm = rjs_realm_current(rt);
    RJS_Result       r;
    RJS_RegExp      *re;
    RJS_RegExpModel *rem;

    if (!rjs_value_is_object(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_REGEXP) {
        if (rjs_same_value(rt, thiz, rjs_o_RegExp_prototype(realm))) {
            r = rjs_string_from_chars(rt, rv, "(?:)", -1);
            goto end;
        }

        r = rjs_throw_type_error(rt, _("the value is not a regular expression"));
        goto end;
    }

    re  = (RJS_RegExp*)rjs_value_get_object(rt, thiz);
    rem = re->model;

    rjs_value_copy(rt, rv, &rem->source);

    r = RJS_OK;
end:
    return r;
}

/*get RegExp.prototype_sticky*/
static RJS_NF(RegExp_prototype_sticky_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_Y);
}

/*get RegExp.prototype.unicode*/
static RJS_NF(RegExp_prototype_unicode_get)
{
    return regexp_has_flag(rt, thiz, rv, RJS_REGEXP_FL_U);
}

static const RJS_BuiltinAccessorDesc
regexp_prototype_accessor_descs[] = {
    {
        "dotAll",
        RegExp_prototype_dotAll_get
    },
    {
        "flags",
        RegExp_prototype_flags_get
    },
    {
        "global",
        RegExp_prototype_global_get
    },
    {
        "hasIndices",
        RegExp_prototype_hasIndices_get
    },
    {
        "ignoreCase",
        RegExp_prototype_ignoreCase_get
    },
    {
        "multiline",
        RegExp_prototype_multiline_get
    },
    {
        "source",
        RegExp_prototype_source_get
    },
    {
        "sticky",
        RegExp_prototype_sticky_get
    },
    {
        "unicode",
        RegExp_prototype_unicode_get
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
regexp_prototype_desc = {
    "RegExp",
    NULL,
    NULL,
    NULL,
    NULL,
    regexp_prototype_function_descs,
    regexp_prototype_accessor_descs,
    NULL,
    "RegExp_prototype"
};

static const RJS_BuiltinFieldDesc
regexp_str_iter_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "RegExp String Iterator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*RegExpStringIteratorPrototype.next*/
static RJS_NF(RegExpStringIteratorPrototype_next)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *mstrv = rjs_value_stack_push(rt);
    RJS_Value *mstr  = rjs_value_stack_push(rt);
    RJS_Value *res   = rjs_value_stack_push(rt);
    RJS_Value *idxv  = rjs_value_stack_push(rt);
    RJS_Bool   done  = RJS_FALSE;
    RJS_RegExpStringIterator *resi;
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_REGEXP_STRING_ITERATOR) {
        r = rjs_throw_type_error(rt, _("the value is not a regular expression string iterator"));
        goto end;
    }

    resi = (RJS_RegExpStringIterator*) rjs_value_get_object(rt, thiz);

    if (resi->done) {
        rjs_value_set_undefined(rt, res);
        done = RJS_TRUE;
    } else {
        if ((r = rjs_regexp_exec(rt, &resi->re, &resi->str, res)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, res)) {
            rjs_value_set_undefined(rt, res);
            resi->done = RJS_TRUE;
            done = RJS_TRUE;
        } else if (!resi->global) {
            resi->done = RJS_TRUE;
        } else {
            if ((r = rjs_get_index(rt, res, 0, mstrv)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, mstrv, mstr)) == RJS_ERR)
                goto end;

            if (rjs_string_get_length(rt, mstr) == 0) {
                int64_t idx;

                if ((r = rjs_get(rt, &resi->re, rjs_pn_lastIndex(rt), idxv)) == RJS_ERR)
                    goto end;

                if ((r = rjs_to_length(rt, idxv, &idx)) == RJS_ERR)
                    goto end;

                idx = adv_str_index(rt, &resi->str, idx, resi->unicode);

                rjs_value_set_number(rt, idxv, idx);

                if ((r = rjs_set(rt, &resi->re, rjs_pn_lastIndex(rt), idxv, RJS_TRUE)) == RJS_ERR)
                    goto end;
            }
        }
    }

    r = rjs_create_iter_result_object(rt, res, done, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
regexp_str_iter_prototype_function_descs[] = {
    {
        "next",
        0,
        RegExpStringIteratorPrototype_next
    },
    {NULL}
};
