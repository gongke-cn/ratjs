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

#if ENC_CONV == ENC_CONV_ICU
#define ENABLE_UNICODE_PROPERTY 1
#else
#define ENABLE_UNICODE_PROPERTY 0
#endif

/**Match character type.*/
typedef enum {
    RJS_REGEXP_CHAR_NORMAL, /**< Normal character.*/
    RJS_REGEXP_CHAR_s,      /**< \s */
    RJS_REGEXP_CHAR_d,      /**< \d */
    RJS_REGEXP_CHAR_w,      /**< \w */
    RJS_REGEXP_CHAR_S,      /**< \S */
    RJS_REGEXP_CHAR_D,      /**< \D */
    RJS_REGEXP_CHAR_W,      /**< \W */
#if ENABLE_UNICODE_PROPERTY
    RJS_REGEXP_CHAR_p,      /**< \p */
    RJS_REGEXP_CHAR_P       /**< \P */
#endif /*ENABLE_UNICODE_PROPERTY*/
} RJS_RegExpCharType;

/**Match character data.*/
typedef union {
    int     c;           /**< Character.*/
#if ENABLE_UNICODE_PROPERTY
    struct {
        int prop;        /**< Property type.*/
        int value;       /**< Property value.*/
    } p;                 /**< Property data.*/
#endif /*ENABLE_UNICODE_PROPERTY*/
} RJS_RegExpCharData;

/**Match character.*/
typedef struct {
    RJS_RegExpCharType type; /**< Character type.*/
    RJS_RegExpCharData c;    /**< Character data.*/
} RJS_RegExpChar;

/**Character class atom.*/
typedef struct {
    RJS_RegExpCharType type;  /**< Character type.*/
    union {
        RJS_RegExpCharData c; /**< Character data.*/
        struct {
            int min; /**< The minimum character code.*/
            int max; /**< The maximum character code.*/
        } range;     /**< The character range.*/
    } a;
} RJS_RegExpClassAtom;

/**Character class.*/
typedef struct {
    RJS_Bool reverse; /**< Reverse the match result.*/
    RJS_VECTOR_DECL(RJS_RegExpClassAtom) atoms; /**< Atoms.*/
} RJS_RegExpClass;

/**Terminal type.*/
typedef enum {
    RJS_REGEXP_TERM_CHAR,       /**< Match a character.*/
    RJS_REGEXP_TERM_ALL,        /**< Match all character.*/
    RJS_REGEXP_TERM_CLASS,      /**< Character class.*/
    RJS_REGEXP_TERM_PATTERN,    /**< Sub pattern.*/
    RJS_REGEXP_TERM_GROUP,      /**< Group.*/
    RJS_REGEXP_TERM_LINE_START, /**< Line start.*/
    RJS_REGEXP_TERM_LINE_END,   /**< Line end.*/
    RJS_REGEXP_TERM_b,          /**< \b */
    RJS_REGEXP_TERM_B,          /**< \B */
    RJS_REGEXP_TERM_BR_ID,      /**< Back reference index.*/
    RJS_REGEXP_TERM_BR_NAME,    /**< Back reference name.*/
    RJS_REGEXP_TERM_LA,         /**< Look ahead.*/
    RJS_REGEXP_TERM_LA_NOT,     /**< Look ahead node.*/
    RJS_REGEXP_TERM_LB,         /**< Look behind.*/
    RJS_REGEXP_TERM_LB_NOT      /**< Look behind not.*/
} RJS_RegExpTermType;

/**Pattern.*/
struct RJS_RegExpPattern_s {
    RJS_List alter_list; /**< Alternative list.*/
};

/**Group.*/
typedef struct {
    RJS_RegExpPattern pattern;    /**< Pattern.*/
    int               index;      /**< Index.*/
    int               name_index; /**< The name's index.*/
    RJS_List          ln;         /**< List node data.*/
} RJS_RegExpGroup;

/**Back reference.*/
typedef struct {
    int      index; /**< Back reference index.*/
    RJS_List ln;    /**< List node data.*/
} RJS_RegExpBackRef;

#if ENABLE_UNICODE_PROPERTY
typedef struct {
    int prop;   /**< Property's type.*/
    int value;  /**< Property's value.*/
} RJS_RegExpProp;
#endif /*ENABLE_UNICODE_PROPERTY*/

/**Terminal.*/
typedef struct RJS_RegExpTerm_s RJS_RegExpTerm;

/**Alternative.*/
typedef struct {
    RJS_List        ln;        /**< List node data.*/
    RJS_List        term_list; /**< The terminals list.*/
    RJS_RegExpTerm *parent;    /**< The parent terminal contains this alternative.*/
} RJS_RegExpAlter;

/**Terminal.*/
struct RJS_RegExpTerm_s {
    RJS_List           ln;          /**< List node data.*/
    RJS_RegExpTermType type;        /**< Terminal type.*/
    int64_t            min;         /**< Minimum match count.*/
    int64_t            max;         /**< Maximum match count.*/
    int                group_start; /**< Group number when terminal start.*/
    int                group_end;   /**< Group number when terminal end.*/
    RJS_Bool           greedy;      /**< Greedy or not.*/
    RJS_RegExpAlter   *alter;       /**< The alternative contains this terminal.*/
    union {
        RJS_RegExpChar     c;        /**< Character.*/
        RJS_RegExpClass    clazz;    /**< Character class.*/
        RJS_RegExpPattern  pattern;  /**< Sub pattern.*/
        RJS_RegExpGroup    group;    /**< Group.*/
        RJS_RegExpBackRef  br;       /**< Back reference.*/
#if ENABLE_UNICODE_PROPERTY
        RJS_RegExpProp     p;        /**< Unicode property.*/
#endif /*ENABLE_UNICODE_PROPERTY*/
        int                br_name_id; /**< Back reference name index.*/
    } t;
};

/**Name entry.*/
typedef struct {
    RJS_List       ln;          /**< List node data.*/
    RJS_HashEntry  he;          /**< Hash table entry data.*/
    RJS_Value     *name;        /**< The name value.*/
    int            index;       /**< The name's index.*/
    int            group_index; /**< The group's index.*/
} RJS_RegExpNameEntry;

/**Parser.*/
typedef struct {
    int                flags;      /**< The flags.*/
    size_t             stack_top;  /**< Value stack top.*/
    RJS_Input          si;         /**< The string input.*/
    RJS_RegExpPattern *pattern;    /**< The pattern.*/
    int                sp;         /**< The match state stack pointer.*/
    int                group_num;  /**< Group number.*/
    RJS_Hash           name_hash;  /**< Group names hash table.*/
    RJS_List           name_list;  /**< Group name list.*/
    RJS_List           group_list; /**< Group list.*/
    RJS_List           br_list;    /**< Back reference list.*/
#if ENABLE_UNICODE_PROPERTY
    RJS_CharBuffer     pn_cb;      /**< Property name buffer.*/
    RJS_CharBuffer     pv_cb;      /**< Property value buffer.*/
#endif /*ENABLE_UNICODE_PROPERTY*/
} RJS_RegExpParser;

/**Regular expression execute context.*/
typedef struct RJS_RegExpCtxt_s    RJS_RegExpCtxt;
/**Regular expression job.*/
typedef struct RJS_RegExpJob_s     RJS_RegExpJob;

/**Regular expression result*/
typedef enum {
    RJS_REGEXP_REJECT  = -1, /**< Reject.*/
    RJS_REGEXP_NEXT    = 0,  /**< Run the next job.*/
    RJS_REGEXP_ACCEPT  = 1,  /**< Accept.*/
    RJS_REGEXP_SUCCESS = 2   /**< Success.*/
} RJS_RegExpResult;

/**Regular expression job.*/
struct RJS_RegExpJob_s {
    RJS_RegExpPattern *pattern; /**< The pattern.*/
    RJS_RegExpAlter   *alter;   /**< The alternative.*/
    RJS_RegExpTerm    *term;    /**< The teminal.*/
    size_t             pos;     /**< The start position of the last loop.*/
    RJS_Bool           reverse; /**< The reverse mode flag.*/
    ssize_t            vsp;     /**< The vector stack's pointer.*/
    int64_t            count;   /**< The match counter.*/
    ssize_t            nextp;   /**< The next job's pointer.*/
    /**Operation function.*/
    RJS_RegExpResult (*op) (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r);
};

/**Regular expression execute context.*/
struct RJS_RegExpCtxt_s {
    RJS_RegExp      *re;      /**< The regular expression.*/
    RJS_RegExpModel *model;   /**< The model of the regular expression.*/
    RJS_Value        str;     /**< The string.*/
    int              flags;   /**< Flags.*/
    RJS_Bool         reverse; /**< Reverse or not.*/
    size_t           pos;     /**< The current position.*/
    size_t           len;     /**< Length of the string,*/
    ssize_t         *vec;     /**< Position vector.*/
    ssize_t          nextp;   /**< The next job's pointer.*/
    RJS_VECTOR_DECL(ssize_t)       vec_stack; /**< The vector stack.*/
    RJS_VECTOR_DECL(RJS_RegExpJob) job_stack; /**< The jobs stack.*/
};

#if 0
#define RJS_REGEXP_LOG(a...) RJS_LOGD(a)
#else
#define RJS_REGEXP_LOG(a...)
#endif

/*Release the pattern.*/
static void
pattern_deinit (RJS_Runtime *rt, RJS_RegExpPattern *pat);
/*Parse the pattern.*/
static RJS_Result
parse_pattern (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpPattern *pat, RJS_RegExpTerm *parent);

/*Scan the reference things in the regular expression model.*/
static void
regexp_model_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_RegExpModel *rem = ptr;

    rjs_gc_scan_value(rt, &rem->source);

    if (rem->names)
        rjs_gc_scan_value_buffer(rt, rem->names, rem->name_num);
}

/*Free the regular expression model.*/
static void
regexp_model_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_RegExpModel *rem = ptr;

    if (rem->group_names)
        RJS_DEL_N(rt, rem->group_names, rem->group_num);

    if (rem->names)
        RJS_DEL_N(rt, rem->names, rem->name_num);

    if (rem->pattern) {
        pattern_deinit(rt, rem->pattern);
        RJS_DEL(rt, rem->pattern);
    }

    RJS_DEL(rt, rem);
}

/*Regular expression model's operation functions.*/
static const RJS_GcThingOps
regexp_model_ops = {
    RJS_GC_THING_REGEXP_MODEL,
    regexp_model_op_gc_scan,
    regexp_model_op_gc_free
};

/*Scan the reference things in the regular expression.*/
static void
regexp_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_RegExp *re = ptr;

    rjs_object_op_gc_scan(rt, re);

    if (re->model)
        rjs_gc_mark(rt, re->model);
}

/*Free the regular expression.*/
static void
regexp_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_RegExp *re = ptr;

    rjs_object_deinit(rt, &re->object);

    RJS_DEL(rt, re);
}

/*Regular expression's operation functions.*/
static const RJS_ObjectOps
regexp_ops = {
    {
        RJS_GC_THING_REGEXP,
        regexp_op_gc_scan,
        regexp_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    NULL,
    NULL
};

/*Initialize the pattern.*/
static void
pattern_init (RJS_Runtime *rt, RJS_RegExpPattern *pat)
{
    rjs_list_init(&pat->alter_list);
}

/*Free the terminal.*/
static void
term_free (RJS_Runtime *rt, RJS_RegExpTerm *term)
{
    switch (term->type) {
    case RJS_REGEXP_TERM_PATTERN:
    case RJS_REGEXP_TERM_LA:
    case RJS_REGEXP_TERM_LA_NOT:
    case RJS_REGEXP_TERM_LB:
    case RJS_REGEXP_TERM_LB_NOT:
        pattern_deinit(rt, &term->t.pattern);
        break;
    case RJS_REGEXP_TERM_GROUP:
        pattern_deinit(rt, &term->t.group.pattern);
        break;
    case RJS_REGEXP_TERM_CLASS:
        rjs_vector_deinit(&term->t.clazz.atoms, rt);
        break;
    default:
        break;
    }

    RJS_DEL(rt, term);
}

/*Free the alternative.*/
static void
alter_free (RJS_Runtime *rt, RJS_RegExpAlter *a)
{
    RJS_RegExpTerm *t, *nt;

    rjs_list_foreach_safe_c(&a->term_list, t, nt, RJS_RegExpTerm, ln) {
        term_free(rt, t);
    }

    RJS_DEL(rt, a);
}

/*Release the pattern.*/
static void
pattern_deinit (RJS_Runtime *rt, RJS_RegExpPattern *pat)
{
    RJS_RegExpAlter *a, *na;

    rjs_list_foreach_safe_c(&pat->alter_list, a, na, RJS_RegExpAlter, ln) {
        alter_free(rt, a);
    }
}

/*Initialize the parser.*/
static void
parser_init (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_Value *src, int flags)
{
    p->stack_top = rjs_value_stack_save(rt);

    rjs_string_input_init(rt, &p->si, src);
    rjs_list_init(&p->name_list);
    rjs_list_init(&p->group_list);
    rjs_list_init(&p->br_list);

    p->flags     = flags;
    p->sp        = 0;
    p->group_num = 1;

    rjs_hash_init(&p->name_hash);

#if ENABLE_UNICODE_PROPERTY
    rjs_char_buffer_init(rt, &p->pn_cb);
    rjs_char_buffer_init(rt, &p->pv_cb);
#endif /*ENABLE_UNICODE_PROPERTY*/

    RJS_NEW(rt, p->pattern);
    pattern_init(rt, p->pattern);
}

/*Release the parser.*/
static void
parser_deinit (RJS_Runtime *rt, RJS_RegExpParser *p)
{
    RJS_RegExpNameEntry *n, *nn;

    /*Free the group names.*/
    rjs_list_foreach_safe_c(&p->name_list, n, nn, RJS_RegExpNameEntry, ln) {
        RJS_DEL(rt, n);
    }

    /*Free the group name hash table.*/
    rjs_hash_deinit(&p->name_hash, &rjs_hash_string_ops, rt);

    rjs_input_deinit(rt, &p->si);
    rjs_value_stack_restore(rt, p->stack_top);

#if ENABLE_UNICODE_PROPERTY
    rjs_char_buffer_deinit(rt, &p->pn_cb);
    rjs_char_buffer_deinit(rt, &p->pv_cb);
#endif /*ENABLE_UNICODE_PROPERTY*/

    /*Clear the pattern.*/
    if (p->pattern) {
        pattern_deinit(rt, p->pattern);
        RJS_DEL(rt, p->pattern);
    }
}

/*Get a unicode charater.*/
static int
get_uc (RJS_Runtime *rt, RJS_RegExpParser *p)
{
    return rjs_input_get_uc(rt, &p->si);
}

/*Push back a unicode character.*/
static void
unget_uc (RJS_Runtime *rt, RJS_RegExpParser *p, int uc)
{
    rjs_input_unget_uc(rt, &p->si, uc);
}

/*Output parse error message.*/
static void
parse_error (RJS_Runtime *rt, RJS_RegExpParser *p, const char *fmt, ...)
{
    RJS_Parser  *parser = rt->parser;
    RJS_Input   *input;
    RJS_Location loc;
    va_list      ap;

    if (parser) {
        input = parser->lex.input;
        loc   = parser->lex.regexp_loc;
    } else if (p) {
        input = &p->si;
        rjs_input_get_location(input, &loc);
    } else {
        return;
    }

    va_start(ap, fmt);
    rjs_message_v(rt, input, RJS_MESSAGE_ERROR, &loc, fmt, ap);
    va_end(ap);
}

/*Parse unicode escape character.*/
static int
parse_uc_escape (RJS_Runtime *rt, RJS_RegExpParser *p)
{
    int v = 0;
    int i;
    int c;

    for (i = 0; i < 4; i ++) {
        c = get_uc(rt, p);
        if (!rjs_uchar_is_xdigit(c)) {
            parse_error(rt, p, _("expect a hexadecimal character here"));
            return RJS_ERR;
        }

        v <<= 4;
        v |= rjs_hex_char_to_number(c);
    }

    return v;
}

/*Parse unicode escape sequence.*/
static int
parse_uc_escape_seq (RJS_Runtime *rt, RJS_RegExpParser *p, int flags)
{
    int c;

    if (flags & RJS_REGEXP_FL_U) {
        c = get_uc(rt, p);
        if (c == '{') {
            int      v          = 0;
            RJS_Bool has_xdigit = RJS_FALSE;

            while (1) {
                c = get_uc(rt, p);

                if (c == '}') {
                    if (!has_xdigit) {
                        parse_error(rt, p, _("expect a hexadecimal character here"));
                        return RJS_ERR;
                    }
                    break;
                }

                if (!rjs_uchar_is_xdigit(c)) {
                    parse_error(rt, p, _("expect a hexadecimal character here"));
                    return RJS_ERR;
                }

                v <<= 4;
                v |= rjs_hex_char_to_number(c);

                if (v > 0x10ffff) {
                    parse_error(rt, p, _("illegal unicode"));
                    return RJS_ERR;
                }

                has_xdigit = RJS_TRUE;
            }

            c = v;
        } else {
            int c1, c2;

            unget_uc(rt, p, c);

            if ((c1 = parse_uc_escape(rt, p)) == RJS_ERR)
                return RJS_ERR;

            if (!rjs_uchar_is_leading_surrogate(c1))
                return c1;

            c = get_uc(rt, p);
            if (c == '\\') {
                c = get_uc(rt, p);
                if (c == 'u') {
                    if ((c2 = parse_uc_escape(rt, p)) == RJS_ERR)
                        return RJS_ERR;

                    if (rjs_uchar_is_trailing_surrogate(c2)) {
                        c = rjs_surrogate_pair_to_uc(c1, c2);
                    } else {
                        int i;

                        for (i = 0; i < 4; i ++) {
                            unget_uc(rt, p, c2 & 0xf);
                            c2 >>= 4;
                        }
                        unget_uc(rt, p, 'u');
                        unget_uc(rt, p, '\\');

                        c = c1;
                    }
                } else {
                    unget_uc(rt, p, c);
                    unget_uc(rt, p, '\\');
                    c = c1;
                }
            } else {
                unget_uc(rt, p, c);
                c = c1;
            }
        }
    } else {
        if ((c = parse_uc_escape(rt, p)) == RJS_ERR)
            return RJS_ERR;
    }

    return c;
}

/*Parse a unicode character.*/
static int
parse_uc (RJS_Runtime *rt, RJS_RegExpParser *p, int flags)
{
    int c;

    c = get_uc(rt, p);
    if (c == '\\') {
        c = get_uc(rt, p);
        if (c != 'u') {
            parse_error(rt, p, _("expect `u' here"));
            return RJS_ERR;
        }
        if ((c = parse_uc_escape_seq(rt, p, flags)) == RJS_ERR)
            return RJS_ERR;
    }

    return c;
}

/*Get the identity escape character.*/
static int
identity_escape (RJS_Runtime *rt, RJS_RegExpParser *p, int c)
{
    if (p->flags & RJS_REGEXP_FL_U) {
        switch (c) {
            case '^':
            case '$':
            case '\\':
            case '.':
            case '*':
            case '+':
            case '?':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '|':
            case '/':
                break;
            default:
                parse_error(rt, p, _("illegal identity escape character"));
                return RJS_ERR;
        }
    }

    return c;
}

#if ENABLE_UNICODE_PROPERTY

#include "rjs_unicode_property_inc.c"

static RJS_Result
parse_prop (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpChar *mc)
{
    int         c;
    const char *pn, *pv;

    rjs_char_buffer_clear(rt, &p->pn_cb);
    rjs_char_buffer_clear(rt, &p->pv_cb);

    while (1) {
        c = get_uc(rt, p);

        if (c == RJS_INPUT_END) {
            parse_error(rt, p, _("expect `}\' at end of unicode property"));
            return RJS_ERR;
        }

        if ((c == '}') || (c == '='))
            break;

        if (!isalpha(c) && (c != '_')) {
            parse_error(rt, p, _("illegcal unicode property name character"));
            return RJS_ERR;
        }

        rjs_char_buffer_append_char(rt, &p->pn_cb, c);
    }

    if (c == '=') {
        while (1) {
            c = get_uc(rt, p);

            if (c == RJS_INPUT_END) {
                parse_error(rt, p, _("expect `}\' at end of unicode property"));
                return RJS_ERR;
            }

            if (c == '}')
                break;

            if (!isalnum(c) && (c != '_')) {
                parse_error(rt, p, _("illegcal unicode property value character"));
                return RJS_ERR;
            }

            rjs_char_buffer_append_char(rt, &p->pv_cb, c);
        }
    }

    pn = rjs_char_buffer_to_c_string(rt, &p->pn_cb);
    pv = rjs_char_buffer_to_c_string(rt, &p->pv_cb);

    if (unicode_property_lookup(pn, pv, &mc->c.p.prop, &mc->c.p.value) == RJS_ERR) {
        parse_error(rt, p, _("illegal unicode property"));
        return RJS_ERR;
    }

    return RJS_OK;
}
#endif /*!ENABLE_UNICODE_PROPERTY*/

/*Parse the escape character.*/
static RJS_Result
parse_escape (RJS_Runtime *rt, RJS_RegExpParser *p, int c, RJS_RegExpChar *mc)
{
    switch (c) {
    case 'f':
        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = '\f';
        break;
    case 'n':
        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = '\n';
        break;
    case 'r':
        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = '\r';
        break;
    case 't':
        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = '\t';
        break;
    case 'v':
        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = '\v';
        break;
    case 'd':
        mc->type = RJS_REGEXP_CHAR_d;
        break;
    case 's':
        mc->type = RJS_REGEXP_CHAR_s;
        break;
    case 'w':
        mc->type = RJS_REGEXP_CHAR_w;
        break;
    case 'D':
        mc->type = RJS_REGEXP_CHAR_D;
        break;
    case 'S':
        mc->type = RJS_REGEXP_CHAR_S;
        break;
    case 'W':
        mc->type = RJS_REGEXP_CHAR_W;
        break;
    case 'x': {
        int c1 = get_uc(rt, p);
        int c2 = get_uc(rt, p);

        if (!rjs_uchar_is_xdigit(c1) || !rjs_uchar_is_xdigit(c2)) {
            unget_uc(rt, p, c2);
            unget_uc(rt, p, c1);
            if ((c = identity_escape(rt, p, 'x')) == RJS_ERR)
                return c;
        } else {
            c = (rjs_hex_char_to_number(c1) << 4) |rjs_hex_char_to_number(c2);
        }

        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = c;
        break;
    }
    case 'u':
        if ((c = parse_uc_escape_seq(rt, p, p->flags)) == RJS_ERR)
            return RJS_ERR;

        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = c;
        break;
    case 'c':
        c = get_uc(rt, p);
        if (!rjs_uchar_is_alpha(c)) {
            unget_uc(rt, p, c);
            if ((c = identity_escape(rt, p, 'c')) == RJS_ERR)
                return c;
        } else {
            c = c & 0x1f;
        }

        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = c;
        break;
    case '0':
        c = get_uc(rt, p);
        if (!rjs_uchar_is_digit(c)) {
            unget_uc(rt, p, c);
            c = 0;
        } else {
            unget_uc(rt, p, c);
            if ((c = identity_escape(rt, p, '0')) == RJS_ERR)
                return c;
        }

        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = c;
        break;
    default:
        if ((c == 'p') || (c == 'P')) {
            int c1;

            c1 = get_uc(rt, p);
            if (c1 == '{') {
#if ENABLE_UNICODE_PROPERTY
                if (parse_prop(rt, p, mc) == RJS_ERR)
                    return RJS_ERR;

                mc->type = (c == 'p') ? RJS_REGEXP_CHAR_p : RJS_REGEXP_CHAR_P;
                break;
#else /*!ENABLE_UNICODE_PROPERTY*/
                while (1) {
                    c = get_uc(rt, p);
                    if (c == '}')
                        break;
                    if (c == RJS_INPUT_END) {
                        parse_error(rt, p, _("expect `}\' here"));
                        return RJS_ERR;
                    }
                }

                parse_error(rt, p, _("do not support unicode property expression"));
                return RJS_ERR;
#endif /*ENABLE_UNICODE_PROPERTY*/
            } else {
                unget_uc(rt, p, c1);
            }
        }
        
        if ((c = identity_escape(rt, p, c)) == RJS_ERR)
            return c;

        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = c;
        break;
    }

    return RJS_OK;
}

/*Parse the group name.*/
static RJS_RegExpNameEntry*
parse_group_name (RJS_Runtime *rt, RJS_RegExpParser *p)
{
    int                  c;
    RJS_UCharBuffer      text;
    RJS_Value           *v;
    RJS_Result           r;
    RJS_HashEntry       *he, **phe;
    RJS_String          *str;
    RJS_RegExpNameEntry *ne = NULL;

    c = get_uc(rt, p);
    if (c != '<') {
        parse_error(rt, p, _("expect `<\' here"));
        return NULL;
    }

    rjs_uchar_buffer_init(rt, &text);

    if ((c = parse_uc(rt, p, RJS_REGEXP_FL_U)) == RJS_ERR)
        goto end;

    if (!rjs_uchar_is_id_start(c)) {
        parse_error(rt, p, _("expect identifier start character here"));
        goto end;
    }

    rjs_uchar_buffer_append_uc(rt, &text, c);

    while (1) {
        c = get_uc(rt, p);
        if (c == '>')
            break;

        if (c == RJS_INPUT_END) {
            parse_error(rt, p, _("expect `>\' here"));
            goto end;
        }

        unget_uc(rt, p, c);

        if ((c = parse_uc(rt, p, RJS_REGEXP_FL_U)) == RJS_ERR)
            goto end;

        if (!rjs_uchar_is_id_continue(c)) {
            parse_error(rt, p, _("expect identifier continue character here"));
            goto end;
        }

        rjs_uchar_buffer_append_uc(rt, &text, c);
    }

    /*Lookup the name.*/
    v = rjs_value_stack_push(rt);
    rjs_string_from_uchars(rt, v, text.items, text.item_num);

    str = rjs_value_get_string(rt, v);
    r   = rjs_hash_lookup(&p->name_hash, str, &he, &phe, &rjs_hash_string_ops, rt);
    if (r) {
        ne = RJS_CONTAINER_OF(he, RJS_RegExpNameEntry, he);
    } else {
        /*Add a new name entry.*/
        RJS_NEW(rt, ne);

        ne->name        = v;
        ne->group_index = -1;
        ne->index       = p->name_hash.entry_num;

        rjs_hash_insert(&p->name_hash, str, &ne->he, phe, &rjs_hash_string_ops, rt);
        rjs_list_append(&p->name_list, &ne->ln);
    }
end:
    rjs_uchar_buffer_deinit(rt, &text);
    return ne;
}

/*Parse the character class atom.*/
static RJS_Result
parse_class_atom (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpChar *mc)
{
    RJS_Result r;
    int        c;

    c = get_uc(rt, p);
    if (c == '\\') {
        c = get_uc(rt, p);
        switch (c) {
        case 'b':
            mc->type = RJS_REGEXP_CHAR_NORMAL;
            mc->c.c  = '\b';
            break;
        default:
            if ((r = parse_escape(rt, p, c, mc)) == RJS_ERR)
                return r;
            break;
        }
    } else {
        mc->type = RJS_REGEXP_CHAR_NORMAL;
        mc->c.c  = c;
    }

    return RJS_OK;
}

/*Parse the character class.*/
static RJS_Result
parse_class (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpTerm *term)
{
    int        c;
    RJS_Result r;

    term->type            = RJS_REGEXP_TERM_CLASS;
    term->t.clazz.reverse = RJS_FALSE;
    rjs_vector_init(&term->t.clazz.atoms);

    c = get_uc(rt, p);
    if (c == '^')
        term->t.clazz.reverse = RJS_TRUE;
    else
        unget_uc(rt, p, c);

    while (1) {
        RJS_RegExpClassAtom ca;
        RJS_RegExpChar      mc;
        int                 min, max;

        c = get_uc(rt, p);
        if (c == ']')
            break;
        if (c == RJS_INPUT_END) {
            parse_error(rt, p, _("expect `]\' here"));
            return RJS_ERR;
        }

        unget_uc(rt, p, c);
        if ((r = parse_class_atom(rt, p, &mc)) == RJS_ERR)
            return r;

        c = get_uc(rt, p);
        if (c == '-') {
            if (mc.type != RJS_REGEXP_CHAR_NORMAL) {
                parse_error(rt, p, _("only normal character can be used in range"));
                return RJS_ERR;
            }

            min = mc.c.c;
            max = mc.c.c;

            c = get_uc(rt, p);
            unget_uc(rt, p, c);

            if (c == ']') {
                unget_uc(rt, p, '-');

                ca.type  = mc.type;
                ca.a.c.c = mc.c.c;
            } else {
                if ((r = parse_class_atom(rt, p, &mc)) == RJS_ERR)
                    return r;

                if (mc.type != RJS_REGEXP_CHAR_NORMAL) {
                    parse_error(rt, p, _("only normal character can be used in range"));
                    return RJS_ERR;
                }

                max = mc.c.c;

                if ((min < 0) || (max < 0)) {
                    parse_error(rt, p, _("character class cannot be used in range"));
                    return RJS_ERR;
                }

                if (min > max) {
                    parse_error(rt, p, _("minimum character code must <= maximum character code"));
                    return RJS_ERR;
                }

                ca.type        = -1;
                ca.a.range.min = min;
                ca.a.range.max = max;
            }
        } else {
            unget_uc(rt, p, c);

            ca.type  = mc.type;
            ca.a.c.c = mc.c.c;
        }

        rjs_vector_append(&term->t.clazz.atoms, ca, rt);
    }

    return RJS_OK;
}

/*Parse the sub-pattern.*/
static RJS_Result
parse_sub_pattern (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpPattern *pat, RJS_RegExpTerm *parent)
{
    RJS_Result r;
    int        c;

    if ((r = parse_pattern(rt, p, pat, parent)) == RJS_ERR)
        return r;

    c = get_uc(rt, p);
    if (c != ')') {
        parse_error(rt, p, _("expect `)\' here"));
        return RJS_ERR;
    }

    return RJS_OK;
}

/*Parse a number.*/
static RJS_Result
parse_number (RJS_Runtime *rt, RJS_RegExpParser *p, int64_t *pv)
{
    int     c;
    int64_t v = 0;

    c = get_uc(rt, p);
    if (!rjs_uchar_is_digit(c)) {
        parse_error(rt, p, _("expect a digit character"));
        return RJS_ERR;
    }

    v = c - '0';

    while (1) {
        c = get_uc(rt, p);
        if (!rjs_uchar_is_digit(c)) {
            unget_uc(rt, p, c);
            break;
        }

        v *= 10;
        v += c - '0';
    }

    *pv = v;
    return RJS_OK;
}

/*Check if 2 terminals can both participate at the same time.*/
static RJS_Bool
terms_both_participate (RJS_Runtime *rt, RJS_RegExpTerm *t1, RJS_RegExpTerm *t2)
{
    typedef struct {
        RJS_List         ln;
        RJS_RegExpTerm  *term;
        RJS_RegExpAlter *alter;
    } Node;
    RJS_RegExpAlter *alter;
    RJS_List         list;
    RJS_RegExpTerm  *tmp;
    Node            *n, *nn;
    RJS_Bool         r = RJS_FALSE;

    rjs_list_init(&list);

    tmp   = t1;
    alter = NULL;
    while (1) {
        RJS_NEW(rt, n);

        n->alter = alter;
        n->term  = tmp;

        rjs_list_append(&list, &n->ln);

        if (!tmp)
            break;

        alter = tmp->alter;
        tmp   = alter->parent;
    }

    tmp   = t2;
    alter = NULL;
    while (1) {
        rjs_list_foreach_c(&list, n, Node, ln) {
            if (n->term == tmp) {
                if (!n->alter || !alter || (n->alter == alter))
                    r = RJS_TRUE;

                tmp = NULL;
                break;
            }
        }

        if (!tmp)
            break;

        alter = tmp->alter;
        tmp   = alter->parent;
    }

    rjs_list_foreach_safe_c(&list, n, nn, Node, ln) {
        RJS_DEL(rt, n);
    }

    return r;
}

/*Parse the terminal.*/
static RJS_Result
parse_term (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpAlter *alter)
{
    RJS_RegExpTerm *term;
    RJS_Result      r;
    int             c;
    RJS_Bool        is_atom = RJS_TRUE;

    RJS_NEW(rt, term);

    term->type        = RJS_REGEXP_TERM_CHAR;
    term->min         = 1;
    term->max         = 1;
    term->greedy      = RJS_TRUE;
    term->alter       = alter;
    term->group_start = p->group_num;
    rjs_list_append(&alter->term_list, &term->ln);

    c = get_uc(rt, p);

    switch (c) {
    case '.':
        term->type = RJS_REGEXP_TERM_ALL;
        break;
    case '^':
        term->type = RJS_REGEXP_TERM_LINE_START;
        is_atom    = RJS_FALSE;
        break;
    case '$':
        term->type = RJS_REGEXP_TERM_LINE_END;
        is_atom    = RJS_FALSE;
        break;
    case '[':
        if ((r = parse_class(rt, p, term)) == RJS_ERR)
            return r;
        break;
    case '(':
        c = get_uc(rt, p);
        if (c == '?') {
            pattern_init(rt, &term->t.pattern);

            c = get_uc(rt, p);
            switch (c) {
            case ':':
                term->type = RJS_REGEXP_TERM_PATTERN;
                if ((r = parse_sub_pattern(rt, p, &term->t.pattern, term)) == RJS_ERR)
                    return r;
                break;
            case '=':
                term->type = RJS_REGEXP_TERM_LA;
                is_atom    = RJS_FALSE;
                if ((r = parse_sub_pattern(rt, p, &term->t.pattern, term)) == RJS_ERR)
                    return r;
                break;
            case '!':
                term->type = RJS_REGEXP_TERM_LA_NOT;
                is_atom    = RJS_FALSE;
                if ((r = parse_sub_pattern(rt, p, &term->t.pattern, term)) == RJS_ERR)
                    return r;
                break;
            case '<':
                c = get_uc(rt, p);
                if (c == '=') {
                    term->type = RJS_REGEXP_TERM_LB;
                    is_atom    = RJS_FALSE;
                    if ((r = parse_sub_pattern(rt, p, &term->t.pattern, term)) == RJS_ERR)
                        return r;
                } else if (c == '!') {
                    term->type = RJS_REGEXP_TERM_LB_NOT;
                    is_atom    = RJS_FALSE;
                    if ((r = parse_sub_pattern(rt, p, &term->t.pattern, term)) == RJS_ERR)
                        return r;
                } else {
                    RJS_RegExpNameEntry *ne;
                    RJS_RegExpTerm      *old;

                    unget_uc(rt, p, c);
                    unget_uc(rt, p, '<');

                    term->type          = RJS_REGEXP_TERM_GROUP;
                    term->t.group.index = p->group_num ++;
                    term->t.group.name_index = -1;

                    /*Get the group name.*/
                    ne = parse_group_name(rt, p);
                    if (!ne)
                        return RJS_ERR;

                    ne->group_index = term->t.group.index;
                    term->t.group.name_index = ne->index;

                    rjs_list_foreach_c(&p->group_list, old, RJS_RegExpTerm, t.group.ln) {
                        if (old->t.group.name_index == ne->index) {
                            if (terms_both_participate(rt, old, term)) {
                                parse_error(rt, p, _("group name \"%s\" is already used"),
                                        rjs_string_to_enc_chars(rt, ne->name, NULL, NULL));
                                return RJS_ERR;
                            }
                        }
                    }

                    rjs_list_append(&p->group_list, &term->t.group.ln);

                    if ((r = parse_sub_pattern(rt, p, &term->t.group.pattern, term)) == RJS_ERR)
                        return r;
                }
                break;
            default:
                parse_error(rt, p, _("expect `:\', `=\', `!\' or `<\' here"));
                return RJS_ERR;
            }
        } else {
            unget_uc(rt, p, c);

            term->type          = RJS_REGEXP_TERM_GROUP;
            term->t.group.index = p->group_num ++;
            term->t.group.name_index = -1;

            rjs_list_append(&p->group_list, &term->t.group.ln);

            pattern_init(rt, &term->t.group.pattern);

            if ((r = parse_sub_pattern(rt, p, &term->t.group.pattern, term)) == RJS_ERR)
                return r;
        }
        break;
    case '\\':
        c = get_uc(rt, p);
        switch (c) {
        case 'b':
            term->type = RJS_REGEXP_TERM_b;
            is_atom    = RJS_FALSE;
            break;
        case 'B':
            term->type = RJS_REGEXP_TERM_B;
            is_atom    = RJS_FALSE;
            break;
        default:
            if ((c >= '1') && (c <= '9')) {
                int v = c - '0';

                while (1) {
                    c = get_uc(rt, p);
                    if (!rjs_uchar_is_digit(c)) {
                        unget_uc(rt, p, c);
                        break;
                    }

                    v *= 10;
                    v += c - '0';
                }

                term->type       = RJS_REGEXP_TERM_BR_ID;
                term->t.br.index = v;

                rjs_list_append(&p->br_list, &term->t.br.ln);
            } else if ((p->flags & RJS_REGEXP_FL_N) && (c == 'k')) {
                RJS_RegExpNameEntry *ne;

                term->type         = RJS_REGEXP_TERM_BR_NAME;
                term->t.br_name_id = -1;

                if (!(ne = parse_group_name(rt, p)))
                    return RJS_ERR;

                term->t.br_name_id = ne->index;
            } else {
                if ((c = parse_escape(rt, p, c, &term->t.c)) == RJS_ERR)
                    return RJS_ERR;
                term->type = RJS_REGEXP_TERM_CHAR;
            }
            break;
        }
        break;
    default:
        if ((c == '?') || (c == '*') || (c == '+') || (c == '|') || (c == '{')
                || (c == ')') || (c == ']') || (c == '}')) {
            parse_error(rt, p, _("illegal character"));
            return RJS_ERR;
        }

        term->t.c.type = RJS_REGEXP_CHAR_NORMAL;
        term->t.c.c.c  = c;
        break;
    }

    if (is_atom) {
        RJS_Bool has_quan = RJS_TRUE;

        c = get_uc(rt, p);
        switch (c) {
        case '?':
            term->min = 0;
            term->max = 1;
            break;
        case '+':
            term->min = 1;
            term->max = -1;
            break;
        case '*':
            term->min = 0;
            term->max = -1;
            break;
        case '{':
            if ((r = parse_number(rt, p, &term->min)) == RJS_ERR)
                return r;
            
            c = get_uc(rt, p);
            if (c == ',') {
                c = get_uc(rt, p);
                if (c != '}') {
                    unget_uc(rt, p, c);
                    if ((r = parse_number(rt, p, &term->max)) == RJS_ERR)
                        return r;

                    if (term->min > term->max) {
                        parse_error(rt, p, _("minimum value must <= maximum value"));
                        return RJS_ERR;
                    }

                    c = get_uc(rt, p);
                    if (c != '}') {
                        parse_error(rt, p, _("expect `}\' here"));
                        return RJS_ERR;
                    }
                } else {
                    term->max = -1;
                }
            } else if (c == '}') {
                term->max = term->min;
            } else {
                parse_error(rt, p, _("expect `,\' or `}\' here"));
                return RJS_ERR;
            }
            break;
        default:
            unget_uc(rt, p, c);
            has_quan = RJS_FALSE;
            break;
        }

        if (has_quan) {
            c = get_uc(rt, p);
            if (c == '?')
                term->greedy = RJS_FALSE;
            else
                unget_uc(rt, p, c);
        }
    }

    term->group_end = p->group_num;

    return RJS_OK;
}

/*Parse the alternative.*/
static RJS_Result
parse_alter (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpPattern *pat, RJS_RegExpTerm *parent)
{
    RJS_Result       r;
    int              c;
    RJS_RegExpAlter *alter;

    /*Add a new alternative.*/
    RJS_NEW(rt, alter);

    alter->parent = parent;

    rjs_list_init(&alter->term_list);
    rjs_list_append(&pat->alter_list, &alter->ln);

    while (1) {
        c = get_uc(rt, p);
        if ((c == '|') || (c == RJS_INPUT_END) || (c == ')')) {
            unget_uc(rt, p, c);
            break;
        }

        /*Add a terminal.*/
        unget_uc(rt, p, c);
        if ((r = parse_term(rt, p, alter)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/*Parse the pattern.*/
static RJS_Result
parse_pattern (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_RegExpPattern *pat, RJS_RegExpTerm *parent)
{
    RJS_Result r;
    int        c;

    if ((r = parse_alter(rt, p, pat, parent)) == RJS_ERR)
        return r;

    while (1) {
        c = get_uc(rt, p);
        if (c != '|') {
            unget_uc(rt, p, c);
            break;
        }

        if ((r = parse_alter(rt, p, pat, parent)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/*Parse the regular expression.*/
static RJS_Result
parse_regexp (RJS_Runtime *rt, RJS_RegExpParser *p)
{
    RJS_RegExpTerm      *term;
    RJS_RegExpNameEntry *ne;
    RJS_Result           r;
    int                  c;

    if ((r = parse_pattern(rt, p, p->pattern, NULL)) == RJS_ERR)
        return r;

    c = get_uc(rt, p);
    if (c != RJS_INPUT_END) {
        parse_error(rt, p, _("expect EOF here"));
        return RJS_ERR;
    }

    /*Check the back reference.*/
    rjs_list_foreach_c(&p->br_list, term, RJS_RegExpTerm, t.br.ln) {
        if (term->t.br.index >= p->group_num) {
            parse_error(rt, p, _("back reference index must <= number of left-capturing parentheses"));
            return RJS_ERR;
        }
    }

    /*Check the group name back reference.*/
    rjs_list_foreach_c(&p->name_list, ne, RJS_RegExpNameEntry, ln) {
        if (ne->group_index == -1) {
            parse_error(rt, p, _("group name \"%s\" is not defined"),
                    rjs_string_to_enc_chars(rt, ne->name, NULL, NULL));
            return RJS_ERR;
        }
    }

    return RJS_OK;
}

/*Generate the source string.*/
static RJS_Result
gen_source (RJS_Runtime *rt, RJS_Value *src, RJS_Value *str)
{
    RJS_UCharBuffer  ucb;
    const RJS_UChar *c, *e;
    size_t           len;

    len = rjs_string_get_length(rt, src);
    if (len == 0) {
        rjs_string_from_chars(rt, str, "(?:)", -1);
        return RJS_OK;
    }

    c = rjs_string_get_uchars(rt, src);
    e = c + len;

    rjs_uchar_buffer_init(rt, &ucb);

    while (c < e) {
        switch (*c) {
        case '\n':
            rjs_uchar_buffer_append_uchar(rt, &ucb, '\\');
            rjs_uchar_buffer_append_uchar(rt, &ucb, 'n');
            break;
        case '\r':
            rjs_uchar_buffer_append_uchar(rt, &ucb, '\\');
            rjs_uchar_buffer_append_uchar(rt, &ucb, 'r');
            break;
        case '/':
            rjs_uchar_buffer_append_uchar(rt, &ucb, '\\');
            rjs_uchar_buffer_append_uchar(rt, &ucb, '/');
            break;
        default:
            rjs_uchar_buffer_append_uchar(rt, &ucb, *c);
            break;
        }
        c ++;
    }

    rjs_string_from_uchars(rt, str, ucb.items, ucb.item_num);

    rjs_uchar_buffer_deinit(rt, &ucb);
    return RJS_OK;
}

/*Generate the regular expression.*/
static RJS_Result
gen_regexp (RJS_Runtime *rt, RJS_RegExpParser *p, RJS_Value *v, RJS_Value *src)
{
    RJS_RegExp      *re;
    RJS_RegExpModel *rem;

    re = (RJS_RegExp*)rjs_value_get_object(rt, v);

    rem = re->model;

    /*Store the source string.*/
    gen_source(rt, src, &rem->source);

    rem->flags     = p->flags;
    rem->group_num = p->group_num;
    rem->name_num  = p->name_hash.entry_num;

    /*Store groups.*/
    if (rem->name_num) {
        RJS_RegExpNameEntry *ne;
        RJS_RegExpTerm      *term;

        RJS_NEW_N(rt, rem->group_names, rem->group_num);
        RJS_NEW_N(rt, rem->names, rem->name_num);
    
        rjs_value_buffer_fill_undefined(rt, rem->names, rem->name_num);

        rjs_list_foreach_c(&p->group_list, term, RJS_RegExpTerm, t.group.ln) {
            rem->group_names[term->t.group.index] = term->t.group.name_index;
        }

        rjs_list_foreach_c(&p->name_list, ne, RJS_RegExpNameEntry, ln) {
            rjs_value_copy(rt, &rem->names[ne->index], ne->name);
        }
    } else {
        rem->group_names = NULL;
        rem->names       = NULL;
    }

    rem->pattern = p->pattern;
    p->pattern   = NULL;

    return RJS_OK;
}

/**
 * Allocate a new regular expression.
 * \param rt The current runtime.
 * \param nt The new target.
 * \param[out] rv Return the regular expression object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_alloc (RJS_Runtime *rt, RJS_Value *nt, RJS_Value *rv)
{
    RJS_RegExp       *re;
    RJS_RegExpModel  *rem;
    RJS_PropertyDesc  pd;

    RJS_NEW(rt, re);

    re->model = NULL;

    /*Initialize the object.*/
    rjs_ordinary_init_from_constructor(rt, &re->object, nt, RJS_O_RegExp_prototype, &regexp_ops, rv);

    /*Allocate the model.*/
    RJS_NEW(rt, rem);

    rem->flags       = 0;
    rem->group_num   = 0;
    rem->name_num    = 0;
    rem->group_names = NULL;
    rem->names       = NULL;
    rem->pattern     = NULL;

    rjs_value_set_undefined(rt, &rem->source);

    re->model = rem;
    rjs_gc_add(rt, rem, &regexp_model_ops);

    /*add "lastIndex".*/
    rjs_property_desc_init(rt, &pd);
    pd.flags = RJS_PROP_FL_HAS_WRITABLE
            |RJS_PROP_FL_HAS_CONFIGURABLE
            |RJS_PROP_FL_HAS_ENUMERABLE
            |RJS_PROP_FL_WRITABLE;
    rjs_define_property_or_throw(rt, rv, rjs_pn_lastIndex(rt), &pd);
    rjs_property_desc_deinit(rt, &pd);

    return RJS_OK;
}

/**
 * Initialize the regular expression.
 * \param rt The current runtime.
 * \param re The regular expression object.
 * \param p The source string.
 * \param f The flags string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_initialize (RJS_Runtime *rt, RJS_Value *re, RJS_Value *p, RJS_Value *f)
{
    RJS_RegExpParser parser;
    RJS_Result       r;
    const RJS_UChar *fc;
    size_t           flen;
    size_t           top         = rjs_value_stack_save(rt);
    RJS_Value       *flags       = rjs_value_stack_push(rt);
    RJS_Value       *src         = rjs_value_stack_push(rt);
    int              flagsv      = 0;
    RJS_Bool         need_deinit = RJS_FALSE;
    RJS_Bool         perr        = RJS_FALSE;

    if (!p || rjs_value_is_undefined(rt, p)) {
        rjs_value_copy(rt, src, rjs_s_empty(rt));
    } else {
        if ((r = rjs_to_string(rt, p, src)) == RJS_ERR)
            goto end;
    }

    if (!f || rjs_value_is_undefined(rt, f)) {
        rjs_value_copy(rt, flags, rjs_s_empty(rt));
    } else {
        if ((r = rjs_to_string(rt, f, flags)) == RJS_ERR)
            goto end;
    }

    perr = RJS_TRUE;

    fc   = rjs_string_get_uchars(rt, flags);
    flen = rjs_string_get_length(rt, flags);

    while (flen) {
        int fv = 0;

        switch (*fc) {
        case 'd':
            fv = RJS_REGEXP_FL_D;
            break;
        case 'g':
            fv = RJS_REGEXP_FL_G;
            break;
        case 'i':
            fv = RJS_REGEXP_FL_I;
            break;
        case 'm':
            fv = RJS_REGEXP_FL_M;
            break;
        case 's':
            fv = RJS_REGEXP_FL_S;
            break;
        case 'u':
            fv = RJS_REGEXP_FL_U|RJS_REGEXP_FL_N;
            break;
        case 'y':
            fv = RJS_REGEXP_FL_Y;
            break;
        default:
            parse_error(rt, NULL, _("illegal regular expression flag"));
            r = RJS_ERR;
            goto end;
        }

        if (flagsv & fv) {
            parse_error(rt, NULL, _("regular expression cannot has duplicated flags"));
            r = RJS_ERR;
            goto end;
        }

        flagsv |= fv;

        fc   ++;
        flen --;
    }

    parser_init(rt, &parser, src, flagsv);
    need_deinit = RJS_TRUE;

    if ((r = parse_regexp(rt, &parser)) == RJS_ERR)
        goto end;

    if (!(flagsv & RJS_REGEXP_FL_N) && !rjs_list_is_empty(&parser.name_list)) {
        parser_deinit(rt, &parser);
        parser_init(rt, &parser, src, flagsv|RJS_REGEXP_FL_N);

        if ((r = parse_regexp(rt, &parser)) == RJS_ERR)
            goto end;
    }

    if ((r = gen_regexp(rt, &parser, re, src)) == RJS_ERR)
        goto end;

    if ((r = rjs_set_number(rt, re, rjs_pn_lastIndex(rt), 0, RJS_TRUE)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    if ((r == RJS_ERR) && perr && !rt->parser)
        rjs_throw_syntax_error(rt, _("regular expression initialize failed"));

    if (need_deinit)
        parser_deinit(rt, &parser);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a new regular expression.
 * \param rt The current runtime.
 * \param p The source string.
 * \param f The flags string.
 * \param[out] re Return the regular expression.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_create (RJS_Runtime *rt, RJS_Value *p, RJS_Value *f, RJS_Value *re)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if ((r = rjs_regexp_alloc(rt, rjs_o_RegExp(realm), re)) == RJS_ERR)
        return r;

    if ((r = rjs_regexp_initialize(rt, re, p, f)) == RJS_ERR)
        return r;

    return RJS_OK;
}

/**
 * Clone a regular expression.
 * \param rt The current runtime.
 * \param[out] dst The result regular expression.
 * \param src The source regular expression.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_clone (RJS_Runtime *rt, RJS_Value *dst, RJS_Value *src)
{
    RJS_RegExp      *sre;
    RJS_RegExp      *dre;
    RJS_PropertyDesc pd;
    RJS_Realm       *realm = rjs_realm_current(rt);

    assert(rjs_value_get_gc_thing_type(rt, src) == RJS_GC_THING_REGEXP);

    sre = rjs_value_get_gc_thing(rt, src);

    RJS_NEW(rt, dre);

    dre->model = sre->model;

    /*Initialize the object.*/
    rjs_object_init(rt, dst, &dre->object, rjs_o_RegExp_prototype(realm), &regexp_ops);

    /*add "lastIndex".*/
    rjs_property_desc_init(rt, &pd);
    pd.flags = RJS_PROP_FL_HAS_WRITABLE
            |RJS_PROP_FL_HAS_CONFIGURABLE
            |RJS_PROP_FL_HAS_ENUMERABLE
            |RJS_PROP_FL_WRITABLE
            |RJS_PROP_FL_HAS_VALUE;
    rjs_value_set_number(rt, pd.value, 0);
    rjs_define_property_or_throw(rt, dst, rjs_pn_lastIndex(rt), &pd);
    rjs_property_desc_deinit(rt, &pd);

    return RJS_OK;
}

/**
 * Create a new regular expression.
 * \param rt The current runtime.
 * \param[out] v Return the regular expression value.
 * \param src The source string.
 * \param flags The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *src, RJS_Value *flags)
{
    return rjs_regexp_create(rt, src, flags, v);
}

/*Canonicalize the character.*/
static int
canonicalize (int c, int flags)
{
    int r;

    if ((flags & RJS_REGEXP_FL_I) && (flags & RJS_REGEXP_FL_U)) {
        r = rjs_uchar_fold_case(c);
    } else if (flags & RJS_REGEXP_FL_I) {
        RJS_UChar cin, cout;

        cin = c;

        if (rjs_uchars_to_upper(&cin, 1, &cout, 1, "") > 1)
            r = c;
        else
            r = cout;
    } else {
        r = c;
    }

    return r;
}

/*Get the last index.*/
static RJS_Result
get_last_index (RJS_Runtime *rt, RJS_Value *v, int64_t *pi)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;
    int64_t    len;

    if ((r = rjs_get(rt, v, rjs_pn_lastIndex(rt), tmp)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_length(rt, tmp, &len)) == RJS_ERR)
        goto end;

    *pi = len;

end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the character.*/
static int
get_char (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, ssize_t pos)
{
    if (pos < 0)
        return -1;

    if (pos >= ctxt->len)
        return -1;

    return rjs_string_get_uchar(rt, &ctxt->str, pos);
}

/*Get the character and move position to the next position.*/
static int
read_char (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt)
{
    int c;

    if (ctxt->reverse) {
        if (ctxt->pos == 0)
            return -1;

        c = rjs_string_get_uchar(rt, &ctxt->str, ctxt->pos - 1);
        if ((ctxt->pos > 1)
                && (ctxt->flags & RJS_REGEXP_FL_U)
                && rjs_uchar_is_trailing_surrogate(c)) {
            int c2;

            c2 = rjs_string_get_uchar(rt, &ctxt->str, ctxt->pos - 2);

            if (rjs_uchar_is_leading_surrogate(c2)) {
                c = rjs_surrogate_pair_to_uc(c2, c);
                ctxt->pos -= 2;
            } else {
                ctxt->pos --;
            }
        } else {
            ctxt->pos --;
        }
    } else {
        if (ctxt->pos >= ctxt->len)
            return -1;

        c = rjs_string_get_uchar(rt, &ctxt->str, ctxt->pos);
        if ((ctxt->pos < ctxt->len - 1)
                && (ctxt->flags & RJS_REGEXP_FL_U)
                && rjs_uchar_is_leading_surrogate(c)) {
            int c2;

            c2 = rjs_string_get_uchar(rt, &ctxt->str, ctxt->pos + 1);

            if (rjs_uchar_is_trailing_surrogate(c2)) {
                c = rjs_surrogate_pair_to_uc(c, c2);
                ctxt->pos += 2;
            } else {
                ctxt->pos ++;
            }
        } else {
            ctxt->pos ++;
        }
    }

    RJS_REGEXP_LOG("read char:%c", c);
    return c;
}

/*Get the next last index.*/
static size_t
adv_str_index (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, size_t index, RJS_Bool full_uc)
{
    int c;

    if (!full_uc)
        return index + 1;

    c = get_char(rt, ctxt, index);
    if (c == -1)
        return index + 1;

    if (!rjs_uchar_is_leading_surrogate(c))
        return index + 1;

    c = get_char(rt, ctxt, index + 1);
    if (rjs_uchar_is_trailing_surrogate(c))
        return index + 2;

    return index + 1;
}

/*Check if the character is a word.*/
static RJS_Bool
is_word (int c)
{
    return rjs_uchar_is_alnum(c) || (c == '_');
}

/*Match the alternative.*/
static RJS_RegExpResult
match_alter (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat,
        RJS_RegExpAlter *alter);
/*Match pattern operation.*/
static RJS_RegExpResult
match_pattern (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat);
/*Match the terminal.*/
static RJS_RegExpResult
match_term (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat,
        RJS_RegExpAlter *alter, RJS_RegExpTerm *term, int64_t cnt, size_t pos);
/*Match the terminal list.*/
static RJS_RegExpResult
match_term_list (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat,
        RJS_RegExpAlter *alter, RJS_RegExpTerm *term);

/*Get the job pointer.*/
static inline RJS_RegExpJob*
get_job (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, size_t jp)
{
    return &ctxt->job_stack.items[jp];
}

/*Push a job to the stack.*/
static size_t
push_job (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt)
{
    size_t         p = ctxt->job_stack.item_num;
    RJS_RegExpJob *job;

    rjs_vector_resize(&ctxt->job_stack, p + 1, rt);

    job = get_job(rt, ctxt, p);

    job->vsp = -1;

    return p;
}

/*Pop up the top job in the stack.*/
static void
pop_job (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt)
{
    size_t         p   = ctxt->job_stack.item_num - 1;
    RJS_RegExpJob *job = get_job(rt, ctxt, p);

    if (job->vsp != -1)
        ctxt->vec_stack.item_num = job->vsp;

    ctxt->job_stack.item_num --;
}

/*Push the position vector.*/
static size_t
push_vec (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt)
{
    size_t p = ctxt->vec_stack.item_num;

    rjs_vector_resize(&ctxt->vec_stack, p + ctxt->model->group_num * 2, rt);

    return p;
}

/*Save the vector to the stack.*/
static void
save_vec (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, size_t vp)
{
    RJS_ELEM_CPY(ctxt->vec_stack.items + vp, ctxt->vec, ctxt->model->group_num * 2);
}

/*Restore the vector from the stack.*/
static void
restore_vec (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, size_t vp)
{
    RJS_ELEM_CPY(ctxt->vec, ctxt->vec_stack.items + vp, ctxt->model->group_num * 2);
}

/*Job OK.*/
static RJS_RegExpResult
job_ok (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    if (r == RJS_REGEXP_NEXT) {
        RJS_REGEXP_LOG("job ok");
        return RJS_REGEXP_ACCEPT;
    }

    return r;
}

/*Success job.*/
static RJS_RegExpResult
job_success (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    if (r == RJS_REGEXP_NEXT) {
        RJS_REGEXP_LOG("job success");

        ctxt->vec[1] = ctxt->pos;

        return RJS_REGEXP_SUCCESS;
    }

    return r;
}

/*Match the character.*/
static RJS_Bool
match_char (RJS_Runtime *rt, RJS_RegExpCharType t, RJS_RegExpCharData *d, int c, int flags)
{
    RJS_Bool r;

    switch (t) {
    case RJS_REGEXP_CHAR_s:
        r = rjs_uchar_is_white_space(c);
        break;
    case RJS_REGEXP_CHAR_d:
        r = rjs_uchar_is_digit(c);
        break;
    case RJS_REGEXP_CHAR_w:
        r = is_word(c);
        break;
    case RJS_REGEXP_CHAR_S:
        r = !rjs_uchar_is_white_space(c);
        break;
    case RJS_REGEXP_CHAR_D:
        r = !rjs_uchar_is_digit(c);
        break;
    case RJS_REGEXP_CHAR_W:
        r = !is_word(c);
        break;
    case RJS_REGEXP_CHAR_NORMAL:
        r = canonicalize(c, flags) == canonicalize(d->c, flags);
        break;
#if ENABLE_UNICODE_PROPERTY
    case RJS_REGEXP_CHAR_p:
        r = unicode_property_match(d->p.prop, d->p.value, canonicalize(c, flags));
        break;
    case RJS_REGEXP_CHAR_P:
        r = !unicode_property_match(d->p.prop, d->p.value, canonicalize(c, flags));
        break;
#endif /*ENABLE_UNICODE_PROPERTY*/
    default:
        assert(0);
        r = RJS_FALSE;
        break;
    }

    return r;
}

/*Match the class atom.*/
static RJS_Bool
match_class_atom (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpClassAtom *atom, int c)
{
    RJS_Bool r;

    if (atom->type != -1) {
        r = match_char(rt, atom->type, &atom->a.c, c, ctxt->flags);
    } else {
        int min = canonicalize(atom->a.range.min, ctxt->flags);
        int max = canonicalize(atom->a.range.max, ctxt->flags);
        int v   = canonicalize(c, ctxt->flags);

        r = (v >= min) && (v <= max);
    }

    return r;
}

/*Match the character class.*/
static RJS_Bool
match_class (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpClass *clazz, int c)
{
    RJS_RegExpClassAtom *atom;
    size_t               i;

    if (clazz->reverse) {
        rjs_vector_foreach(&clazz->atoms, i, atom) {
            if (match_class_atom(rt, ctxt, atom, c))
                return RJS_FALSE;
        }

        return RJS_TRUE;
    } else {
        rjs_vector_foreach(&clazz->atoms, i, atom) {
            if (match_class_atom(rt, ctxt, atom, c))
                return RJS_TRUE;
        }

        return RJS_FALSE;
    }
}

/*Group end job.*/
static RJS_RegExpResult
job_group_end (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    RJS_RegExpTerm *term = job->term;

    if (r == RJS_REGEXP_NEXT) {
        RJS_REGEXP_LOG("job group end %d", term->t.group.index);

        ctxt->vec[term->t.group.index * 2 + 1] = ctxt->pos;

        return RJS_REGEXP_NEXT;
    }

    return r;
}

/*Match the group.*/
static RJS_RegExpResult
match_group (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpTerm *term)
{
    size_t         jid;
    RJS_RegExpJob *job;

    ctxt->vec[term->t.group.index * 2] = ctxt->pos;

    jid = push_job(rt, ctxt);
    job = get_job(rt, ctxt, jid);

    job->op    = job_group_end;
    job->term  = term;
    job->nextp = ctxt->nextp;

    ctxt->nextp = jid;

    return match_pattern(rt, ctxt, &term->t.group.pattern);
}

/*LA/LB end job.*/
static RJS_RegExpResult
job_la_lb_end (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    RJS_RegExpTerm *term = job->term;

    RJS_REGEXP_LOG("job la lb end");

    ctxt->pos     = job->pos;
    ctxt->reverse = job->reverse;
    ctxt->nextp   = job->nextp;

    if ((term->type == RJS_REGEXP_TERM_LA) || (term->type == RJS_REGEXP_TERM_LB)) {
        return (r == RJS_REGEXP_REJECT) ? RJS_REGEXP_REJECT : RJS_REGEXP_NEXT;
    } else {
        if (r == RJS_REGEXP_REJECT) {
            int i;

            for (i = term->group_start; i < term->group_end; i ++) {
                ctxt->vec[i * 2]     = -1;
                ctxt->vec[i * 2 + 1] = -1;
            }
        }

        return (r == RJS_REGEXP_REJECT) ? RJS_REGEXP_NEXT : RJS_REGEXP_REJECT;
    }
}

/*Match look ahead/behind.*/
static RJS_RegExpResult
match_la_lb (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpTerm *term)
{
    size_t         jid;
    RJS_RegExpJob *job;

    jid = push_job(rt, ctxt);
    job = get_job(rt, ctxt, jid);

    job->op      = job_la_lb_end;
    job->pos     = ctxt->pos;
    job->reverse = ctxt->reverse;
    job->term    = term;
    job->nextp   = ctxt->nextp;

    jid = push_job(rt, ctxt);
    job = get_job(rt, ctxt, jid);

    job->op    = job_ok;
    job->nextp = -1;

    ctxt->nextp = jid;

    if ((term->type == RJS_REGEXP_TERM_LA) || (term->type == RJS_REGEXP_TERM_LA_NOT)) {
        ctxt->reverse = RJS_FALSE;
    } else {
        ctxt->reverse = RJS_TRUE;
    }

    return match_pattern(rt, ctxt, &term->t.pattern);
}

/*Back reference match.*/
static RJS_Bool
match_back_ref (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, int id)
{
    ssize_t  start, end, tmp;
    size_t   p1, p2, i, len;
    int      c1, c2;
    RJS_Bool r;

    if (id >= ctxt->model->group_num) {
        r = RJS_TRUE;
        goto end;
    }

    start = ctxt->vec[id * 2];
    end   = ctxt->vec[id * 2 + 1];

    if ((start == -1) || (end == -1)) {
        r = RJS_TRUE;
        goto end;
    }

    if (start > end) {
        tmp   = start;
        start = end;
        end   = tmp;
    }

    len = end - start;

    if (ctxt->reverse) {
        if (ctxt->pos < len) {
            r = RJS_FALSE;
            goto end;
        }

        p1 = ctxt->pos - len;
    } else {
        if (ctxt->pos + len > ctxt->len) {
            r = RJS_FALSE;
            goto end;
        }

        p1 = ctxt->pos;
    }

    p2 = start;

    for (i = 0; i < len; i ++) {
        c1 = rjs_string_get_uchar(rt, &ctxt->str, p1);
        c2 = rjs_string_get_uchar(rt, &ctxt->str, p2);

        if (canonicalize(c1, ctxt->flags) != canonicalize(c2, ctxt->flags)) {
            r = RJS_FALSE;
            goto end;
        }

        p1 ++;
        p2 ++;
    }

    if (ctxt->reverse)
        ctxt->pos -= len;
    else
        ctxt->pos += len;

    r = RJS_TRUE;
end:
    return r;
}

/*Match an atom.*/
static RJS_RegExpResult
match_atom (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpTerm *term)
{
    int              c, nc, i;
    RJS_RegExpResult r;

    RJS_REGEXP_LOG("match atom");

    switch (term->type) {
    case RJS_REGEXP_TERM_CHAR:
        if ((c = read_char(rt, ctxt)) == -1)
            return RJS_REGEXP_REJECT;

        if (!match_char(rt, term->t.c.type, &term->t.c.c, c, ctxt->flags))
            return RJS_REGEXP_REJECT;
        break;
    case RJS_REGEXP_TERM_ALL:
        if ((c = read_char(rt, ctxt)) == -1)
            return RJS_REGEXP_REJECT;

        if (!(ctxt->flags & RJS_REGEXP_FL_S)) {
            if (rjs_uchar_is_line_terminator(c))
                return RJS_REGEXP_REJECT;
        }
        break;
    case RJS_REGEXP_TERM_CLASS:
        if ((c = read_char(rt, ctxt)) == -1)
            return RJS_REGEXP_REJECT;

        if (!match_class(rt, ctxt, &term->t.clazz, c))
            return RJS_REGEXP_REJECT;
        break;
    case RJS_REGEXP_TERM_LINE_START:
        c = get_char(rt, ctxt, ctxt->pos - 1);
        if (ctxt->flags & RJS_REGEXP_FL_M) {
            if ((c != -1) && !rjs_uchar_is_line_terminator(c))
                return RJS_REGEXP_REJECT;
        } else {
            if (c != -1)
                return RJS_REGEXP_REJECT;
        }
        break;
    case RJS_REGEXP_TERM_LINE_END:
        c = get_char(rt, ctxt, ctxt->pos);
        if (ctxt->flags & RJS_REGEXP_FL_M) {
            if ((c != -1) && !rjs_uchar_is_line_terminator(c))
                return RJS_REGEXP_REJECT;
        } else {
            if (c != -1)
                return RJS_REGEXP_REJECT;
        }
        break;
    case RJS_REGEXP_TERM_b:
        c  = get_char(rt, ctxt, ctxt->pos - 1);
        nc = get_char(rt, ctxt, ctxt->pos);

        if (is_word(c) == is_word(nc))
            return RJS_REGEXP_REJECT;
        break;
    case RJS_REGEXP_TERM_B:
        c  = get_char(rt, ctxt, ctxt->pos - 1);
        nc = get_char(rt, ctxt, ctxt->pos);

        if (is_word(c) != is_word(nc))
            return RJS_REGEXP_REJECT;
        break;
    case RJS_REGEXP_TERM_BR_ID:
        if (!(match_back_ref(rt, ctxt, term->t.br.index)))
            return RJS_REGEXP_REJECT;
        break;
    case RJS_REGEXP_TERM_BR_NAME:
        for (i = 0; i < ctxt->model->group_num; i ++) {
            if (ctxt->model->group_names[i] == term->t.br_name_id) {
                if ((ctxt->vec[i * 2] != -1) && (ctxt->vec[i * 2 + 1] != -1)) {
                    if (!(match_back_ref(rt, ctxt, i)))
                        return RJS_REGEXP_REJECT;
                    break;
                }
            }
        }
        break;
    case RJS_REGEXP_TERM_PATTERN:
        return match_pattern(rt, ctxt, &term->t.pattern);
    case RJS_REGEXP_TERM_GROUP:
        return match_group(rt, ctxt, term);
    case RJS_REGEXP_TERM_LA:
    case RJS_REGEXP_TERM_LA_NOT:
    case RJS_REGEXP_TERM_LB:
    case RJS_REGEXP_TERM_LB_NOT:
        if ((r = match_la_lb(rt, ctxt, term)) == RJS_REGEXP_REJECT)
            return r;
        break;
    }

    return RJS_REGEXP_NEXT;
}

/*Greedy next job.*/
static RJS_RegExpResult
job_greedy_next (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    RJS_REGEXP_LOG("job greedy next");

    ctxt->nextp = job->nextp;

    if (r == RJS_REGEXP_ACCEPT)
        return r;

    ctxt->pos = job->pos;
    restore_vec(rt, ctxt, job->vsp);

    return RJS_REGEXP_NEXT;
}

/*Match terminal job.*/
static RJS_Result
job_term (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    if (r == RJS_REGEXP_NEXT) {
        RJS_RegExpPattern *pat   = job->pattern;
        RJS_RegExpAlter   *alter = job->alter;
        RJS_RegExpTerm    *term  = job->term;
        int64_t            cnt   = job->count;
        size_t             pos   = job->pos;

        RJS_REGEXP_LOG("job term count:%d", cnt);

        return match_term(rt, ctxt, pat, alter, term, cnt, pos);
    }

    return r;
}

/*Non-greedy loop job.*/
static RJS_RegExpResult
job_non_greedy_loop (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    RJS_RegExpPattern *pat;
    RJS_RegExpAlter   *alter;
    RJS_RegExpTerm    *term;
    int64_t            cnt;
    size_t             jid;
    RJS_RegExpJob     *njob;

    RJS_REGEXP_LOG("job non-greedy loop:%d", job->count);

    ctxt->nextp = job->nextp;

    if (r == RJS_REGEXP_ACCEPT)
        return r;

    ctxt->pos = job->pos;
    restore_vec(rt, ctxt, job->vsp);

    pat   = job->pattern;
    alter = job->alter;
    term  = job->term;
    cnt   = job->count;

    jid  = push_job(rt, ctxt);
    njob = get_job(rt, ctxt, jid);

    njob->op       = job_term;
    njob->pattern  = pat;
    njob->alter    = alter;
    njob->term     = term;
    njob->count    = cnt + 1;
    njob->pos      = ctxt->pos;
    njob->nextp    = ctxt->nextp;

    ctxt->nextp = jid;

    if (cnt > 1) {
        int i;

        for (i = term->group_start; i < term->group_end; i ++) {
            ctxt->vec[i * 2]     = -1;
            ctxt->vec[i * 2 + 1] = -1;
        }
    }

    return match_atom(rt, ctxt, term);
}

/*Match the terminal.*/
static RJS_RegExpResult
match_term (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat, RJS_RegExpAlter *alter,
        RJS_RegExpTerm *term, int64_t cnt, size_t pos)
{
    if ((term->min == 0) && (term->max == 0))
        return RJS_REGEXP_NEXT;

    if (cnt > term->min) {
        if ((term->max != -1) && (cnt > term->max))
            return RJS_REGEXP_NEXT;

        if ((cnt > 1) && (pos == ctxt->pos))
            return RJS_REGEXP_NEXT;
    }

    if (cnt <= term->min) {
        size_t         jid;
        RJS_RegExpJob *job;

        jid = push_job(rt, ctxt);
        job = get_job(rt, ctxt, jid);

        job->op       = job_term;
        job->pattern  = pat;
        job->alter    = alter;
        job->term     = term;
        job->count    = cnt + 1;
        job->pos      = ctxt->pos;
        job->nextp    = ctxt->nextp;

        ctxt->nextp = jid;
    } else if (term->greedy) {
        size_t         jid;
        RJS_RegExpJob *job;

        jid = push_job(rt, ctxt);
        job = get_job(rt, ctxt, jid);

        job->vsp   = push_vec(rt, ctxt);
        job->op    = job_greedy_next;
        job->pos   = ctxt->pos;
        job->nextp = ctxt->nextp;

        save_vec(rt, ctxt, job->vsp);

        jid = push_job(rt, ctxt);
        job = get_job(rt, ctxt, jid);

        job->op      = job_term;
        job->pattern = pat;
        job->alter   = alter;
        job->term    = term;
        job->count   = cnt + 1;
        job->pos     = ctxt->pos;
        job->nextp   = ctxt->nextp;

        ctxt->nextp = jid;
    } else {
        size_t         jid;
        RJS_RegExpJob *job;

        jid = push_job(rt, ctxt);
        job = get_job(rt, ctxt, jid);

        job->vsp     = push_vec(rt, ctxt);
        job->op      = job_non_greedy_loop;
        job->pattern = pat;
        job->alter   = alter;
        job->term    = term;
        job->count   = cnt;
        job->pos     = ctxt->pos;
        job->nextp   = ctxt->nextp;

        save_vec(rt, ctxt, job->vsp);

        return RJS_REGEXP_NEXT;
    }

    if (cnt > 1) {
        int i;

        for (i = term->group_start; i < term->group_end; i ++) {
            ctxt->vec[i * 2]     = -1;
            ctxt->vec[i * 2 + 1] = -1;
        }
    }

    return match_atom(rt, ctxt, term);
}

/*Next alternative job.*/
static RJS_RegExpResult
job_alter (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    RJS_RegExpPattern *pat;
    RJS_RegExpAlter   *alter;

    RJS_REGEXP_LOG("job alter");

    ctxt->nextp = job->nextp;

    if (r == RJS_REGEXP_ACCEPT)
        return r;

    pat   = job->pattern;
    alter = job->alter;

    ctxt->pos = job->pos;

    restore_vec(rt, ctxt, job->vsp);

    return match_alter(rt, ctxt, pat, alter);
}

/*Match the terminal list job.*/
static RJS_RegExpResult
job_term_list (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpJob *job, RJS_RegExpResult r)
{
    if (r == RJS_REGEXP_NEXT) {
        RJS_RegExpPattern *pat   = job->pattern;
        RJS_RegExpAlter   *alter = job->alter;
        RJS_RegExpTerm    *term  = job->term;

        RJS_REGEXP_LOG("job term list");
        return match_term_list(rt, ctxt, pat, alter, term);
    }

    return r;
}

/*Match the terminal list.*/
static RJS_RegExpResult
match_term_list (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat,
        RJS_RegExpAlter *alter, RJS_RegExpTerm *term)
{
    RJS_RegExpTerm  *nterm;

    if (ctxt->reverse) {
        if (term->ln.prev != &alter->term_list)
            nterm = RJS_CONTAINER_OF(term->ln.prev, RJS_RegExpTerm, ln);
        else
            nterm = NULL;
    } else {
        if (term->ln.next != &alter->term_list)
            nterm = RJS_CONTAINER_OF(term->ln.next, RJS_RegExpTerm, ln);
        else
            nterm = NULL;
    }

    if (nterm) {
        size_t         jid;
        RJS_RegExpJob *job;

        jid = push_job(rt, ctxt);
        job = get_job(rt, ctxt, jid);

        job->op      = job_term_list;
        job->pattern = pat;
        job->alter   = alter;
        job->term    = nterm;
        job->nextp   = ctxt->nextp;

        ctxt->nextp = jid;
    }

    return match_term(rt, ctxt, pat, alter, term, 1, -1);
}

/*Match the alternative.*/
static RJS_RegExpResult
match_alter (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat, RJS_RegExpAlter *alter)
{
    RJS_RegExpTerm *term;

    if (alter->ln.next != &pat->alter_list) {
        RJS_RegExpAlter *nalter;
        size_t           jid;
        RJS_RegExpJob   *job;

        nalter = RJS_CONTAINER_OF(alter->ln.next, RJS_RegExpAlter, ln);
        jid    = push_job(rt, ctxt);
        job    = get_job(rt, ctxt, jid);

        job->vsp     = push_vec(rt, ctxt);
        job->pos     = ctxt->pos;
        job->pattern = pat;
        job->alter   = nalter;
        job->nextp   = ctxt->nextp;
        job->op      = job_alter;

        save_vec(rt, ctxt, job->vsp);
    }

    if (rjs_list_is_empty(&alter->term_list))
        return RJS_REGEXP_NEXT;

    if (ctxt->reverse)
        term = RJS_CONTAINER_OF(alter->term_list.prev, RJS_RegExpTerm, ln);
    else
        term = RJS_CONTAINER_OF(alter->term_list.next, RJS_RegExpTerm, ln);

    return match_term_list(rt, ctxt, pat, alter, term);
}

/*Match pattern operation.*/
static RJS_RegExpResult
match_pattern (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, RJS_RegExpPattern *pat)
{
    RJS_RegExpAlter *alter;

    if (rjs_list_is_empty(&pat->alter_list))
        return RJS_REGEXP_NEXT;

    alter = RJS_CONTAINER_OF(pat->alter_list.next, RJS_RegExpAlter, ln);

    return match_alter(rt, ctxt, pat, alter);
}

/*Match the regular expression.*/
static RJS_Bool
match (RJS_Runtime *rt, RJS_RegExpCtxt *ctxt, size_t pos)
{

    RJS_RegExpJob    *job;
    size_t            jid;
    int               i;
    RJS_RegExpResult  r;

    RJS_REGEXP_LOG("match from %d", pos);

    /*Store the position.*/
    ctxt->pos = pos;

    /*Clear the context.*/
    ctxt->reverse            = RJS_FALSE;
    ctxt->nextp              = -1;
    ctxt->vec_stack.item_num = 0;
    ctxt->job_stack.item_num = 0;

    for (i = 0; i < ctxt->model->group_num * 2; i ++)
        ctxt->vec[i] = -1;

    jid = push_job(rt, ctxt);
    job = get_job(rt, ctxt, jid);
    job->op     = job_success;
    job->nextp  = -1;
    ctxt->nextp = jid;

    /*Store the start position to the result vector.*/
    ctxt->vec[0] = pos;

    r = match_pattern(rt, ctxt, ctxt->model->pattern);

    /*Run the jobs and watchers.*/
    while (ctxt->job_stack.item_num) {
        RJS_REGEXP_LOG("%s", (r == RJS_REGEXP_ACCEPT) ? "accept"
                : ((r == RJS_REGEXP_REJECT) ? "reject" : "next"));

        if (r == RJS_REGEXP_NEXT) {
            job = get_job(rt, ctxt, ctxt->nextp);

            ctxt->nextp = job->nextp;

            if ((r = job->op(rt, ctxt, job, r)) == RJS_REGEXP_SUCCESS)
                break;
        } else {
            jid = ctxt->job_stack.item_num - 1;
            job = get_job(rt, ctxt, jid);

            pop_job(rt, ctxt);

            r = job->op(rt, ctxt, job, r);
        }
    }

    return (r == RJS_REGEXP_SUCCESS) ? RJS_TRUE : RJS_FALSE;
}

/**
 * Execute the regular expression built-in process.
 * \param rt The current runtime.
 * \param v The regular expression value.
 * \param str The string.
 * \param[out] rv Return the match result.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_builtin_exec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *str, RJS_Value *rv)
{
    RJS_RegExpCtxt   ctxt;
    RJS_Result       r;
    int64_t          last_idx, end_idx;
    int              i;
    RJS_Bool         has_group, has_indices;
    RJS_RegExp      *re      = (RJS_RegExp*)rjs_value_get_object(rt, v);
    RJS_RegExpModel *rem     = re->model;
    ssize_t          vec[rem->group_num * 2];
    RJS_Value       *gitems  = NULL;
    RJS_Value       *igitems = NULL;
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *sub     = rjs_value_stack_push(rt);
    RJS_Value       *idx     = rjs_value_stack_push(rt);
    RJS_Value       *groups  = rjs_value_stack_push(rt);
    RJS_Value       *indices = rjs_value_stack_push(rt);
    RJS_Value       *igroups = rjs_value_stack_push(rt);
    RJS_Value       *startp  = rjs_value_stack_push(rt);
    RJS_Value       *endp    = rjs_value_stack_push(rt);
    RJS_Value       *indice  = rjs_value_stack_push(rt);

    assert(rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_REGEXP);
    assert(rjs_value_is_string(rt, str));

    rjs_value_copy(rt, &ctxt.str, str);
    ctxt.len   = rjs_string_get_length(rt, str);
    ctxt.re    = re;
    ctxt.model = rem;
    ctxt.flags = rem->flags;
    ctxt.vec   = vec;

    rjs_vector_init(&ctxt.vec_stack);
    rjs_vector_init(&ctxt.job_stack);

    if ((r = get_last_index(rt, v, &last_idx)) == RJS_ERR)
        goto end;

    if (!(ctxt.model->flags & (RJS_REGEXP_FL_G|RJS_REGEXP_FL_Y)))
        last_idx = 0;

    /*Match the regular expression.*/
    while (1) {
        if (last_idx > ctxt.len) {
            if (ctxt.flags & (RJS_REGEXP_FL_G|RJS_REGEXP_FL_Y)) {
                if ((r = rjs_set_number(rt, v, rjs_pn_lastIndex(rt), 0, RJS_TRUE)) == RJS_ERR)
                    goto end;
            }

            rjs_value_set_null(rt, rv);
            r = RJS_OK;
            goto end;
        }

        r = match(rt, &ctxt, last_idx);

        if (!r) {
            if (ctxt.flags & RJS_REGEXP_FL_Y) {
                if ((r = rjs_set_number(rt, v, rjs_pn_lastIndex(rt), 0, RJS_TRUE)) == RJS_ERR)
                    goto end;
                
                rjs_value_set_null(rt, rv);
                r = RJS_OK;
                goto end;
            }

            last_idx = adv_str_index(rt, &ctxt, last_idx, ctxt.flags & RJS_REGEXP_FL_U);
        } else {
            break;
        }
    }

    /*Update the last index.*/
    end_idx = ctxt.vec[1];

    if (ctxt.flags & (RJS_REGEXP_FL_G|RJS_REGEXP_FL_Y)) {
        if ((r = rjs_set_number(rt, v, rjs_pn_lastIndex(rt), end_idx, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    /*Create the result array.*/
    if ((r = rjs_array_new(rt, rv, ctxt.model->group_num, NULL)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, idx, last_idx);
    rjs_create_data_property_or_throw(rt, rv, rjs_pn_index(rt), idx);
    rjs_create_data_property_or_throw(rt, rv, rjs_pn_input(rt), str);

    rjs_string_substr(rt, str, ctxt.vec[0], ctxt.vec[1], sub);
    rjs_create_data_property_or_throw_index(rt, rv, 0, sub);

    has_group   = ctxt.model->group_names ? RJS_TRUE : RJS_FALSE;
    has_indices = (ctxt.flags & RJS_REGEXP_FL_D) ? RJS_TRUE : RJS_FALSE;

    if (has_indices) {
        if ((r = rjs_array_new(rt, indices, ctxt.model->group_num, NULL)) == RJS_ERR)
            goto end;

        if (has_group) {
            rjs_object_new(rt, igroups, rjs_v_null(rt));

            igitems = rjs_value_stack_push_n(rt, ctxt.model->name_num);
        } else {
            rjs_value_set_undefined(rt, igroups);
        }

        rjs_create_data_property_or_throw(rt, indices, rjs_pn_groups(rt), igroups);
    }

    if (has_group) {
        rjs_object_new(rt, groups, rjs_v_null(rt));

        gitems = rjs_value_stack_push_n(rt, ctxt.model->name_num);
    } else {
        rjs_value_set_undefined(rt, groups);
    }

    rjs_create_data_property_or_throw(rt, rv, rjs_pn_groups(rt), groups);

    if (has_indices) {
        rjs_value_set_number(rt, startp, vec[0]);
        rjs_value_set_number(rt, endp, vec[1]);
        rjs_create_array_from_elements(rt, indice, startp, endp, NULL);
        rjs_create_data_property_or_throw_index(rt, indices, 0, indice);
    }

    for (i = 1; i < ctxt.model->group_num; i ++) {
        ssize_t start, end;

        start = ctxt.vec[i * 2];
        end   = ctxt.vec[i * 2 + 1];

        if (start > end) {
            ssize_t tmp = start;

            start = end;
            end   = tmp;
        }

        if ((start == -1) || (end == -1)) {
            rjs_value_set_undefined(rt, sub);

            if (has_indices)
                rjs_value_set_undefined(rt, indice);
        } else {
            rjs_string_substr(rt, str, start, end, sub);
            rjs_create_data_property_or_throw_index(rt, rv, i, sub);

            if (has_indices) {
                rjs_value_set_number(rt, startp, start);
                rjs_value_set_number(rt, endp, end);
                rjs_create_array_from_elements(rt, indice, startp, endp, NULL);
            }

            if (has_group) {
                int gn = ctxt.model->group_names[i];

                if (gn != -1) {
                    RJS_Value *dst;

                    dst = rjs_value_buffer_item(rt, gitems, gn);
                    rjs_value_copy(rt, dst, sub);

                    if (has_indices) {
                        dst = rjs_value_buffer_item(rt, igitems, gn);
                        rjs_value_copy(rt, dst, indice);
                    }
                }
            }
        }

        if (has_indices) {
            rjs_create_data_property_or_throw_index(rt, indices, i, indice);
        }
    }

    if (has_indices)
        rjs_create_data_property_or_throw(rt, rv, rjs_pn_indices(rt), indices);

    if (has_group) {
        int i;

        for (i = 0; i < ctxt.model->name_num; i ++) {
            RJS_PropertyName gn_pn;
            RJS_Value       *v;

            rjs_property_name_init(rt, &gn_pn, &ctxt.model->names[i]);

            v = rjs_value_buffer_item(rt, gitems, i);
            rjs_create_data_property_or_throw(rt, groups, &gn_pn, v);

            if (has_indices) {
                v = rjs_value_buffer_item(rt, igitems, i);
                rjs_create_data_property_or_throw(rt, igroups, &gn_pn, v);
            }

            rjs_property_name_deinit(rt, &gn_pn);
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    rjs_vector_deinit(&ctxt.vec_stack, rt);
    rjs_vector_deinit(&ctxt.job_stack, rt);
    return r;
}

/**
 * Execute the regular expression.
 * \param rt The current runtime.
 * \param v The regular expression value.
 * \param str The string.
 * \param[out] rv Return the match result.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_regexp_exec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *str, RJS_Value *rv)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *exec = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get(rt, v, rjs_pn_exec(rt), exec)) == RJS_ERR)
        goto end;

    if (rjs_is_callable(rt, exec)) {
        if ((r = rjs_call(rt, exec, v, str, 1, rv)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_object(rt, rv) && !rjs_value_is_null(rt, rv)) {
            r = rjs_throw_type_error(rt, _("the \"exec\" result is not an object or null"));
            goto end;
        }

        goto end;
    }

    if (rjs_value_get_gc_thing_type(rt, v) != RJS_GC_THING_REGEXP) {
        r = rjs_throw_type_error(rt, _("the value is not an regular expression"));
        goto end;
    }

    r = rjs_regexp_builtin_exec(rt, v, str, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}
