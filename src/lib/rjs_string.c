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

/*Free the string.*/
static void
string_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_String *s = ptr;

    /*Remove the property key entry.*/
    if (s->flags & RJS_STRING_FL_PROP_KEY) {
        RJS_HashEntry *e, **pe;
        RJS_Result     r;

        r = rjs_hash_lookup(&rt->str_prop_key_hash, s, &e, &pe,
                &rjs_hash_string_ops, rt);
        assert(r == RJS_TRUE);

        rjs_hash_remove(&rt->str_prop_key_hash, pe, rt);

        RJS_DEL(rt, e);
    }

    /*Free the character buffer.*/
    if (!(s->flags & RJS_STRING_FL_STATIC)) {
        if (s->uchars)
            RJS_DEL_N(rt, s->uchars, s->length);
    }

    RJS_DEL(rt, s);
}

/*String GC operation functions.*/
static const RJS_GcThingOps
string_gc_ops = {
    RJS_GC_THING_STRING,
    NULL,
    string_op_gc_free
};

/*Get the unicode character from the string.*/
static inline const RJS_UChar*
string_get_uchars (RJS_String *s)
{
    return s->uchars;
}

/*Allocate a new string.*/
static RJS_String*
string_new (RJS_Runtime *rt, RJS_Value *v, int flags, size_t len)
{
    RJS_String *s;

    RJS_NEW(rt, s);

    s->flags  = flags;
    s->length = len;
    s->uchars = NULL;

    rjs_value_set_string(rt, v, s);

    rjs_gc_add(rt, s, &string_gc_ops);

    return s;
}

/**
 * Get the unicode code point at the position in the string.
 * \param rt The current runtime.
 * \param v The string value.
 * \param idx The character's index.
 * \return The unicode code point.
 */
int
rjs_string_get_uc (RJS_Runtime *rt, RJS_Value *v, size_t idx)
{
    RJS_String *s = rjs_value_get_string(rt, v);
    int         c1, c2, uc;

    assert(idx < s->length);

    c1 = s->uchars[idx];

    if (!rjs_uchar_is_leading_surrogate(c1) || (idx == s->length - 1)) {
        uc = c1;
    } else {
        c2 = s->uchars[idx + 1];

        if (rjs_uchar_is_trailing_surrogate(c2))
            uc = rjs_surrogate_pair_to_uc(c1, c2);
        else
            uc = c1;
    }

    return uc;
}

/**
 * Create a string from characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param chars The characters buffer.
 * \param len Characters in the buffer.
 * -1 means the characters are 0 terminated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_from_chars (RJS_Runtime *rt, RJS_Value *v, const char *chars, size_t len)
{
    RJS_String *s;
    size_t      i;

    if (len == (size_t)-1)
        len = strlen(chars);

    s = string_new(rt, v, 0, len);

    if (len) {
        s->uchars = rjs_alloc(rt, sizeof(RJS_UChar) * len);
        if (!s->uchars)
            return rjs_throw_range_error(rt, _("string is too long"));

        if (chars) {
            for (i = 0; i < len; i ++)
                s->uchars[i] = chars[i];
        }
    }

    return RJS_OK;
}

/**
 * Create a string from encoded characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param chars The characters buffer.
 * \param len Characters in the buffer.
 * -1 means the characters are 0 terminated.
 * \param enc The encoding of the character.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_from_enc_chars (RJS_Runtime *rt, RJS_Value *v,
        const char *chars, size_t len, const char *enc)
{
    RJS_Conv        conv;
    RJS_CharBuffer  cb;
    RJS_Result      r;
    RJS_String     *s;
    size_t          uc_len;
    RJS_Bool        conv_init = RJS_FALSE;

    if (!enc)
        enc = RJS_ENC_UTF8;

    if (len == (size_t)-1)
        len = strlen(chars);

    rjs_char_buffer_init(rt, &cb);

    if ((r = rjs_conv_init(rt, &conv, enc, RJS_ENC_UCHAR)) == RJS_ERR)
        goto end;
    conv_init = RJS_TRUE;

    if ((r = rjs_conv_to_buffer(rt, &conv, chars, len, &cb)) == RJS_ERR)
        goto end;

    uc_len = cb.item_num / sizeof(RJS_UChar);

    s = string_new(rt, v, 0, uc_len);

    if (uc_len) {
        s->uchars = rjs_alloc(rt, sizeof(RJS_UChar) * uc_len);
        if (!s->uchars) {
            r = rjs_throw_range_error(rt, _("string is too long"));
            goto end;
        }

        RJS_ELEM_CPY(s->uchars, (RJS_UChar*)cb.items, uc_len);
    }

    r = RJS_OK;
end:
    if (conv_init)
        rjs_conv_deinit(rt, &conv);

    rjs_char_buffer_deinit(rt, &cb);

    return r;
}

/**
 * Create a string from unicode characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param uchars The unicode characters buffer.
 * \param len Unicode characters in the buffer.
 * -1 means the unicode characters are 0 terminated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_from_uchars (RJS_Runtime *rt, RJS_Value *v,
        const RJS_UChar *uchars, size_t len)
{
    RJS_String *s;

    if (len == (size_t)-1) {
        const RJS_UChar *uc = uchars;

        len = 0;

        while (*uc) {
            len ++;
            uc ++;
        }
    }

    s = string_new(rt, v, 0, len);

    if (len) {
        s->uchars = rjs_alloc(rt, sizeof(RJS_UChar) * len);
        if (!s->uchars)
            return rjs_throw_range_error(rt, _("string is too long"));

        if (uchars)
            RJS_ELEM_CPY(s->uchars, uchars, len);
    }

    return RJS_OK;
}

/**
 * Create a string from static unicode characters.
 * \param rt The current runtime.
 * \param v The value to store the string.
 * \param uchars The unicode characters buffer.
 * \param len Unicode characters in the buffer.
 * -1 means the unicode characters are 0 terminated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_from_static_uchars (RJS_Runtime *rt, RJS_Value *v,
        const RJS_UChar *uchars, size_t len)
{
    RJS_String *s;

    if (len == (size_t)-1) {
        const RJS_UChar *uc = uchars;

        len = 0;

        while (*uc) {
            len ++;
            uc ++;
        }
    }

    s = string_new(rt, v, RJS_STRING_FL_STATIC, len);

    s->uchars = (RJS_UChar*)uchars;

    return RJS_OK;
}

/**
 * Load a string from a file.
 * \param rt The current runtime.
 * \param[out] str The result string.
 * \param fn The filename.
 * \param enc The character encoding of the file.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_from_file (RJS_Runtime *rt, RJS_Value *str, const char *fn, const char *enc)
{
    FILE           *fp        = NULL;
    RJS_Bool        conv_init = RJS_FALSE;
    size_t          left      = 0;
    char            buf[1024];
    size_t          n;
    RJS_Result      r;
    RJS_Conv        conv;
    RJS_UCharBuffer ucb;

    rjs_uchar_buffer_init(rt, &ucb);

    if (!(fp = fopen(fn, "rb"))) {
        RJS_LOGE("cannot open file \"%s\"", fn);
        r = RJS_ERR;
        goto end;
    }

    if (!enc)
        enc = RJS_ENC_UTF8;

    if ((r = rjs_conv_init(rt, &conv, enc, RJS_ENC_UCHAR)) == RJS_ERR)
        goto end;
    conv_init = RJS_TRUE;

    while (1) {
        const char *in;
        char       *out;
        size_t      in_left, out_left, out_cap;

        n = fread(buf + left, 1, sizeof(buf) - left, fp);
        if (!n) {
            if (feof(fp) && !left)
                break;

            if (ferror(fp)) {
                RJS_LOGE("file read error");
                r = RJS_ERR;
                goto end;
            }
        }

        left += n;

        out_left = ucb.item_cap - ucb.item_num;
        if (out_left < left) {
            size_t cap = RJS_MAX(ucb.item_num + left, ucb.item_cap * 2);

            rjs_vector_set_capacity(&ucb, cap, rt);

            out_left = ucb.item_cap - ucb.item_num;
        }

        in        = buf;
        in_left   = left;
        out       = (char*)(ucb.items + ucb.item_num);
        out_cap   = out_left;
        out_left *= sizeof(RJS_UChar);

        if ((r = rjs_conv_run(rt, &conv, &in, &in_left, &out, &out_left)) == RJS_ERR)
            goto end;

        out_left /= sizeof(RJS_UChar);
        if (out_left == out_cap) {
            r = RJS_ERR;
            goto end;
        }

        ucb.item_num = ucb.item_cap - out_left;

        if (in_left)
            memmove(buf, buf + left - in_left, in_left);

        left = in_left;
    }

    r = rjs_string_from_uchars(rt, str, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);

    if (conv_init)
        rjs_conv_deinit(rt, &conv);
    if (fp)
        fclose(fp);
    return r;
}

/**
 * Convert the string to encoded characters.
 * \param rt The current runtime.
 * \param v The value of the string.
 * \param cb The output character buffer.
 * \param enc The encoding.
 * \return 0 terminated encoded characters.
 */
const char*
rjs_string_to_enc_chars (RJS_Runtime *rt, RJS_Value *v,
        RJS_CharBuffer *cb, const char *enc)
{
    RJS_String *s;
    RJS_Conv    conv;
    RJS_Result  r;
    RJS_Bool    conv_init = RJS_FALSE;
    const char *cstr      = NULL;

    if (!enc)
        enc = RJS_ENC_UTF8;
    if (!cb)
        cb = &rt->tmp_cb;

    assert(rjs_value_is_string(rt, v));

    s = rjs_value_get_string(rt, v);

    if ((r = rjs_conv_init(rt, &conv, RJS_ENC_UCHAR, enc)) == RJS_ERR)
        goto end;
    conv_init = RJS_TRUE;

    rjs_char_buffer_clear(rt, cb);

    rjs_conv_to_buffer(rt, &conv, (char*)s->uchars, s->length * sizeof(RJS_UChar), cb);

    cstr = rjs_char_buffer_to_c_string(rt, cb);
end:
    if (conv_init)
        rjs_conv_deinit(rt, &conv);

    return cstr;
}

/**
 * Convert a string to property key.
 * The property key is unique if the string values are same.
 * \param rt The rt.
 * \param v The string value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_to_property_key (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_String *s;

    assert(rjs_value_is_string(rt, v));

    s = rjs_value_get_string(rt, v);

    if (!(s->flags & RJS_STRING_FL_PROP_KEY)) {
        RJS_HashEntry *e, **pe;
        RJS_Result     r;

        r = rjs_hash_lookup(&rt->str_prop_key_hash, s, &e, &pe,
                &rjs_hash_string_ops, rt);
        if (r == RJS_TRUE) {
            s = e->key;

            rjs_value_set_string(rt, v, s);
        } else {
            RJS_NEW(rt, e);

            rjs_hash_insert(&rt->str_prop_key_hash, s, e, pe,
                    &rjs_hash_string_ops, rt);

            s->flags |= RJS_STRING_FL_PROP_KEY;
        }
    }

    return RJS_OK;
}

/**
 * Convert the string to array index.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[out] pi Return the index value.
 * \retval RJS_TRUE The string is an array index.
 * \retval RJS_FALSE The string is not an array index.
 */
RJS_Bool
rjs_string_to_index_internal (RJS_Runtime *rt, RJS_Value *v, int64_t *pi)
{
    RJS_String      *s;
    const RJS_UChar *uc;
    size_t           len;
    int64_t          n;
    RJS_Bool         is_idx = RJS_FALSE;

    assert(rjs_value_is_string(rt, v));

    s   = rjs_value_get_string(rt, v);
    uc  = string_get_uchars(s);
    len = s->length;

    while (len) {
        if (!rjs_uchar_is_white_space(*uc))
            break;

        uc  ++;
        len --;
    }

    if (!len)
        goto end;
        
    if (!rjs_uchar_is_digit(*uc))
        goto end;

    n = 0;
    while (len) {
        if (!rjs_uchar_is_digit(*uc))
            break;

        n *= 10;
        n += *uc - '0';

        if (n > 0xfffffffe)
            goto end;

        uc  ++;
        len --;
    }

    while (len) {
        if (!rjs_uchar_is_white_space(*uc))
            goto end;

        uc  ++;
        len --;
    }

    is_idx = RJS_TRUE;
    *pi    = n;
end:
    if (!is_idx)
        s->flags |= RJS_STRING_FL_NOT_INDEX;

    return is_idx;
}

/**
 * Check if the string is a canonical numeric index string.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[pn] pn Return the number value.
 * \retval RJS_TRUE The string is a numeric index string.
 * \retval RJS_FALSE The string is not a numeric index string.
 */
RJS_Bool
rjs_canonical_numeric_index_string_internal (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn)
{
    RJS_Result r;
    RJS_Number n;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *nv  = rjs_value_stack_push(rt);
    RJS_Value *sv  = rjs_value_stack_push(rt);

    if (rjs_string_get_length(rt, v) == 2) {
        const RJS_UChar *c = rjs_string_get_uchars(rt, v);

        if ((c[0] == '-') && (c[1] == '0')) {
            *pn = -0.;
            r = RJS_TRUE;
            goto end;
        }
    }

    rjs_to_number(rt, v, &n);

    rjs_value_set_number(rt, nv, n);
    rjs_to_string(rt, nv, sv);

    r = rjs_string_equal(rt, sv, v);
    if (!r) {
        RJS_String *s = rjs_value_get_string(rt, v);

        s->flags |= RJS_STRING_FL_NOT_NUMBER;
    } else if (pn) {
        *pn = n;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the string to a value.*/
static RJS_Result
parse_string (RJS_Runtime *rt, RJS_Value *str, RJS_Bool is_big_int, RJS_Value *v)
{
    RJS_Input   si;
    RJS_Lex     lex;
    RJS_Token   tok;
    RJS_Result  r;
    int         sign = 0;

    if ((r = rjs_string_input_init(rt, &si, str)) == RJS_ERR)
        return r;

    rjs_token_init(rt, &tok);

    rjs_lex_init(rt, &lex, &si);

    si.flags   |= RJS_INPUT_FL_NO_MSG;
    lex.status &= ~RJS_LEX_ST_FIRST_TOKEN;
    lex.status |= RJS_LEX_ST_NO_MSG|RJS_LEX_ST_NO_SEP|RJS_LEX_ST_NO_LEGACY_OCT;

    if (is_big_int)
        lex.flags |= RJS_LEX_FL_BIG_INT;

    if ((r = rjs_lex_get_token(rt, &lex, &tok)) == RJS_ERR)
        goto end;

    if (tok.type == RJS_TOKEN_plus) {
        sign = 1;
        if ((r = rjs_lex_get_token(rt, &lex, &tok)) == RJS_ERR)
            goto end;
    } else if (tok.type == RJS_TOKEN_minus) {
        sign = 2;
        if ((r = rjs_lex_get_token(rt, &lex, &tok)) == RJS_ERR)
            goto end;
    }

    if (tok.type == RJS_TOKEN_NUMBER) {
        if (sign && !(tok.flags & RJS_TOKEN_FL_DECIMAL)) {
            r = RJS_FALSE;
            goto end;
        }

#if ENABLE_BIG_INT
        if (is_big_int) {
            if (rjs_value_is_big_int(rt, tok.value)) {
                if (sign == 2) {
                    rjs_big_int_unary_minus(rt, tok.value, v);
                } else {
                    rjs_value_copy(rt, v, tok.value);
                }
            } else {
                int64_t i = rjs_value_get_number(rt, tok.value);

                if (sign == 2)
                    i = -i;

                rjs_big_int_from_int64(rt, v, i);
            }
        } else
#endif /*ENABLE_BIG_INT*/
        {
            if (rjs_value_is_number(rt, tok.value)) {
                RJS_Number n = rjs_value_get_number(rt, tok.value);

                if (sign == 2)
                    n = -n;

                rjs_value_set_number(rt, v, n);
            } else {
                r = RJS_FALSE;
                goto end;
            }
        }

        if ((r = rjs_lex_get_token(rt, &lex, &tok)) == RJS_ERR)
            goto end;
    } else if (!is_big_int
            && (tok.type == RJS_TOKEN_IDENTIFIER)
            && rjs_string_equal(rt, tok.value, rjs_s_Infinity(rt))) {
        rjs_value_set_number(rt, v, (sign == 2) ? -INFINITY : INFINITY);

        if ((r = rjs_lex_get_token(rt, &lex, &tok)) == RJS_ERR)
            goto end;
    } else {
        if (sign) {
            r = RJS_FALSE;
            goto end;
        }

#if ENABLE_BIG_INT
        if (is_big_int)
            rjs_big_int_from_int(rt, v, 0);
        else
#endif /*ENABLE_BIG_INT*/
            rjs_value_set_number(rt, v, (sign == 2) ? -0. : 0.);
    }

    r = RJS_OK;

    if (tok.type != RJS_TOKEN_END)
        r = RJS_FALSE;

    if (rjs_lex_error(&lex))
        r = RJS_FALSE;

end:
    if (r == RJS_FALSE) {
#if ENABLE_BIG_INT
        if (is_big_int)
            rjs_value_set_undefined(rt, v);
        else
#endif /*ENABLE_BIG_INT*/
            rjs_value_set_number(rt, v, NAN);
    }

    rjs_lex_deinit(rt, &lex);
    rjs_token_deinit(rt, &tok);
    rjs_input_deinit(rt, &si);
    return RJS_OK;
}

/**
 * Convert a string to number.
 * \param rt The current runtime.
 * \param v The string value.
 * \return The number.
 */
RJS_Number
rjs_string_to_number (RJS_Runtime *rt, RJS_Value *v)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *nv  = rjs_value_stack_push(rt);
    RJS_Number n;

    parse_string(rt, v, RJS_FALSE, nv);

    n = rjs_value_get_number(rt, nv);

    rjs_value_stack_restore(rt, top);
    return n;
}

#if ENABLE_BIG_INT

/**
 * Convert a string to big integer.
 * \param rt The current runtime.
 * \param v The string value.
 * \param[out] bi Return the big integer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_to_big_int (RJS_Runtime *rt, RJS_Value *v, RJS_Value *bi)
{
    return parse_string(rt, v, RJS_TRUE, bi);
}

#endif /*ENABLE_BIG_INT*/

#include <rjs_string_table_inc.c>

/**
 * Lookup the internal symbol by its name.
 * \param rt The current runtime.
 * \param name The name of the symbol.
 * \return The symbol value.
 */
RJS_Value*
rjs_internal_symbol_lookup (RJS_Runtime *rt, const char *name)
{
    int          id = RJS_PN_STR_MAX;
    const char **n  = sym_name_table;

    while (*n) {
        if (!strcmp(*n, name))
            return &rt->prop_name_values[id];

        n  ++;
        id ++;
    }

    return NULL;
}

/**
 * Initialize the string resource in the rt.
 * \param rt The rt to be initialized.
 */
void
rjs_runtime_string_init (RJS_Runtime *rt)
{
    RJS_PropertyName *pn;
    const char      **pcstr;
    RJS_Value        *v;
    size_t            top = rjs_value_stack_save(rt);
    RJS_Value        *tmp = rjs_value_stack_push(rt);

    /*Initialize properties hash table.*/
    rjs_hash_init(&rt->str_prop_key_hash);

    /*Create internal strings.*/
    pcstr = string_table;
    v     = rt->strings;
    while (*pcstr) {
        rjs_string_from_chars(rt, v, *pcstr, -1);
        rjs_string_to_property_key(rt, v);

        pcstr ++;
        v     ++;
    }

    /*Create property names.*/
    pcstr = str_prop_table;
    v     = rt->prop_name_values;
    pn    = rt->prop_names;
    while (*pcstr) {
        rjs_string_from_chars(rt, v, *pcstr, -1);
        rjs_string_to_property_key(rt, v);
        rjs_property_name_init(rt, pn, v);

        pcstr ++;
        v     ++;
        pn    ++;
    }

    pcstr = sym_prop_table;
    while (*pcstr) {
        rjs_string_from_chars(rt, tmp, *pcstr, -1);
        rjs_symbol_new(rt, v, tmp);
        rjs_property_name_init(rt, pn, v);

        pcstr ++;
        v     ++;
        pn    ++;
    }

    rjs_value_stack_restore(rt, top);
}

/**
 * Release the string resource in the rt.
 * \param rt The rt to be released.
 */
void
rjs_runtime_string_deinit (RJS_Runtime *rt)
{
    size_t i;

    rjs_hash_deinit(&rt->str_prop_key_hash, &rjs_hash_string_ops, rt);

    for (i = 0; i < RJS_PN_MAX; i ++) {
        RJS_PropertyName *pn;

        pn = &rt->prop_names[i];

        rjs_property_name_deinit(rt, pn);
    }
}

/**
 * Scan the internal strings in the rt.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_internal_strings (RJS_Runtime *rt)
{
    rjs_gc_scan_value_buffer(rt, rt->strings, RJS_S_MAX);
    rjs_gc_scan_value_buffer(rt, rt->prop_name_values, RJS_PN_MAX);
}

/*Calculate the string key hash code.*/
static size_t
hash_op_string_key (void *data, void *key)
{
    RJS_String *s   = key;
    size_t      len = s->length;
    size_t      v;
    const RJS_UChar *c, *ec;

    if (!len)
        return 0;

    v = 0x19781009;

    c  = string_get_uchars(s);
    ec = c + len;

    while (c < ec) {
        v = (v << 5) | *c;
        c ++;
    }

    return v;
}

/*Check 2 string keys are equal.*/
static RJS_Bool
hash_op_string_equal (void *data, void *k1, void *k2)
{
    RJS_String *s1 = k1;
    RJS_String *s2 = k2;
    size_t      l1, l2;
    const RJS_UChar *c1, *c2, *ec1;

    if (s1 == s2)
        return RJS_TRUE;

    if ((s1->flags & RJS_STRING_FL_PROP_KEY)
            && (s2->flags & RJS_STRING_FL_PROP_KEY))
        return RJS_FALSE;

    l1 = s1->length;
    l2 = s2->length;
    if (l1 != l2)
        return RJS_FALSE;

    c1  = string_get_uchars(s1);
    c2  = string_get_uchars(s2);
    ec1 = c1 + l1;

    while (c1 < ec1) {
        if (*c1 != *c2)
            return RJS_FALSE;

        c1 ++;
        c2 ++;
    }

    return RJS_TRUE;
}

/**String key type hash table operation functions.*/
const RJS_HashOps
rjs_hash_string_ops = {
    rjs_hash_op_realloc,
    hash_op_string_key,
    hash_op_string_equal
};

/**
 * Check if 2 strings are equal.
 * \param rt The current runtime.
 * \param s1 String value 1.
 * \param s2 String value 2.
 * \retval RJS_TRUE s1 == s1.
 * \retval RJS_FALSE s1 != s2.
 */
RJS_Bool
rjs_string_equal (RJS_Runtime *rt, RJS_Value *s1, RJS_Value *s2)
{
    if (rjs_value_is_index_string(rt, s1)
            && rjs_value_is_index_string(rt, s2)) {
        return rjs_value_get_index_string(rt, s1)
                == rjs_value_get_index_string(rt, s2);
    }
    
    return hash_op_string_equal(rt,
            rjs_value_get_string(rt, s1),
            rjs_value_get_string(rt, s2));
}

/**
 * Get the hash key of a string value.
 * \param rt The current runtime.
 * \param v The string value.
 * \return The hash key code of the string.
 */
size_t
rjs_string_hash_key (RJS_Runtime *rt, RJS_Value *v)
{
    return hash_op_string_key(rt, rjs_value_get_string(rt, v));
}

/**
 * Get the substring.
 * \param rt The current runtime.
 * \param orig The origin string.
 * \param start The substring's start position.
 * \param end The substring's last character's position + 1.
 * \param[out] sub Return the substring.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_substr (RJS_Runtime *rt, RJS_Value *orig, size_t start, size_t end, RJS_Value *sub)
{
    size_t           len;
    const RJS_UChar *cb;

    assert(rjs_value_is_string(rt, orig));

    len = rjs_string_get_length(rt, orig);

    assert((end <= len) && (end >= start));

    if ((start == 0) && (end == len)) {
        rjs_value_copy(rt, sub, orig);
        return RJS_OK;
    }

    cb = rjs_string_get_uchars(rt, orig);

    return rjs_string_from_uchars(rt, sub, cb + start, end - start);
}

/**
 * Concatenate 2 strings.
 * \param rt The current runtime.
 * \param s1 String 1.
 * \param s2 String 2.
 * \param[out] sr Return the new string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_concat (RJS_Runtime *rt, RJS_Value *s1, RJS_Value *s2, RJS_Value *sr)
{
    size_t           l1, l2, l;
    RJS_UChar       *d;
    const RJS_UChar *c1, *c2;
    RJS_Result       r;

    assert(rjs_value_is_string(rt, s1));
    assert(rjs_value_is_string(rt, s2));

    l1 = rjs_string_get_length(rt, s1);
    l2 = rjs_string_get_length(rt, s2);

    if (!l1) {
        rjs_value_copy(rt, sr, s2);
        return RJS_OK;
    }

    if (!l2) {
        rjs_value_copy(rt, sr, s1);
        return RJS_OK;
    }

    l = l1 + l2;

    if ((r = rjs_string_from_uchars(rt, sr, NULL, l)) == RJS_ERR)
        return r;

    d  = (RJS_UChar*)rjs_string_get_uchars(rt, sr);
    c1 = rjs_string_get_uchars(rt, s1);
    c2 = rjs_string_get_uchars(rt, s2);

    RJS_ELEM_CPY(d, c1, l1);
    RJS_ELEM_CPY(d + l1, c2, l2);

    return RJS_OK;
}

/**
 * Compare 2 strings.
 * \param rt The current runtime.
 * \param v1 String value 1.
 * \param v2 String value 2.
 * \retval RJS_COMPARE_EQUAL v1 == v2.
 * \retval RJS_COMPARE_LESS v1 < v2.
 * \retval RJS_COMPARE_GREATER v1 > v2.
 * \retval RJS_ERR On error.
 */
RJS_CompareResult
rjs_string_compare (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    const RJS_UChar *c1, *c2;
    size_t           l1, l2;

    assert(rjs_value_is_string(rt, v1));
    assert(rjs_value_is_string(rt, v2));

    l1 = rjs_string_get_length(rt, v1);
    l2 = rjs_string_get_length(rt, v2);

    c1 = rjs_string_get_uchars(rt, v1);
    c2 = rjs_string_get_uchars(rt, v2);

    while (l1 && l2) {
        int r = *c1 - *c2;

        if (r < 0)
            return RJS_COMPARE_LESS;
        if (r > 0)
            return RJS_COMPARE_GREATER;

        c1 ++;
        c2 ++;
        l1 --;
        l2 --;
    }

    if (!l1 && !l2)
        return RJS_COMPARE_EQUAL;

    if (l1)
        return RJS_COMPARE_GREATER;

    return RJS_COMPARE_LESS;
}

/**
 * Trim the space characters of the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param flags The trim flags.
 * \param[out] Return the result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_trim (RJS_Runtime *rt, RJS_Value *str, int flags, RJS_Value *rstr)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *s   = rjs_value_stack_push(rt);
    size_t           len;
    const RJS_UChar *c, *b, *e;
    RJS_Result       r;

    if ((r = rjs_require_object_coercible(rt, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, str, s)) == RJS_ERR)
        goto end;

    c   = rjs_string_get_uchars(rt, s);
    len = rjs_string_get_length(rt, s);
    b   = c;
    e   = b + len;

    if (flags & RJS_STRING_TRIM_START) {
        while (b < e) {
            if (!rjs_uchar_is_white_space(*b))
                break;
            b ++;
        }
    }

    if (flags & RJS_STRING_TRIM_END) {
        while (e > b) {
            if (!rjs_uchar_is_white_space(e[-1]))
                break;
            e --;
        }
    }

    r = rjs_string_substr(rt, s, b - c, e - c, rstr);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Pad the substring at the beginning or end of the string.
 * \param rt The current runtime.
 * \param o The string object.
 * \param max_len The maximum length of the result.
 * \param fill_str The pattern string.
 * \param pos The pad position.
 * \param[out] rs The result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_string_pad (RJS_Runtime *rt, RJS_Value *o, RJS_Value *max_len, RJS_Value *fill_str,
        RJS_StringPadPosition pos, RJS_Value *rs)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *str  = rjs_value_stack_push(rt);
    RJS_Value       *pstr = rjs_value_stack_push(rt);
    int64_t          ilen;
    size_t           slen, plen;
    RJS_UChar       *d;
    const RJS_UChar *s;
    RJS_Result       r;

    if ((r = rjs_to_string(rt, o, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_length(rt, max_len, &ilen)) == RJS_ERR)
        goto end;

    slen = rjs_string_get_length(rt, str);

    if (ilen < slen) {
        rjs_value_copy(rt, rs, str);
        r = RJS_OK;
        goto end;
    }

    if (rjs_value_is_undefined(rt, fill_str)) {
        pstr = rjs_s_space(rt);
    } else {
        if ((r = rjs_to_string(rt, fill_str, pstr)) == RJS_ERR)
            goto end;
    }

    plen = rjs_string_get_length(rt, pstr);

    if (plen == 0) {
        rjs_value_copy(rt, rs, str);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_string_from_uchars(rt, rs, NULL, ilen)) == RJS_ERR)
        goto end;

    d = (RJS_UChar*)rjs_string_get_uchars(rt, rs);

    if (pos == RJS_STRING_PAD_END) {
        s = rjs_string_get_uchars(rt, str);
        RJS_ELEM_CPY(d, s, slen);
        d += slen;
    }

    ilen -= slen;
    s     = rjs_string_get_uchars(rt, pstr);

    while (ilen > 0) {
        if (ilen >= plen) {
            RJS_ELEM_CPY(d, s, plen);
            d    += plen;
            ilen -= plen;
        } else {
            RJS_ELEM_CPY(d, s, ilen);
            d    += ilen;
            ilen  = 0;
        }
    }

    if (pos == RJS_STRING_PAD_START) {
        s = rjs_string_get_uchars(rt, str);
        RJS_ELEM_CPY(d, s, slen);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Search the unicode character in the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param uc The unicode character to be searched.
 * \param pos Searching start position.
 * \return The character's index in the string.
 * \retval -1 Cannot find the character.
 */
ssize_t
rjs_string_index_of_uchar (RJS_Runtime *rt, RJS_Value *str, RJS_UChar uc, size_t pos)
{
    size_t           len;
    const RJS_UChar *b, *e, *c;

    len = rjs_string_get_length(rt, str);
    b   = rjs_string_get_uchars(rt, str);
    c   = b + pos;
    e   = b + len;

    while (c < e) {
        if (*c == uc)
            return c - b;

        c ++;
    }

    return -1;
}

/**
 * Search the substring in the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param sub The substring to be searched.
 * \param pos Searching start position.
 * \return The substring's first character's index.
 * \retval -1 Cannot find the substring.
 */
ssize_t
rjs_string_index_of (RJS_Runtime *rt, RJS_Value *str, RJS_Value *sub, size_t pos)
{
    size_t           len, slen;
    const RJS_UChar *b, *e, *c, *sb, *se;

    len  = rjs_string_get_length(rt, str);
    slen = rjs_string_get_length(rt, sub);

    if (!slen && (pos <= len))
        return pos;

    if ((pos > len) || (len - pos < slen))
        return -1;

    b  = rjs_string_get_uchars(rt, str);
    sb = rjs_string_get_uchars(rt, sub);
    c  = b + pos;
    e  = b + len - slen;
    se = sb + slen;

    while (c <= e) {
        const RJS_UChar *t     = c;
        const RJS_UChar *sc    = sb;
        RJS_Bool         match = RJS_TRUE;

        while (sc < se) {
            if (*sc != *t) {
                match = RJS_FALSE;
                break;
            }

            sc ++;
            t  ++;
        }

        if (match)
            return c - b;

        c ++;
    }

    return -1;
}

/**
 * Search the last substring in the string.
 * \param rt The current runtime.
 * \param str The string.
 * \param sub The substring to be searched.
 * \param pos Searching start position.
 * \return The substring's first character's index.
 * \retval -1 Cannot find the substring.
 */
ssize_t
rjs_string_last_index_of (RJS_Runtime *rt, RJS_Value *str, RJS_Value *sub, size_t pos)
{
    size_t           len, slen;
    const RJS_UChar *b, *c, *sb, *se;

    len  = rjs_string_get_length(rt, str);
    slen = rjs_string_get_length(rt, sub);

    if (len < slen)
        return -1;

    pos = RJS_MIN(pos, len - slen);

    if (!slen)
        return pos;

    b  = rjs_string_get_uchars(rt, str);
    sb = rjs_string_get_uchars(rt, sub);
    c  = b + pos;
    se = sb + slen;

    while (c >= b) {
        const RJS_UChar *t     = c;
        const RJS_UChar *sc    = sb;
        RJS_Bool         match = RJS_TRUE;

        while (sc < se) {
            if (*sc != *t) {
                match = RJS_FALSE;
                break;
            }

            sc ++;
            t  ++;
        }

        if (match)
            return c - b;

        c --;
    }

    return -1;
}

/**
 * Convert an index string to normal string.
 * \param rt The current runtime.
 * \param v The index string value, and return the normal string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_index_string_normalize (RJS_Runtime *rt, RJS_Value *v)
{
    uint32_t i;
    char     buf[32];

    assert(rjs_value_is_index_string(rt, v));

    i = rjs_value_get_index_string(rt, v);

    snprintf(buf, sizeof(buf), "%u", i);

    return rjs_string_from_chars(rt, v, buf, -1);
}

/**
 * Get the substitution.
 * \param rt The current runtime.
 * \param str The origin string.
 * \param pos The position of the matched string.
 * \param captures The captured substrings.
 * \param rep_templ The replace template string.
 * \retval rv The result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_substitution (RJS_Runtime *rt, RJS_Value *str, size_t pos,
        RJS_Value *captures, RJS_Value *rep_templ, RJS_Value *rv)
{
    RJS_UCharBuffer  ucb;
    const RJS_UChar *c, *b, *e, *nb, *ne;
    size_t           len, slen, mlen;
    int64_t          cn, n;
    RJS_Result       r;
    RJS_Value       *cap;
    RJS_Value       *caps   = NULL;
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *m      = rjs_value_stack_push(rt);
    RJS_Value       *mstr   = rjs_value_stack_push(rt);
    RJS_Value       *sub    = rjs_value_stack_push(rt);
    RJS_Value       *sstr   = rjs_value_stack_push(rt);
    RJS_Value       *groups = rjs_value_stack_push(rt);
    RJS_Value       *gobj   = rjs_value_stack_push(rt);
    RJS_Value       *name   = rjs_value_stack_push(rt);

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_length_of_array_like(rt, captures, &cn)) == RJS_ERR)
        goto end;

    if ((r = rjs_get_index_v(rt, captures, 0, m)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, m, mstr)) == RJS_ERR)
        goto end;

    if (cn > 1) {
        caps = rjs_value_stack_push_n(rt, cn - 1);

        for (n = 1; n < cn; n ++) {
            if ((r = rjs_get_index_v(rt, captures, n, m)) == RJS_ERR)
                goto end;

            cap = rjs_value_buffer_item(rt, caps, n - 1);

            if (!rjs_value_is_undefined(rt, m)) {
                if ((r = rjs_to_string(rt, m, cap)) == RJS_ERR)
                    goto end;
            }
        }
    }

    if ((r = rjs_get_v(rt, captures, rjs_pn_groups(rt), groups)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, groups)) {
        if ((r = rjs_to_object(rt, groups, gobj)) == RJS_ERR)
            goto end;

        rjs_value_copy(rt, groups, gobj);
    }

    slen = rjs_string_get_length(rt, str);
    mlen = rjs_string_get_length(rt, mstr);

    b   = rjs_string_get_uchars(rt, rep_templ);
    len = rjs_string_get_length(rt, rep_templ);
    c   = b;
    e   = b + len;

    while (c < e) {
        if (*c == '$') {
            c ++;

            if (c < e) {
                switch (*c) {
                case '$':
                    rjs_uchar_buffer_append_uchar(rt, &ucb, '$');
                    c ++;
                    break;
                case '`':
                    rjs_string_substr(rt, str, 0, pos, sub);
                    rjs_uchar_buffer_append_string(rt, &ucb, sub);
                    c ++;
                    break;
                case '&':
                    rjs_uchar_buffer_append_string(rt, &ucb, mstr);
                    c ++;
                    break;
                case '\'':
                    rjs_string_substr(rt, str, pos + mlen, slen, sub);
                    rjs_uchar_buffer_append_string(rt, &ucb, sub);
                    c ++;
                    break;
                case '<':
                    c ++;
                    nb = c;
                    ne = c;
                    while (ne < e) {
                        if (*ne == '>')
                            break;
                        ne ++;
                    }

                    if ((ne == e) || rjs_value_is_undefined(rt, groups)) {
                        rjs_uchar_buffer_append_uchar(rt, &ucb, '$');
                        rjs_uchar_buffer_append_uchar(rt, &ucb, '<');
                    } else {
                        RJS_PropertyName pn;

                        rjs_string_substr(rt, rep_templ, nb - b, ne - b, name);
                        c = ne + 1;

                        rjs_property_name_init(rt, &pn, name);
                        if ((r = rjs_get_v(rt, groups, &pn, sub)) == RJS_OK) {
                            if (!rjs_value_is_undefined(rt, sub)) {
                                if ((r = rjs_to_string(rt, sub, sstr)) == RJS_OK)
                                    rjs_uchar_buffer_append_string(rt, &ucb, sstr);
                            }
                        }
                        rjs_property_name_deinit(rt, &pn);

                        if (r == RJS_ERR)
                            goto end;
                    }
                    break;
                default:
                    if (rjs_uchar_is_digit(*c)) {
                        const RJS_UChar *t = c - 1;
                        int              n = *c - '0';

                        c ++;

                        if ((c < e) && rjs_uchar_is_digit(*c)) {
                            int nn;

                            nn = n * 10;
                            nn += *c - '0';

                            if ((nn >= 0) && (nn < cn)) {
                                n = nn;
                                c ++;
                            }
                        }

                        if ((n == 0) || (n >= cn)) {
                            rjs_uchar_buffer_append_uchars(rt, &ucb, t, c - t);
                        } else {
                            cap = rjs_value_buffer_item(rt, caps, n - 1);

                            if (!rjs_value_is_undefined(rt, cap))
                                rjs_uchar_buffer_append_string(rt, &ucb, cap);
                        }
                    } else {
                        rjs_uchar_buffer_append_uchar(rt, &ucb, '$');
                    }
                    break;
                }
            } else {
                rjs_uchar_buffer_append_uchar(rt, &ucb, '$');
            }
        } else {
            rjs_uchar_buffer_append_uchar(rt, &ucb, *c);
            c ++;
        }
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);

end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}
