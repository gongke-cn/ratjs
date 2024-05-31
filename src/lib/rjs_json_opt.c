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

/**JSON parser.*/
typedef struct {
    RJS_Lex         lex;    /**< The lexical analyzer.*/
    RJS_Token       token;  /**< The token.*/
    RJS_Bool        cached; /**< The token is cached.*/
    RJS_CharBuffer  cb;     /**< Character buffer.*/
    RJS_UCharBuffer ucb;    /**< Unicode character buffer.*/
    RJS_Bool        error;  /**< Error flag.*/
} RJS_JsonParser;

/**JSON property.*/
typedef struct {
    RJS_List         ln;  /**< List node data.*/
    RJS_HashEntry    he;  /**< Hash table entry.*/
    RJS_PropertyName pn;  /**< The property name.*/
} RJS_JsonProp;

/**JSON state entry.*/
typedef struct RJS_JsonStateStack_s RJS_JsonStateStack;
/**JSON state entry.*/
struct RJS_JsonStateStack_s {
    RJS_JsonStateStack *bot;   /**< The bottom entry.*/
    RJS_Value          *value; /**< The value.*/
};

/**Stringify state.*/
typedef struct {
    RJS_Value          *replacer;  /**< Replacer function.*/
    RJS_List            prop_list; /**< Property list.*/
    RJS_Hash            prop_hash; /**< Property hash table.*/
    ssize_t             prop_num;  /**< Properties number.*/
    RJS_Value          *indent;    /**< Indent string.*/
    RJS_Value          *gap;       /**< Gap string.*/
    RJS_JsonStateStack *stack;     /**< The value stack.*/
} RJS_JsonState;

/*Parse the value.*/
static RJS_Result
parse_value (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Value *v);
/*Serialize the JSON property.*/
static RJS_Result
serialize_json_property (RJS_Runtime *rt, RJS_JsonState *js, RJS_PropertyName *pn,
        RJS_Value *holder, RJS_Value *rv);

/*Get a token from the parser.*/
static RJS_Token*
get_json_token (RJS_Runtime *rt, RJS_JsonParser *jp)
{
    RJS_Result r;

    if (jp->cached) {
        jp->cached = RJS_FALSE;
        return &jp->token;
    }

    if ((r = rjs_lex_get_json_token(rt, &jp->lex, &jp->token)) == RJS_ERR)
        return NULL;

    return &jp->token;
}

/*Push back a token to the input.*/
static void
unget_json_token (RJS_Runtime *rt, RJS_JsonParser *jp)
{
    assert(!jp->cached);

    jp->cached = RJS_TRUE;
}

/*Initialize the JSON parser.*/
static void
json_parser_init (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Input *input)
{
    jp->error  = RJS_FALSE;
    jp->cached = RJS_FALSE;

    rjs_token_init(rt, &jp->token);
    rjs_char_buffer_init(rt, &jp->cb);
    rjs_uchar_buffer_init(rt, &jp->ucb);
    rjs_lex_init(rt, &jp->lex, input);

    jp->lex.status |= RJS_LEX_ST_JSON;
}

/*Release the JSON parser.*/
static void
json_parser_deinit (RJS_Runtime *rt, RJS_JsonParser *jp)
{
    rjs_char_buffer_deinit(rt, &jp->cb);
    rjs_uchar_buffer_deinit(rt, &jp->ucb);
    rjs_lex_deinit(rt, &jp->lex);
}

/*Output error message.*/
static void
json_parse_error (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Location *loc, const char *fmt, ...)
{
    RJS_Location curr_loc;
    va_list      ap;

    jp->error = RJS_TRUE;

    if (!loc) {
        loc = &curr_loc;
        rjs_input_get_location(jp->lex.input, loc);
    }

    va_start(ap, fmt);
    rjs_message(rt, jp->lex.input, RJS_MESSAGE_ERROR, loc, fmt, ap);
    va_end(ap);
}

/*Parse an array.*/
static RJS_Result
parse_array (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Value *v)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *item = rjs_value_stack_push(rt);
    size_t     idx  = 0;
    RJS_Token *tok;
    RJS_Result r;

    rjs_array_new(rt, v, 0, NULL);

    while (1) {
        if (!(tok = get_json_token(rt, jp))) {
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_rbracket)
            break;


        unget_json_token(rt, jp);

        if ((r = parse_value(rt, jp, item)) == RJS_ERR)
            goto end;

        rjs_set_index(rt, v, idx, item, RJS_TRUE);

        idx ++;

        if (!(tok = get_json_token(rt, jp))) {
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_rbracket)
            break;

        if (tok->type != RJS_TOKEN_comma) {
            json_parse_error(rt, jp, &tok->location, _("expect `,\' here"));
            r = RJS_ERR;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse an object.*/
static RJS_Result
parse_object (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Value *v)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_Value       *kv  = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Token       *tok;
    RJS_Result       r;

    rjs_object_new(rt, v, NULL);

    while (1) {
        if (!(tok = get_json_token(rt, jp))) {
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_rbrace)
            break;

        if ((tok->type == RJS_TOKEN_STRING)
                || (tok->type == RJS_TOKEN_IDENTIFIER)) {
            rjs_value_copy(rt, key, tok->value);
        } else {
            json_parse_error(rt, jp, &tok->location, _("expect a string here"));
            r = RJS_ERR;
            goto end;
        }

        if (!(tok = get_json_token(rt, jp))) {
            r = RJS_ERR;
            goto end;
        }

        if (tok->type != RJS_TOKEN_colon) {
            json_parse_error(rt, jp, &tok->location, _("expect `:\' here"));
            r = RJS_ERR;
            goto end;
        }

        if ((r = parse_value(rt, jp, kv)) == RJS_ERR)
            goto end;

        rjs_property_name_init(rt, &pn, key);
        rjs_create_data_property_or_throw(rt, v, &pn, kv);
        rjs_property_name_deinit(rt, &pn);

        if (!(tok = get_json_token(rt, jp))) {
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_rbrace)
            break;

        if (tok->type != RJS_TOKEN_comma) {
            json_parse_error(rt, jp, &tok->location, _("expect `,\' or `}\' here"));
            r = RJS_ERR;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the number.*/
static RJS_Result
parse_json_number (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Value *v)
{
    RJS_Result r;
    RJS_Token *tok;

    if (!(tok = get_json_token(rt, jp)))
        return RJS_ERR;

    switch (tok->type) {
    case RJS_TOKEN_NUMBER:
        rjs_value_copy(rt, v, tok->value);
        r = RJS_OK;
        break;
    case RJS_TOKEN_IDENTIFIER:
        if (rjs_string_equal(rt, tok->value, rjs_s_Infinity(rt))) {
            rjs_value_set_number(rt, v, INFINITY);
            r = RJS_OK;
        } else if (rjs_string_equal(rt, tok->value, rjs_s_NaN(rt))) {
            rjs_value_set_number(rt, v, NAN);
            r = RJS_OK;
        } else {
            json_parse_error(rt, jp, &tok->location, _("illegal token"));
            r = RJS_ERR;
        }
        break;
    default:
        json_parse_error(rt, jp, &tok->location, _("illegal token"));
        r = RJS_ERR;
        break;
    }

    return r;
}

/*Parse the value.*/
static RJS_Result
parse_value (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Value *v)
{
    RJS_Result r;
    RJS_Token *tok;

    if (!(tok = get_json_token(rt, jp)))
        return RJS_ERR;

    switch (tok->type) {
    case RJS_TOKEN_lbracket:
        r = parse_array(rt, jp, v);
        break;
    case RJS_TOKEN_lbrace:
        r = parse_object(rt, jp, v);
        break;
    case RJS_TOKEN_STRING:
    case RJS_TOKEN_NUMBER:
        rjs_value_copy(rt, v, tok->value);
        r = RJS_OK;
        break;
    case RJS_TOKEN_plus:
        r = parse_json_number(rt, jp, v);
        break;
    case RJS_TOKEN_minus:
        if ((r = parse_json_number(rt, jp, v)) == RJS_OK) {
            RJS_Number n;

            n = rjs_value_get_number(rt, v);
            rjs_value_set_number(rt, v, -n);
        }
        break;
    case RJS_TOKEN_IDENTIFIER:
        if (rjs_string_equal(rt, tok->value, rjs_s_null(rt))) {
            rjs_value_set_null(rt, v);
            r = RJS_OK;
        } else if (rjs_string_equal(rt, tok->value, rjs_s_true(rt))) {
            rjs_value_set_boolean(rt, v, RJS_TRUE);
            r = RJS_OK;
        } else if (rjs_string_equal(rt, tok->value, rjs_s_false(rt))) {
            rjs_value_set_boolean(rt, v, RJS_FALSE);
            r = RJS_OK;
        } else if (rjs_string_equal(rt, tok->value, rjs_s_Infinity(rt))) {
            rjs_value_set_number(rt, v, INFINITY);
            r = RJS_OK;
        } else if (rjs_string_equal(rt, tok->value, rjs_s_NaN(rt))) {
            rjs_value_set_number(rt, v, NAN);
            r = RJS_OK;
        } else {
            json_parse_error(rt, jp, &tok->location, _("illegal token"));
            r = RJS_ERR;
        }
        break;
    default:
        json_parse_error(rt, jp, &tok->location, _("illegal token"));
        r = RJS_ERR;
        break;
    }

    return r;
}

/*Parse the JSON.*/
static RJS_Result
parse_json (RJS_Runtime *rt, RJS_JsonParser *jp, RJS_Value *v)
{
    RJS_Result r;
    RJS_Token *tok;

    if ((r = parse_value(rt, jp, v)) == RJS_ERR)
        return r;

    if (!(tok = get_json_token(rt, jp)))
        return RJS_ERR;

    if (tok->type != RJS_TOKEN_END) {
        json_parse_error(rt, jp, &tok->location, _("expect EOF here"));
        return RJS_ERR;
    }

    return RJS_OK;
}

/*Create a JSON from the input.*/
static RJS_Result
json_from_input (RJS_Runtime *rt, RJS_Input *input, RJS_Value *res)
{
    RJS_JsonParser jp;
    RJS_Result     r;
    size_t         top = rjs_value_stack_save(rt);

    input->flags |= RJS_INPUT_FL_CRLF_TO_LF;

    json_parser_init(rt, &jp, input);

    r = parse_json(rt, &jp, res);

    json_parser_deinit(rt, &jp);

    if (r == RJS_ERR)
        rjs_throw_syntax_error(rt, _("JSON parse error"));

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a JSON from an input.
 * \param rt The current runtime.
 * \param[out] res Store the output value.
 * \param input The text input.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_json_from_input (RJS_Runtime *rt, RJS_Value *res, RJS_Input *input)
{
    return json_from_input(rt, input, res);
}

/**
 * Create a JSON from a file.
 * \param rt The current runtime.
 * \param[out] res Store the output value.
 * \param filename The filename.
 * \param enc The file's character encoding.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_json_from_file (RJS_Runtime *rt, RJS_Value *res, const char *filename, const char *enc)
{
    RJS_Input  input;
    RJS_Result r;

    if ((r = rjs_file_input_init(rt, &input, filename, enc)) == RJS_ERR)
        return r;

    r = json_from_input(rt, &input, res);

    rjs_input_deinit(rt, &input);
    return r;
}

/**
 * Parse the JSON from a string.
 * \param rt The current runtime.
 * \param[out] res Store the output value.
 * \param str The string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_json_from_string (RJS_Runtime *rt, RJS_Value *res, RJS_Value *str)
{
    RJS_Input  input;
    RJS_Result r;

    assert(rjs_value_is_string(rt, str));

    if ((r = rjs_string_input_init(rt, &input, str)) == RJS_ERR)
        return r;

    r = json_from_input(rt, &input, res);

    rjs_input_deinit(rt, &input);
    return r;
}

/*Initialize the JSON stringify state.*/
static void
state_init (RJS_Runtime *rt, RJS_JsonState *js)
{
    js->replacer = NULL;
    js->gap      = NULL;
    js->indent   = NULL;
    js->stack    = NULL;
    js->prop_num = -1;

    rjs_list_init(&js->prop_list);
    rjs_hash_init(&js->prop_hash);
}

/*Release the JSON stringify state.*/
static void
state_deinit (RJS_Runtime *rt, RJS_JsonState *js)
{
    RJS_JsonProp *p, *np;

    rjs_list_foreach_safe_c(&js->prop_list, p, np, RJS_JsonProp, ln) {
        rjs_property_name_deinit(rt, &p->pn);
        RJS_DEL(rt, p);
    }

    rjs_hash_deinit(&js->prop_hash, &rjs_hash_value_ops, rt);
}

/*Generate the JSON string.*/
static void
quote_json_string (RJS_Runtime *rt, RJS_JsonState *js, RJS_Value *str, RJS_Value *rv)
{
    const RJS_UChar *c   = rjs_string_get_uchars(rt, str);
    size_t           len = rjs_string_get_length(rt, str);
    RJS_UCharBuffer  ucb;

    rjs_uchar_buffer_init(rt, &ucb);

    rjs_uchar_buffer_append_uc(rt, &ucb, '\"');

    while (len > 0) {
        int uc;

        if (rjs_uchar_is_leading_surrogate(c[0])
                && (len > 1)
                && rjs_uchar_is_trailing_surrogate(c[1])) {
            uc = rjs_surrogate_pair_to_uc(c[0], c[1]);

            c   += 2;
            len -= 2;
        } else {
            uc = c[0];

            c   ++;
            len --;
        }

        switch (uc) {
        case '\b':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\b", 2);
            break;
        case '\t':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\t", 2);
            break;
        case '\n':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\n", 2);
            break;
        case '\f':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\f", 2);
            break;
        case '\r':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\r", 2);
            break;
        case '\"':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\\"", 2);
            break;
        case '\\':
            rjs_uchar_buffer_append_chars(rt, &ucb, "\\\\", 2);
            break;
        default:
            if ((uc < 0x20) || ((uc >= 0xd800) && (uc <= 0xdfff))) {
                char buf[7];

                buf[0] = '\\';
                buf[1] = 'u';
                buf[2] = rjs_number_to_hex_char_l(uc >> 12);
                buf[3] = rjs_number_to_hex_char_l((uc >> 8) & 0xf);
                buf[4] = rjs_number_to_hex_char_l((uc >> 4) & 0xf);
                buf[5] = rjs_number_to_hex_char_l(uc & 0xf);
                buf[6] = 0;

                rjs_uchar_buffer_append_chars(rt, &ucb, buf, -1);
            } else {
                rjs_uchar_buffer_append_uc(rt, &ucb, uc);
            }
        }
    }

    rjs_uchar_buffer_append_uc(rt, &ucb, '\"');

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    rjs_uchar_buffer_deinit(rt, &ucb);
}

/*Check if the value is already in the stack.*/
static RJS_Result
stack_cyclical_check (RJS_Runtime *rt, RJS_JsonState *js, RJS_Value *v)
{
    RJS_JsonStateStack *s;

    for (s = js->stack; s; s = s->bot) {
        if (rjs_same_value(rt, s->value, v))
            return rjs_throw_type_error(rt, _("cyclical reference"));
    }

    return RJS_OK;
}

/*Serialize the array.*/
static RJS_Result
serialize_json_array (RJS_Runtime *rt, RJS_JsonState *js, RJS_Value *value, RJS_Value *rv)
{
    size_t             top      = rjs_value_stack_save(rt);
    RJS_Value         *idxv     = rjs_value_stack_push(rt);
    RJS_Value         *key      = rjs_value_stack_push(rt);
    RJS_Value         *stepback = rjs_value_stack_push(rt);
    RJS_Value         *indent   = rjs_value_stack_push(rt);
    RJS_Value         *str      = rjs_value_stack_push(rt);
    int64_t            len      = 0, index;
    RJS_UCharBuffer    ucb;
    RJS_JsonStateStack jss;
    RJS_Result         r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = stack_cyclical_check(rt, js, value)) == RJS_ERR)
        goto end;

    jss.value = value;
    jss.bot   = js->stack;
    js->stack = &jss;

    if ((r = rjs_length_of_array_like(rt, value, &len)) == RJS_ERR)
        goto end;

    rjs_uchar_buffer_append_uchar(rt, &ucb, '[');

    if (len && js->gap) {
        rjs_value_copy(rt, stepback, js->indent);
        rjs_string_concat(rt, js->indent, js->gap, indent);
        rjs_value_copy(rt, js->indent, indent);
    }

    for (index = 0; index < len; index ++) {
        RJS_PropertyName pn;

        rjs_value_set_number(rt, idxv, index);
        rjs_to_string(rt, idxv, key);

        rjs_property_name_init(rt, &pn, key);
        r = serialize_json_property(rt, js, &pn, value, str);
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;

        if (index)
            rjs_uchar_buffer_append_uchar(rt, &ucb, ',');

        if (js->gap) {
            rjs_uchar_buffer_append_uchar(rt, &ucb, '\n');
            rjs_uchar_buffer_append_string(rt, &ucb, js->indent);
        }

        if (rjs_value_is_undefined(rt, str)) {
            rjs_uchar_buffer_append_chars(rt, &ucb, "null", -1);
        } else {
            rjs_uchar_buffer_append_string(rt, &ucb, str);
        }
    }

    if (len && js->gap) {
        rjs_value_copy(rt, js->indent, stepback);
        rjs_uchar_buffer_append_uchar(rt, &ucb, '\n');
        rjs_uchar_buffer_append_string(rt, &ucb, js->indent);
    }

    rjs_uchar_buffer_append_uchar(rt, &ucb, ']');

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    r = RJS_OK;
end:
    if (js->stack == &jss)
        js->stack = jss.bot;

    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Serialize a property key and a value.*/
static void
serialize_key_value (RJS_Runtime *rt, RJS_JsonState *js, RJS_UCharBuffer *ucb, RJS_Value *k, RJS_Value *v)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *qstr = rjs_value_stack_push(rt);

    quote_json_string(rt, js, k, qstr);

    rjs_uchar_buffer_append_string(rt, ucb, qstr);
    rjs_uchar_buffer_append_uchar(rt, ucb, ':');

    if (js->gap)
        rjs_uchar_buffer_append_uchar(rt, ucb, ' ');

    rjs_uchar_buffer_append_string(rt, ucb, v);

    rjs_value_stack_restore(rt, top);
}

/*Serialize the object.*/
static RJS_Result
serialize_json_object (RJS_Runtime *rt, RJS_JsonState *js, RJS_Value *value, RJS_Value *rv)
{
    size_t             top      = rjs_value_stack_save(rt);
    RJS_Value         *stepback = rjs_value_stack_push(rt);
    RJS_Value         *indent   = rjs_value_stack_push(rt);
    RJS_Value         *keys     = rjs_value_stack_push(rt);
    RJS_Value         *kv       = rjs_value_stack_push(rt);
    RJS_Value         *str      = rjs_value_stack_push(rt);
    RJS_Bool           empty    = RJS_TRUE;
    RJS_UCharBuffer    ucb;
    RJS_PropertyDesc   pd;
    RJS_JsonStateStack jss;
    RJS_Result         r;

    rjs_property_desc_init(rt, &pd);
    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = stack_cyclical_check(rt, js, value)) == RJS_ERR)
        goto end;

    jss.value = value;
    jss.bot   = js->stack;
    js->stack = &jss;

    rjs_uchar_buffer_append_uchar(rt, &ucb, '{');

    if (js->gap) {
        rjs_value_copy(rt, stepback, js->indent);
        rjs_string_concat(rt, js->indent, js->gap, indent);
        rjs_value_copy(rt, js->indent, indent);
    }

    if (js->prop_num == -1) {
        RJS_PropertyKeyList *pkl;
        size_t               i;

        if ((r = rjs_object_own_property_keys(rt, value, keys)) == RJS_ERR)
            goto end;

        pkl = rjs_value_get_gc_thing(rt, keys);

        /*Remove properties is not enumerable.*/
        for (i = 0; i < pkl->keys.item_num; i ++) {
            RJS_PropertyName pn;
            RJS_Value       *key = &pkl->keys.items[i];

            if (!rjs_value_is_string(rt, key))
                continue;

            rjs_property_name_init(rt, &pn, key);
            r = rjs_object_get_own_property(rt, value, &pn, &pd);
            if (r == RJS_OK) {
                if (r && !(pd.flags & RJS_PROP_FL_ENUMERABLE))
                    rjs_value_set_undefined(rt, key);
            }
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
        }

        /*Add properties.*/
        for (i = 0; i < pkl->keys.item_num; i ++) {
            RJS_PropertyName pn;
            RJS_Value       *key = &pkl->keys.items[i];

            if (!rjs_value_is_string(rt, key))
                continue;

            rjs_property_name_init(rt, &pn, key);

            r = rjs_get(rt, value, &pn, kv);
            if (r == RJS_OK) {
                r = serialize_json_property(rt, js, &pn, value, str);
                if (r == RJS_OK) {
                    if (rjs_value_is_string(rt, str)) {
                        if (!empty)
                            rjs_uchar_buffer_append_uchar(rt, &ucb, ',');

                        if (js->gap) {
                            rjs_uchar_buffer_append_uchar(rt, &ucb, '\n');
                            rjs_uchar_buffer_append_string(rt, &ucb, js->indent);
                        }

                        serialize_key_value(rt, js, &ucb, key, str);

                        empty = RJS_FALSE;
                    }
                }
            }

            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
        }
    } else {
        RJS_JsonProp *jp;

        rjs_list_foreach_c(&js->prop_list, jp, RJS_JsonProp, ln) {
            if ((r = serialize_json_property(rt, js, &jp->pn, value, str)) == RJS_ERR)
                goto end;

            if (rjs_value_is_string(rt, str)) {
                if (!empty)
                    rjs_uchar_buffer_append_uchar(rt, &ucb, ',');

                if (js->gap) {
                    rjs_uchar_buffer_append_uchar(rt, &ucb, '\n');
                    rjs_uchar_buffer_append_string(rt, &ucb, js->indent);
                }

                serialize_key_value(rt, js, &ucb, jp->pn.name, str);

                empty = RJS_FALSE;
            }
        }
    }

    if (js->gap) {
        rjs_value_copy(rt, js->indent, stepback);

        if (!empty) {
            rjs_uchar_buffer_append_uchar(rt, &ucb, '\n');
            rjs_uchar_buffer_append_string(rt, &ucb, js->indent);
        }
    }

    rjs_uchar_buffer_append_uchar(rt, &ucb, '}');

    rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
    r = RJS_OK;
end:
    if (js->stack == &jss)
        js->stack = jss.bot;

    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Serialize the JSON property.*/
static RJS_Result
serialize_json_property (RJS_Runtime *rt, RJS_JsonState *js, RJS_PropertyName *pn,
        RJS_Value *holder, RJS_Value *rv)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *key   = rjs_value_stack_push(rt);
    RJS_Value *value = rjs_value_stack_push(rt);
    RJS_Value *func  = rjs_value_stack_push(rt);
    RJS_Value *tmp   = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get(rt, holder, pn, value)) == RJS_ERR)
        goto end;

    if (rjs_value_is_object(rt, value)
#if ENABLE_BIG_INT
            || rjs_value_is_big_int(rt, value)
#endif /*ENABLE_BIG_INT*/
            ) {
        if ((r = rjs_get_v(rt, value, rjs_pn_toJSON(rt), func)) == RJS_ERR)
            goto end;

        if (rjs_is_callable(rt, func)) {
            if ((r = rjs_call(rt, func, value, pn->name, 1, tmp)) == RJS_ERR)
                goto end;

            rjs_value_copy(rt, value, tmp);
        }
    }

    if (js->replacer) {
        rjs_value_copy(rt, key, pn->name);

        if ((r = rjs_call(rt, js->replacer, holder, key, 2, tmp)) == RJS_ERR)
            goto end;

        rjs_value_copy(rt, value, tmp);
    }

    if (rjs_value_get_gc_thing_type(rt, value) == RJS_GC_THING_PRIMITIVE) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, value);

        if (rjs_value_is_number(rt, &po->value)) {
            RJS_Number n;

            if ((r = rjs_to_number(rt, value, &n)) == RJS_ERR)
                goto end;

            rjs_value_set_number(rt, value, n);
        } else if (rjs_value_is_string(rt, &po->value)) {
            if ((r = rjs_to_string(rt, value, tmp)) == RJS_ERR)
                goto end;

            rjs_value_copy(rt, value, tmp);
        } else if (rjs_value_is_boolean(rt, &po->value)) {
            rjs_value_copy(rt, value, &po->value);
        }
#if ENABLE_BIG_INT
        else if (rjs_value_is_big_int(rt, &po->value)) {
            rjs_value_copy(rt, value, &po->value);
        }
#endif /*ENABLE_BIG_INT*/
    }

    if (rjs_value_is_null(rt, value)) {
        rjs_value_copy(rt, rv, rjs_s_null(rt));
    } else if (rjs_value_is_boolean(rt, value)) {
        RJS_Bool b = rjs_value_get_boolean(rt, value);

        if (b)
            rjs_value_copy(rt, rv, rjs_s_true(rt));
        else
            rjs_value_copy(rt, rv, rjs_s_false(rt));
    } else if (rjs_value_is_string(rt, value)) {
        quote_json_string(rt, js, value, rv);
    } else if (rjs_value_is_number(rt, value)) {
        RJS_Number n;

        n = rjs_value_get_number(rt, value);

        if (isinf(n) || isnan(n)) {
            rjs_value_copy(rt, rv, rjs_s_null(rt));
        } else {
            if ((r = rjs_to_string(rt, value, rv)) == RJS_ERR)
                goto end;
        }
    }
#if ENABLE_BIG_INT
    else if (rjs_value_is_big_int(rt, value)) {
        r = rjs_throw_type_error(rt, _("cannot convert big integer to JSON"));
        goto end;
    }
#endif /*ENABLE_BIG_INT*/
    else if (rjs_value_is_object(rt, value) && !rjs_is_callable(rt, value)) {
        if (rjs_is_array(rt, value)) {
            r = serialize_json_array(rt, js, value, rv);
        } else {
            r = serialize_json_object(rt, js, value, rv);
        }
        if (r == RJS_ERR)
            goto end;
    } else {
        rjs_value_set_undefined(rt, rv);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Convert the value to JSON string.
 * \param rt The current runtime.
 * \param v The input value.
 * \param replacer The replacer function.
 * \param space The space value.
 * \param[out] str The result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_json_stringify (RJS_Runtime *rt, RJS_Value *v, RJS_Value *replacer,
        RJS_Value *space, RJS_Value *str)
{
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *prop    = rjs_value_stack_push(rt);
    RJS_Value       *item    = rjs_value_stack_push(rt);
    RJS_Value       *sp      = rjs_value_stack_push(rt);
    RJS_Value       *wrapper = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Number       n;
    RJS_JsonState    js;
    RJS_Result       r;

    state_init(rt, &js);

    if (rjs_value_is_object(rt, replacer)) {
        if (rjs_is_callable(rt, replacer)) {
            js.replacer = replacer;
        } else if (rjs_is_array(rt, replacer)) {
            int64_t len, k;

            if ((r = rjs_length_of_array_like(rt, replacer, &len)) == RJS_ERR)
                goto end;

            for (k = 0; k < len; k ++) {
                if ((r = rjs_get_index(rt, replacer, k, prop)) == RJS_ERR)
                    goto end;

                rjs_value_set_undefined(rt, item);

                if (rjs_value_is_string(rt, prop)) {
                    rjs_value_copy(rt, item, prop);
                } else if (rjs_value_is_number(rt, prop)) {
                    rjs_to_string(rt, prop, item);
                } else if (rjs_value_get_gc_thing_type(rt, prop) == RJS_GC_THING_PRIMITIVE) {
                    RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, prop);

                    if (rjs_value_is_string(rt, &po->value) || rjs_value_is_number(rt, &po->value)) {
                        if ((r = rjs_to_string(rt, prop, item)) == RJS_ERR)
                            goto end;
                    }
                }

                if (!rjs_value_is_undefined(rt, item)) {
                    RJS_HashEntry *he, **phe;

                    r = rjs_hash_lookup(&js.prop_hash, item, &he, &phe, &rjs_hash_value_ops, rt);
                    if (!r) {
                        RJS_JsonProp *jp;
                        RJS_Value    *pv;

                        RJS_NEW(rt, jp);

                        pv = rjs_value_stack_push(rt);

                        rjs_value_copy(rt, pv, item);
                        rjs_property_name_init(rt, &jp->pn, pv);
                        rjs_hash_insert(&js.prop_hash, jp->pn.name, &jp->he, phe, &rjs_hash_value_ops, rt);
                        rjs_list_append(&js.prop_list, &jp->ln);
                    }
                }
            }

            js.prop_num = js.prop_hash.entry_num;
        }
    }

    rjs_value_copy(rt, sp, space);

    if (rjs_value_get_gc_thing_type(rt, space) == RJS_GC_THING_PRIMITIVE) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, space);

        if (rjs_value_is_number(rt, &po->value)) {
            if ((r = rjs_to_number(rt, space, &n)) == RJS_ERR)
                goto end;

            rjs_value_set_number(rt, sp, n);
        } else if (rjs_value_is_string(rt, &po->value)) {
            if ((r = rjs_to_string(rt, space, sp)) == RJS_ERR)
                goto end;
        }
    }

    if (rjs_value_is_number(rt, sp)) {
        ssize_t slen, i;

        if ((r = rjs_to_integer_or_infinity(rt, sp, &n)) == RJS_ERR)
            goto end;

        slen = RJS_MIN(n, 10);
        if (slen >= 1) {
            RJS_UChar *c;

            js.gap = rjs_value_stack_push(rt);

            rjs_string_from_uchars(rt, js.gap, NULL, slen);

            c = (RJS_UChar*)rjs_string_get_uchars(rt, js.gap);

            for (i = 0; i < slen; i ++)
                c[i] = ' ';
        }
    } else if (rjs_value_is_string(rt, sp)) {
        size_t len = rjs_string_get_length(rt, sp);

        if (len) {
            js.gap = rjs_value_stack_push(rt);

            if (len > 10) {
                rjs_string_substr(rt, sp, 0, 10, js.gap);
            } else {
                rjs_value_copy(rt, js.gap, sp);
            }
        }
    }

    if (js.gap) {
        js.indent = rjs_value_stack_push(rt);
        rjs_value_copy(rt, js.indent, rjs_s_empty(rt));
    }

    rjs_ordinary_object_create(rt, NULL, wrapper);
    rjs_property_name_init(rt, &pn, rjs_s_empty(rt));
    rjs_create_data_property_or_throw(rt, wrapper, &pn, v);
    r = serialize_json_property(rt, &js, &pn, wrapper, str);
    rjs_property_name_deinit(rt, &pn);
end:
    state_deinit(rt, &js);
    rjs_value_stack_restore(rt, top);
    return r;
}
