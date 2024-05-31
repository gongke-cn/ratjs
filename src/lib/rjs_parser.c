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

/**Private identifier for accessor's getter.*/
#define PRIV_ID_FL_GET    1
/**Private identifier for accessor's setter.*/
#define PRIV_ID_FL_SET    2
/**Private identifier for static method.*/
#define PRIV_ID_FL_STATIC 4

/**Contains await expression.*/
#define CONTAINS_FL_AWAIT_EXPR 1
/**Contains yield expression.*/
#define CONTAINS_FL_YIELD_EXPR 2
/**Contains super call.*/
#define CONTAINS_FL_SUPER_CALL 4
/**Contains super property expression.*/
#define CONTAINS_FL_SUPER_PROP 8
/**Contains new target.*/
#define CONTAINS_FL_NEW_TARGET 16
/**Contains arguments.*/
#define CONTAINS_FL_ARGUMENTS  32
/**Contains await.*/
#define CONTAINS_FL_AWAIT      64

/**Error recover type.*/
typedef enum {
    RECOVER_SCRIPT, /**< Script.*/
    RECOVER_MODULE, /**< Module.*/
    RECOVER_BLOCK,  /**< Block.*/
    RECOVER_SWITCH, /**< Switch statements.*/
    RECOVER_CLASS   /**< Class.*/
} RecoverType;

/**Priority.*/
typedef enum {
    PRIO_LOWEST, /**< Lowest priority.*/
    PRIO_COMMA,  /**< , */
    PRIO_ASSI,   /**< Assignment.*/
    PRIO_COND,   /**< Conditional expression.*/
    PRIO_QUES,   /**< ?? */
    PRIO_OR,     /**< || */
    PRIO_AND,    /**< && */
    PRIO_BOR,    /**< | */
    PRIO_BXOR,   /**< ^ */
    PRIO_BAND,   /**< & */
    PRIO_EQ,     /**< ==, !=, ===, !== */
    PRIO_REL,    /**< <. >, <=, >= */
    PRIO_SHIFT,  /**< <<, >>, >>> */
    PRIO_ADD,    /**< +, - */
    PRIO_MUL,    /**< *, /, % */
    PRIO_EXP,    /**< ** */
    PRIO_UNARY,  /**< Unary expression.*/
    PRIO_UPDATE, /**< ++, -- */
    PRIO_LH,     /**< Left hand.*/
    PRIO_NEW,    /**< New expression.*/
    PRIO_MEMBER, /**< Member expression.*/
    PRIO_HIGHEST /**< Highest priority.*/
} Priority;

/**Attributes of parameters.*/
typedef struct {
    RJS_Bool has_dup;  /**< Has duplicate parameters.*/
    RJS_Bool simple;   /**< Simple parameters.*/
    RJS_Bool has_expr; /**< Has expression in the parameters.*/
    RJS_Bool has_args; /**< Has a parameter \"arguments\".*/
} ParamAttr;

/*Check if the token is the identifier.*/
static RJS_Bool
token_is_identifier (RJS_TokenType type, int flags, RJS_IdentifierType itype);
/*Check if the expression is valid.*/
static RJS_Bool
check_expr (RJS_Runtime *rt, RJS_Value *v);
/*Check the binding element list in parameters.*/
static void
binding_element_list_param (RJS_Runtime *rt, RJS_List *l, ParamAttr *pa);
/*Add declaration in binding elements.*/
static void
binding_element_list_lex (RJS_Runtime *rt, RJS_AstDeclType dtype, RJS_List *list);
/*Parse the expression with in flag and priority.*/
static RJS_Result
parse_expr_in_prio (RJS_Runtime *rt, Priority prio, RJS_Value *ve);
/*Parse the expression with in flag.*/
static RJS_Result
parse_expr_in (RJS_Runtime *rt, RJS_Value *ve);
/*Parse the expression with pririty.*/
static RJS_Result
parse_expr_prio (RJS_Runtime *rt, Priority prio, RJS_Value *ve);
/*Parse the property name.*/
static RJS_Result
parse_property_name (RJS_Runtime *rt, RJS_Value *vn);
/*Parse binding.*/
static RJS_Result
parse_binding (RJS_Runtime *rt, RJS_Value *vb);
/*Parse binding element.*/
static RJS_Result
parse_binding_element (RJS_Runtime *rt, RJS_Value *vb, RJS_Value *vinit);
/*Parse the method's parameters and body.*/
static RJS_Result
parse_method_params_body (RJS_Runtime *rt, RJS_Bool in_obj, RJS_AstClassElem *m, int flags);
/*Parse the method.*/
static RJS_Result
parse_method (RJS_Runtime *rt, RJS_Bool is_static, RJS_Bool in_obj, RJS_Value *vm);
/*Parse the class element name.*/
static RJS_Result
parse_class_element_name (RJS_Runtime *rt, RJS_Bool in_obj, RJS_Bool is_static,
        int flags, RJS_Value *vn);
/*Parse function body.*/
static RJS_Result
parse_func_body (RJS_Runtime *rt);
/*Parse the hoistable declaration.*/
static RJS_Result
parse_hoistable_decl (RJS_Runtime *rt, RJS_Bool is_expr, RJS_Value *vf);
/*Parse the class declaration.*/
static RJS_Result
parse_class_decl (RJS_Runtime *rt, RJS_Bool is_expr, RJS_Value *vc);
/*Parse a statement.*/
static RJS_Result
parse_stmt (RJS_Runtime *rt, RJS_Value *vs);
/*Parse a statement list item.*/
static RJS_Result
parse_stmt_list_item (RJS_Runtime *rt, RJS_Value *vs);
/*Convert the expression to binding.*/
static RJS_Result
expr_to_binding (RJS_Runtime *rt, RJS_Ast *e, RJS_Value *b, RJS_Bool is_binding);
/*Convert the expression to binding and initializer.*/
static RJS_Result
expr_to_binding_init (RJS_Runtime *rt, RJS_Ast *e, RJS_Value *b, RJS_Value *init, RJS_Bool is_binding);
/*Output parse error message.*/
static void
parse_error (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...);
/*Output parse warning message.*/
static void
parse_warning (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...);
/*Output previous definition note message.*/
static void
parse_prev_define_note (RJS_Runtime *rt, RJS_Location *loc);

#if ENABLE_MODULE
/*Export the binding.*/
static void
export_binding (RJS_Runtime *rt, RJS_Value *vb, RJS_List *list);
#endif /*ENABLE_MODULE*/

/*Initialize the hash table.*/
static void
hash_init (RJS_Hash *hash)
{
    rjs_hash_init(hash);
}

/*Release the hash table.*/
static void
hash_deinit (RJS_Runtime *rt, RJS_Hash *hash)
{
    rjs_hash_deinit(hash, &rjs_hash_string_ops, rt);
}

/*Lookup the hash table.*/
static RJS_HashEntry*
hash_lookup (RJS_Runtime *rt, RJS_Hash *hash, RJS_Value *v, RJS_HashEntry ***phe)
{
    RJS_String    *str;
    RJS_HashEntry *he;
    RJS_Result     r;

    str = rjs_value_get_string(rt, v);

    r = rjs_hash_lookup(hash, str, &he, phe, &rjs_hash_string_ops, rt);

    return r ? he : NULL;
}

/*Insert an entry to the hash table.*/
static void
hash_insert (RJS_Runtime *rt, RJS_Hash *hash, RJS_Value *v, RJS_HashEntry *he, RJS_HashEntry **phe)
{
    RJS_String *str;

    str = rjs_value_get_string(rt, v);

    rjs_hash_insert(hash, str, he, phe, &rjs_hash_string_ops, rt);
}

/*Scan the AST node list.*/
static void
gc_scan_ast_list (RJS_Runtime *rt, RJS_List *l)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(l, ast, RJS_Ast, ln) {
        rjs_gc_mark(rt, ast);
    }
}

/*Scan the referenced GC things in the AST node.*/
static void
ast_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Ast    *ast = ptr;
    RJS_AstOps *ops = (RJS_AstOps*)ast->gc_thing.ops;
    size_t      addr;
    RJS_Value  *pv;
    RJS_List   *pl;
    RJS_Hash   *ph;
    size_t      i;

    addr = RJS_PTR2SIZE(ast);

    pv = RJS_SIZE2PTR(addr + ops->value_offset);
    for (i = 0; i < ops->value_num; i ++) {
        rjs_gc_scan_value(rt, pv);
        pv ++;
    }

    pl = RJS_SIZE2PTR(addr + ops->list_offset);
    for (i = 0; i < ops->list_num; i ++) {
        gc_scan_ast_list(rt, pl);
        pl ++;
    }

    ph = RJS_SIZE2PTR(addr + ops->hash_offset);
    for (i = 0; i < ops->hash_num; i ++) {
        RJS_HashEntry *he;
        size_t         id;

        rjs_hash_foreach(ph, id, he) {
            rjs_gc_mark(rt, he->key);
        }

        ph ++;
    }
}

/*Free the AST node.*/
static void
ast_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Ast    *ast = ptr;
    RJS_AstOps *ops = (RJS_AstOps*)ast->gc_thing.ops;
    size_t      addr;
    RJS_Hash   *ph;
    size_t      i;

    addr = RJS_PTR2SIZE(ast);

    ph = RJS_SIZE2PTR(addr + ops->hash_offset);
    for (i = 0; i < ops->hash_num; i ++) {
        hash_deinit(rt, ph);
        ph ++;
    }

    rjs_free(rt, ast, ops->size);
}

#include <rjs_ast_inc.c>

/*Initialize the location.*/
static void
loc_init (RJS_Location *loc)
{
    loc->first_line   = 1;
    loc->first_column = 1;
    loc->last_line    = 1;
    loc->last_column  = 1;
}

/*Update the first character's location.*/
static void
loc_update_first (RJS_Location *loc, RJS_Location *first)
{
    loc->first_line   = first->first_line;
    loc->first_column = first->first_column;
    loc->first_pos    = first->first_pos;
}

/*Update the last character's location.*/
static void
loc_update_last (RJS_Location *loc, RJS_Location *last)
{
    loc->last_line   = last->last_line;
    loc->last_column = last->last_column;
    loc->last_pos    = last->last_pos;
}

/*Update the last character's location from a token.*/
static void
loc_update_last_token (RJS_Runtime *rt, RJS_Location *loc)
{
    RJS_Parser *parser = rt->parser;
    RJS_Token  *tok    = &parser->curr_token;

    loc_update_last(loc, &tok->location);
}

/*Allocate a new AST node.*/
static void*
ast_new (RJS_Runtime *rt, RJS_Value *v, RJS_AstType type, RJS_Location *loc)
{
    RJS_AstModelType  model = ast_type_model_tab[type];
    const RJS_AstOps *ops   = &ast_ops_tab[model];
    RJS_Ast          *ast;
    size_t            addr;
    RJS_Value        *pv;
    RJS_List         *pl;
    RJS_Hash         *ph;
    size_t            n;

    ast = rjs_alloc(rt, ops->size);

    ast->type = type;

    if (loc)
        ast->location = *loc;
    else
        loc_init(&ast->location);

    addr = RJS_PTR2SIZE(ast);

    pv = RJS_SIZE2PTR(addr + ops->value_offset);
    n  = ops->value_num;
    while (n --) {
        rjs_value_set_undefined(rt, pv);
        pv ++;
    }

    pl = RJS_SIZE2PTR(addr + ops->list_offset);
    n  = ops->list_num;
    while (n --) {
        rjs_list_init(pl);
        pl ++;
    }

    ph = RJS_SIZE2PTR(addr + ops->hash_offset);
    n  = ops->hash_num;
    while (n --) {
        hash_init(ph);
        ph ++;
    }

    rjs_value_set_gc_thing(rt, v, ast);
    rjs_gc_add(rt, ast, &ops->gc_thing_ops);

    return ast;
}

/*Get the AST node from the value.*/
static void*
ast_get (RJS_Runtime *rt, RJS_Value *v)
{
    if (rjs_value_is_undefined(rt, v))
        return NULL;

    return (RJS_Ast*)rjs_value_get_gc_thing(rt, v);
}

/*Append an AST node to the list.*/
static void
ast_list_append (RJS_Runtime *rt, RJS_List *list, RJS_Value *v)
{
    RJS_Ast *ast = ast_get(rt, v);

    if (ast)
        rjs_list_append(list, &ast->ln);
}

/*Update the location's last character postion from the AST node.*/
static RJS_Bool
loc_update_last_ast (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *v)
{
    RJS_Ast *ast = ast_get(rt, v);

    if (!ast)
        return RJS_FALSE;

    loc_update_last(loc, &ast->location);
    return RJS_TRUE;
}

/*Add a value entry.*/
static RJS_AstValueEntry*
value_entry_add (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *v)
{
    RJS_Parser        *parser = rt->parser;
    RJS_HashEntry     *he, **phe;
    RJS_AstValueEntry *ve;
    RJS_Result         r;

    r = rjs_hash_lookup(&parser->value_entry_hash, v, &he, &phe,
            &rjs_hash_value_ops, rt);
    if (r) {
        ve = RJS_CONTAINER_OF(he, RJS_AstValueEntry, he);
    } else {
        size_t     top = rjs_value_stack_save(rt);
        RJS_Value *tmp = rjs_value_stack_push(rt);

        ve = ast_new(rt, tmp, RJS_AST_ValueEntry, loc);

        ve->id = -1;

        rjs_value_copy(rt, &ve->value, v);

        rjs_hash_insert(&parser->value_entry_hash, &ve->value, &ve->he, phe,
                &rjs_hash_value_ops, rt);
        ast_list_append(rt, &parser->value_entry_list, tmp);

        rjs_value_stack_restore(rt, top);
    }

    return ve;
}

/*Create a value expression.*/
static RJS_AstValueExpr*
value_expr_new (RJS_Runtime *rt, RJS_Value *ev, RJS_Location *loc, RJS_Value *v)
{
    RJS_AstValueExpr *ve;

    ve = ast_new(rt, ev, RJS_AST_ValueExpr, loc);

    ve->ve    = value_entry_add(rt, loc, v);
    ve->flags = 0;

    return ve;
}

/*Get the top function in the stack.*/
static RJS_AstFunc*
func_top (RJS_Runtime *rt)
{
    RJS_Parser  *parser = rt->parser;
    RJS_AstFunc *func   = parser->func_stack;

    assert(func);

    return func;
}

/*Push a declaration to the stack.*/
static RJS_AstDecl*
decl_push (RJS_Runtime *rt)
{
    RJS_Parser  *parser = rt->parser;
    size_t       top    = rjs_value_stack_save(rt);
    RJS_Value   *vdecl  = rjs_value_stack_push(rt);
    RJS_AstFunc *func   = func_top(rt);
    RJS_AstDecl *decl;

    decl = ast_new(rt, vdecl, RJS_AST_Decl, NULL);

    decl->id              = -1;
    decl->func            = func;
    decl->binding_ref_num = 0;

    decl->bot = parser->decl_stack;
    parser->decl_stack = decl;

    rjs_list_append(&parser->decl_list, &decl->ast.ln);

    rjs_value_stack_restore(rt, top);
    return decl;
}

/*Get the top declaration in the stack.*/
static RJS_AstDecl*
decl_top (RJS_Runtime *rt)
{
    RJS_Parser  *parser = rt->parser;
    RJS_AstDecl *decl   = parser->decl_stack;

    assert(decl);

    return decl;
}

/*Check if the declaration is a lexically declaration.*/
static RJS_Bool
decl_is_lex (RJS_Runtime *rt, RJS_AstDeclType type)
{
    RJS_AstDecl *decl = decl_top(rt);
    RJS_AstFunc *func;
    RJS_Bool     r;

    switch (type) {
    case RJS_AST_DECL_LET:
    case RJS_AST_DECL_CONST:
    case RJS_AST_DECL_STRICT:
    case RJS_AST_DECL_CLASS:
        r = RJS_TRUE;
        break;
    case RJS_AST_DECL_VAR:
    case RJS_AST_DECL_PARAMETER:
        r = RJS_FALSE;
        break;
    case RJS_AST_DECL_FUNCTION:
        func = func_top(rt);

        if (func->flags & RJS_AST_FUNC_FL_MODULE)
            r = RJS_TRUE;
        else
            r = func->lex_decl != decl;
        break;
    default:
        assert(0);
        r = RJS_TRUE;
    }

    return r;
}

/*Add a binding reference.*/
static RJS_AstBindingRef*
binding_ref_new (RJS_Runtime *rt, RJS_AstDecl *decl, RJS_Location *loc, RJS_Value *id)
{
    RJS_AstBindingRef *br;
    RJS_HashEntry     *he, **phe;

    he = hash_lookup(rt, &decl->binding_ref_hash, id, &phe);
    if (he) {
        br = RJS_CONTAINER_OF(he, RJS_AstBindingRef, he);
    } else {
        size_t     top  = rjs_value_stack_save(rt);
        RJS_Value *tmp  = rjs_value_stack_push(rt);

        br = ast_new(rt, tmp, RJS_AST_BindingRef, loc);

        br->id   = -1;
        br->decl = decl;
        br->name = value_entry_add(rt, loc, id);

        hash_insert(rt, &decl->binding_ref_hash, &br->name->value, &br->he, phe);
        ast_list_append(rt, &decl->binding_ref_list, tmp);

        rjs_value_stack_restore(rt, top);
    }
    return br;
}

/*Add a declaration item.*/
static RJS_AstDeclItem*
decl_item_add (RJS_Runtime *rt, RJS_AstDeclType type, RJS_Location *loc,
        RJS_Value *id, RJS_Bool *dup)
{
    RJS_HashEntry   *he, **phe;
    RJS_AstDecl     *decl, *ref_decl;
    RJS_AstFunc     *func   = func_top(rt);
    RJS_Parser      *parser = rt->parser;
    RJS_AstDeclItem *di     = NULL;
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *tmp    = rjs_value_stack_push(rt);

    if (dup)
        *dup = RJS_FALSE;

    if (decl_is_lex(rt, type) || (type == RJS_AST_DECL_PARAMETER))
        ref_decl = decl_top(rt);
    else
        ref_decl = func->var_decl;

    decl = decl_top(rt);

    he = hash_lookup(rt, &decl->item_hash, id, &phe);
    if (he) {
        RJS_Bool can_dup = RJS_FALSE;

        di = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

        if ((di->type == RJS_AST_DECL_PARAMETER) && (type == RJS_AST_DECL_PARAMETER)) {
            if (!(parser->flags & RJS_PARSE_FL_STRICT))
                can_dup = RJS_TRUE;
        } else {
            if (!decl_is_lex(rt, type) && !decl_is_lex(rt, di->type))
                can_dup = RJS_TRUE;
        }

        if (can_dup) {
            parse_warning(rt, loc, _("\"%s\" is already defined"),
                    rjs_string_to_enc_chars(rt, id, NULL, NULL));
        } else {
            parse_error(rt, loc, _("\"%s\" is already defined"),
                    rjs_string_to_enc_chars(rt, id, NULL, NULL));
        }

        parse_prev_define_note(rt, &di->ast.location);

        if (dup)
            *dup = RJS_TRUE;
    } else {
        di = ast_new(rt, tmp, RJS_AST_DeclItem, loc);

        di->type        = type;
        di->binding_ref = binding_ref_new(rt, ref_decl, loc, id);

        hash_insert(rt, &decl->item_hash, &di->binding_ref->name->value, &di->he, phe);
        ast_list_append(rt, &decl->item_list, tmp);

        /*Add the variable to the upper declarations.*/
        if (ref_decl != decl) {
            decl = decl->bot;

            while (1) {
                RJS_AstDeclItem *var_di;

                he = hash_lookup(rt, &decl->item_hash, id, &phe);
                if (he) {
                    var_di = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

                    if ((var_di->type != RJS_AST_DECL_VAR)
                            && (var_di->type != RJS_AST_DECL_PARAMETER)) {
                        parse_error(rt, loc, _("\"%s\" is already defined"),
                                rjs_string_to_enc_chars(rt, id, NULL, NULL));
                    } else {
                        parse_warning(rt, loc, _("\"%s\" is already defined"),
                                rjs_string_to_enc_chars(rt, id, NULL, NULL));
                    }

                    parse_prev_define_note(rt, &var_di->ast.location);
                } else {
                    var_di = ast_new(rt, tmp, RJS_AST_DeclItem, loc);

                    var_di->type        = type;
                    var_di->binding_ref = di->binding_ref;

                    hash_insert(rt, &decl->item_hash, &var_di->binding_ref->name->value, &var_di->he, phe);
                    ast_list_append(rt, &decl->item_list, tmp);
                }

                if (decl == ref_decl)
                    break;

                decl = decl->bot;
            }
        }
    }

    rjs_value_stack_restore(rt, top);
    return di;
}

/*Create a new binding table.*/
static RJS_AstBindingTable*
binding_table_new (RJS_Runtime *rt, RJS_Value *v, RJS_AstDecl *decl)
{
    RJS_AstBindingTable *bt;
    RJS_Parser          *parser = rt->parser;

    bt = ast_get(rt, v);

    if (!bt) {
        bt = ast_new(rt, v, RJS_AST_BindingTable, NULL);

        bt->num  = 0;
        bt->decl = decl;
        bt->id   = -1;

        rjs_list_append(&parser->binding_table_list, &bt->ast.ln);
    }

    return bt;
}

/*Create a new function table.*/
static RJS_AstFuncTable*
func_table_new (RJS_Runtime *rt, RJS_Value *v, RJS_AstDecl *decl)
{
    RJS_AstFuncTable *ft;
    RJS_Parser       *parser = rt->parser;

    ft = ast_get(rt, v);

    if (!ft) {
        ft = ast_new(rt, v, RJS_AST_FuncTable, NULL);

        ft->num  = 0;
        ft->decl = decl;
        ft->id   = -1;

        rjs_list_append(&parser->func_table_list, &ft->ast.ln);
    }

    return ft;
}

/*Add a function declaration reference to the list.*/
static RJS_AstFuncDeclRef*
func_decl_ref_new (RJS_Runtime *rt, RJS_AstDeclItem *di, RJS_AstFunc *func)
{
    RJS_AstFunc        *cf   = func_top(rt);
    RJS_Ast            *bast = cf->block_stack;
    RJS_AstFuncTable   *ft   = NULL;
    RJS_AstFuncDeclRef *fdr;
    size_t              top;
    RJS_Value          *tmp;

    if (bast) {
        if (bast->type == RJS_AST_Block) {
            RJS_AstBlock *blk = (RJS_AstBlock*)bast;

            ft = func_table_new(rt, &blk->func_table, blk->decl);
        } else if (bast->type == RJS_AST_SwitchStmt) {
            RJS_AstSwitchStmt *ss = (RJS_AstSwitchStmt*)bast;

            ft = func_table_new(rt, &ss->func_table, ss->decl);
        } else {
            assert(0);
        }
    } else {
        ft = func_table_new(rt, &cf->func_table, cf->var_decl);
    }

    rjs_list_foreach_c(&ft->func_decl_ref_list, fdr, RJS_AstFuncDeclRef, ast.ln) {
        if (fdr->decl_item == di) {
            fdr->ast.location = func->ast.location;
            fdr->func = func;
            return fdr;
        }
    }

    top = rjs_value_stack_save(rt);
    tmp = rjs_value_stack_push(rt);

    fdr = ast_new(rt, tmp, RJS_AST_FuncDeclRef, &func->ast.location);

    fdr->decl_item = di;
    fdr->func      = func;

    ast_list_append(rt, &ft->func_decl_ref_list, tmp);
    ft->num ++;

    rjs_value_stack_restore(rt, top);

    return fdr;
}

/*Popup the top declaration in the stack.*/
static RJS_AstDecl*
decl_pop (RJS_Runtime *rt)
{
    RJS_Parser  *parser = rt->parser;
    RJS_AstDecl *decl   = parser->decl_stack;

    if (decl)
        parser->decl_stack = decl->bot;

    return decl;
}

/*Create an identifier.*/
static RJS_AstId*
id_new (RJS_Runtime *rt, RJS_Value *v, RJS_Location *loc, RJS_Value *i)
{
    RJS_AstId *id;

    id = ast_new(rt, v, RJS_AST_Id, loc);

    id->identifier = value_entry_add(rt, loc, i);
    id->flags      = 0;

    return id;
}

/*Create a new property reference.*/
static RJS_AstPropRef*
prop_ref_new (RJS_Runtime *rt, RJS_Value *v, RJS_Location *loc, RJS_AstFunc *func, RJS_Value *id)
{
    RJS_AstPropRef *pr;
    RJS_HashEntry  *he, **phe;
    RJS_Parser     *parser = rt->parser;

    he = hash_lookup(rt, &func->prop_ref_hash, id, &phe);
    if (he) {
        pr = RJS_CONTAINER_OF(he, RJS_AstPropRef, he);
        rjs_value_set_gc_thing(rt, v, pr);
        return pr;
    }

    pr = ast_new(rt, v, RJS_AST_PropRef, loc);

    pr->func = func;
    pr->prop = value_entry_add(rt, loc, id);
    pr->id   = -1;

    rjs_list_append(&parser->prop_ref_list, &pr->ast.ln);
    hash_insert(rt, &func->prop_ref_hash, &pr->prop->value, &pr->he, phe);

    return pr;
}

/*Push a contains list.*/
static RJS_AstContainsListStack*
contains_list_push (RJS_Runtime *rt)
{
    size_t                    top    = rjs_value_stack_save(rt);
    RJS_Value                *vt     = rjs_value_stack_push(rt);
    RJS_Parser               *parser = rt->parser;
    RJS_AstContainsListStack *cls;

    cls = ast_new(rt, vt, RJS_AST_ContainsListStack, NULL);

    cls->bot = parser->contains_list_stack;
    parser->contains_list_stack = cls;

    rjs_value_stack_restore(rt, top);
    return cls;
}

/*Popup the current contains list.*/
static void
contains_list_pop (RJS_Runtime *rt, RJS_Bool join)
{
    RJS_Parser               *parser = rt->parser;
    RJS_AstContainsListStack *cls    = parser->contains_list_stack;
    RJS_AstContainsListStack *bot    = cls->bot;

    if (bot && join) {
        rjs_list_join(&bot->list, &cls->list);
        rjs_list_init(&cls->list);
    }

    parser->contains_list_stack = bot;
}

/*Add a node to the contains list.*/
static void
contains_list_add (RJS_Runtime *rt, RJS_AstType type, RJS_Location *loc)
{
    size_t                    top    = rjs_value_stack_save(rt);
    RJS_Value                *vt     = rjs_value_stack_push(rt);
    RJS_Parser               *parser = rt->parser;
    RJS_AstContainsListStack *cls    = parser->contains_list_stack;

    if (cls) {
        ast_new(rt, vt, type, loc);
        ast_list_append(rt, &cls->list, vt);
    }

    rjs_value_stack_restore(rt, top);
}

/*Check the contains list.*/
static RJS_Bool
contains_list_check (RJS_Runtime *rt, int flags)
{
    RJS_Bool                  r      = RJS_TRUE;
    RJS_Parser               *parser = rt->parser;
    RJS_AstContainsListStack *cls    = parser->contains_list_stack;

    if (cls) {
        RJS_Ast *n;

        rjs_list_foreach_c(&cls->list, n, RJS_Ast, ln) {
            if ((flags & CONTAINS_FL_AWAIT_EXPR) && (n->type == RJS_AST_AwaitExprRef)) {
                parse_error(rt, &n->location, _("await expression cannot be used here"));
                r = RJS_FALSE;
            } else if ((flags & CONTAINS_FL_YIELD_EXPR) && (n->type == RJS_AST_YieldExprRef)) {
                parse_error(rt, &n->location, _("yield expression cannot be used here"));
                r = RJS_FALSE;
            } else if ((flags & CONTAINS_FL_SUPER_CALL) && (n->type == RJS_AST_SuperCallRef)) {
                parse_error(rt, &n->location, _("super call cannot be used here"));
                r = RJS_FALSE;
            } else if ((flags & CONTAINS_FL_SUPER_PROP) && (n->type == RJS_AST_SuperPropRef)) {
                parse_error(rt, &n->location, _("super property cannot be used here"));
                r = RJS_FALSE;
            } else if ((flags & CONTAINS_FL_NEW_TARGET) && (n->type == RJS_AST_NewTargetRef)) {
                parse_error(rt, &n->location, _("\"new.target\" cannot be used here"));
                r = RJS_FALSE;
            } else if ((flags & CONTAINS_FL_ARGUMENTS) && (n->type == RJS_AST_ArgumentsRef)) {
                parse_error(rt, &n->location, _("\"arguments\" cannot be used here"));
                r = RJS_FALSE;
            } else if ((flags & CONTAINS_FL_AWAIT) && (n->type == RJS_AST_AwaitRef)) {
                parse_error(rt, &n->location, _("`await\' cannot be used here"));
                r = RJS_FALSE;
            }
        }
    }

    return r;
}

/*Push a no strict token list entry to the stack.*/
static RJS_AstNoStrictListStack*
no_strict_list_push (RJS_Runtime *rt)
{
    size_t                    top    = rjs_value_stack_save(rt);
    RJS_Value                *vt     = rjs_value_stack_push(rt);
    RJS_Parser               *parser = rt->parser;
    RJS_AstNoStrictListStack *ils;

    ils = ast_new(rt, vt, RJS_AST_NoStrictListStack, NULL);

    ils->bot = parser->no_strict_list_stack;
    parser->no_strict_list_stack = ils;

    rjs_value_stack_restore(rt, top);
    return ils;
}

/*Add an identifier to the no strict identifier list.*/
static void
no_strict_list_add (RJS_Runtime *rt, RJS_AstType type, RJS_Value *id)
{
    size_t                    top    = rjs_value_stack_save(rt);
    RJS_Value                *vt     = rjs_value_stack_push(rt);
    RJS_Parser               *parser = rt->parser;
    RJS_AstNoStrictListStack *ils    = parser->no_strict_list_stack;
    RJS_AstValue             *v;

    v = ast_new(rt, vt, type, NULL);

    rjs_value_copy(rt, &v->value, id);
    ast_list_append(rt, &ils->list, vt);

    rjs_value_stack_restore(rt, top);
}

/*Popup the top no strict token list entry.*/
static void
no_strict_list_pop (RJS_Runtime *rt, RJS_Bool is_func, RJS_Bool join)
{
    RJS_Parser               *parser = rt->parser;
    RJS_AstNoStrictListStack *ils    = parser->no_strict_list_stack;
    RJS_AstNoStrictListStack *bot    = ils->bot;
    RJS_Bool                  strict;

    if (!join) {
        if (is_func) {
            RJS_AstFunc *func = func_top(rt);

            strict = func->flags & RJS_AST_FUNC_FL_STRICT;
        } else {
            strict = RJS_TRUE;
        }

        /*Check the identifiers cannot be in strict mode.*/
        if (strict) {
            RJS_AstValue *v;

            rjs_list_foreach_c(&ils->list, v, RJS_AstValue, ast.ln) {
                if (v->ast.type == RJS_AST_IdRef) {
                    rjs_parse_error(rt, &v->ast.location,
                            _("\"%s\" cannot be used as binding identifier in strict mode"),
                            rjs_string_to_enc_chars(rt, &v->value, NULL, NULL));
                } else if (v->ast.type == RJS_AST_StrRef) {
                    rjs_parse_error(rt, &v->ast.location,
                            _("legacy escape character cannot be used in strict mode"));
                }
            }
        }
    } else if (bot) {
        /*Join the identifiers to the bottom list.*/
        rjs_list_join(&bot->list, &ils->list);
        rjs_list_init(&ils->list);
    }

    parser->no_strict_list_stack = bot;
}

/*Save the top no strict token list stack entry to the value.*/
static void
no_strict_list_save (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Parser               *parser = rt->parser;
    RJS_AstNoStrictListStack *ils    = parser->no_strict_list_stack;

    rjs_value_set_gc_thing(rt, v, ils);

    parser->no_strict_list_stack = ils->bot;
}

/*Restore the no strict token list stack entry to the stack top.*/
static void
no_strict_list_restore (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Parser               *parser = rt->parser;
    RJS_AstNoStrictListStack *bot    = parser->no_strict_list_stack;
    RJS_AstNoStrictListStack *ils    = rjs_value_get_gc_thing(rt, v);

    ils->bot = bot;
    parser->no_strict_list_stack = ils;
}

/*Push a function to the stack.*/
static RJS_AstFunc*
func_push (RJS_Runtime *rt, RJS_Location *loc)
{
    RJS_Parser  *parser = rt->parser;
    size_t       top    = rjs_value_stack_save(rt);
    RJS_Value   *vf     = rjs_value_stack_push(rt);
    RJS_AstFunc *func;

    func = ast_new(rt, vf, RJS_AST_Func, loc);

    func->flags          = 0;
    func->name           = NULL;
    func->binding_name   = NULL;
    func->id             = -1;
    func->param_len      = 0;
    func->param_decl     = NULL;
    func->var_decl       = NULL;
    func->lex_decl       = NULL;
    func->label_stack    = NULL;
    func->break_stack    = NULL;
    func->continue_stack = NULL;
    func->data           = NULL;
    func->block_stack    = NULL;
    func->prop_ref_start = 0;
    func->prop_ref_num   = 0;

    if (parser->flags & RJS_PARSE_FL_STRICT)
        func->flags |= RJS_AST_FUNC_FL_STRICT;

    func->bot = parser->func_stack;
    parser->func_stack = func;

    rjs_list_append(&parser->func_list, &func->ast.ln);

    rjs_value_stack_restore(rt, top);
    return func;
}

/*Get the identifier AST.*/
static RJS_AstId*
id_get (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Ast *ast = ast_get(rt, v);

    if (!ast)
        return NULL;

    while (ast->type == RJS_AST_ParenthesesExpr) {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        ast = ast_get(rt, &ue->operand);
    }

    if (ast->type == RJS_AST_Id)
        return (RJS_AstId*)ast;

    return NULL;
}

/*Get the value expression AST.*/
static RJS_AstValueExpr*
value_expr_get (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Ast *ast = ast_get(rt, v);

    while (ast->type == RJS_AST_ParenthesesExpr) {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        ast = ast_get(rt, &ue->operand);
    }

    if (ast->type == RJS_AST_ValueExpr)
        return (RJS_AstValueExpr*)ast;

    return NULL;
}

/*Set the function's name.*/
static void
func_set_name (RJS_Runtime *rt, RJS_Ast *ast, RJS_Location *loc, RJS_Value *name)
{
    while (ast->type == RJS_AST_ParenthesesExpr) {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        ast = ast_get(rt, &ue->operand);
    }

    if (ast->type == RJS_AST_FuncExpr) {
        RJS_AstFuncRef *fr = (RJS_AstFuncRef*)ast;

        if (!fr->func->name)
            fr->func->name = value_entry_add(rt, loc, name);
    } else if (ast->type == RJS_AST_ClassExpr) {
        RJS_AstClassRef *cr = (RJS_AstClassRef*)ast;

        if (cr->clazz->constructor) {
            RJS_AstFunc *constr = cr->clazz->constructor->func;

            if (constr && !constr->name)
                constr->name = value_entry_add(rt, loc, name);
        } else if (!cr->clazz->name) {
            cr->clazz->name = value_entry_add(rt, loc, name);
        }
    }
}

/*Check the binding in parameter.*/
static void
binding_param (RJS_Runtime *rt, RJS_Value *v, ParamAttr *pa)
{
    RJS_Ast     *ast  = ast_get(rt, v);
    RJS_AstFunc *func = func_top(rt);

    switch (ast->type) {
    case RJS_AST_Id: {
        RJS_AstId *id = (RJS_AstId*)ast;
        RJS_Bool   dup;

        if (rjs_string_equal(rt, &id->identifier->value, rjs_s_arguments(rt)))
            pa->has_args = RJS_TRUE;

        decl_item_add(rt, RJS_AST_DECL_PARAMETER, &id->ast.location, &id->identifier->value, &dup);
        if (dup)
            pa->has_dup = RJS_TRUE;

#if ENABLE_GENERATOR
        if (func->flags & RJS_AST_FUNC_FL_GENERATOR) {
            if (rjs_string_equal(rt, &id->identifier->value, rjs_s_yield(rt))) {
                parse_error(rt, &id->ast.location, _("`yield\' cannot be used in generator's parameters"));
            }
        }
#endif /*ENABLE_GENERATOR*/
        break;
    }
    case RJS_AST_ObjectBinding:
    case RJS_AST_ArrayBinding: {
        RJS_AstList *l = (RJS_AstList*)ast;

        pa->simple = RJS_FALSE;

        binding_element_list_param(rt, &l->list, pa);
        break;
    }
    default:
        assert(0);
    }
}

/*Check the binding element list in parameters.*/
static void
binding_element_list_param (RJS_Runtime *rt, RJS_List *l, ParamAttr *pa)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(l, ast, RJS_Ast, ln) {
        switch (ast->type) {
        case RJS_AST_BindingElem: {
            RJS_AstBindingElem *be = (RJS_AstBindingElem*)ast;

            if (!rjs_value_is_undefined(rt, &be->init)) {
                pa->has_expr = RJS_TRUE;
                pa->simple   = RJS_FALSE;
            }

            binding_param(rt, &be->binding, pa);
            break;
        }
        case RJS_AST_BindingProp: {
            RJS_AstBindingProp *bp = (RJS_AstBindingProp*)ast;
            RJS_Ast            *n  = ast_get(rt, &bp->name);

            if ((n->type != RJS_AST_Id) && (n->type != RJS_AST_ValueExpr))
                pa->has_expr = RJS_TRUE;

            if (!rjs_value_is_undefined(rt, &bp->init)) {
                pa->has_expr = RJS_TRUE;
                pa->simple   = RJS_FALSE;
            }

            binding_param(rt, &bp->binding, pa);
            break;
        }
        case RJS_AST_Rest: {
            RJS_AstRest *rest = (RJS_AstRest*)ast;

            pa->simple = RJS_FALSE;

            binding_param(rt, &rest->binding, pa);
            break;
        }
        case RJS_AST_Elision:
        case RJS_AST_LastElision:
            break;
        default:
            break;
        }
    }
}

/*Get the function's parameters' length.*/
static size_t
func_get_param_len (RJS_Runtime *rt, RJS_AstFunc *func)
{
    RJS_Ast *ast;
    size_t   len = 0;

    rjs_list_foreach_c(&func->param_list, ast, RJS_Ast, ln) {
        RJS_AstBindingElem *be;

        if (ast->type == RJS_AST_Rest)
            break;

        be = (RJS_AstBindingElem*)ast;
        if (ast_get(rt, &be->init))
            break;

        len ++;
    }

    return len;
}

/*Start the function body.*/
static void
func_body (RJS_Runtime *rt)
{
    ParamAttr    pa;
    RJS_Parser  *parser = rt->parser;
    RJS_AstFunc *func   = func_top(rt);

    if (parser->flags & RJS_PARSE_FL_STRICT)
        func->flags |= RJS_AST_FUNC_FL_STRICT;

    /*Push the parameter lexical declaration.*/
    func->param_decl = decl_push(rt);

    /*Check the parameters.*/
    pa.has_dup  = RJS_FALSE;
    pa.has_expr = RJS_FALSE;
    pa.has_args = RJS_FALSE;
    pa.simple   = RJS_TRUE;

    binding_element_list_param(rt, &func->param_list, &pa);

    func->param_len = func_get_param_len(rt, func);

    if (pa.has_dup && !pa.simple) {
        parse_error(rt, &func->ast.location, _("duplicated parameters cano only be in simple parameters list"));
    }

    if (pa.has_dup)
        func->flags |= RJS_AST_FUNC_FL_DUP_PARAM;
    if (pa.has_args)
        func->flags |= RJS_AST_FUNC_FL_ARGS_PARAM;
    if (pa.has_expr)
        func->flags |= RJS_AST_FUNC_FL_EXPR_PARAM;
    if (pa.simple)
        func->flags |= RJS_AST_FUNC_FL_SIMPLE_PARAM;

    /*Push the variable lexical declaration.*/
    if (func->flags & RJS_AST_FUNC_FL_EXPR_PARAM) {
        func->var_decl = decl_push(rt);
    } else {
        func->var_decl = func->param_decl;
    }

    /*Push the lexically declataion.*/
    if (!(func->flags & RJS_AST_FUNC_FL_STRICT)
            && !(func->flags & (RJS_AST_FUNC_FL_SCRIPT|RJS_AST_FUNC_FL_EVAL))) {
        func->lex_decl = decl_push(rt);
    } else {
        func->lex_decl = func->var_decl;
    }
}

/*Popup the top function in the stack.*/
static RJS_AstFunc*
func_pop (RJS_Runtime *rt)
{
    RJS_Parser  *parser    = rt->parser;
    RJS_AstFunc *func      = parser->func_stack;
    RJS_Bool     need_args = RJS_TRUE;
    size_t       top       = rjs_value_stack_save(rt);
    RJS_Value   *tmp       = rjs_value_stack_push(rt);

    assert(func);

    /*Check the function with "use strict" use the simple parameters list.*/
    if (func->flags & RJS_AST_FUNC_FL_USE_STRICT) {
        if (!(func->flags & RJS_AST_FUNC_FL_SIMPLE_PARAM)) {
            parse_error(rt, &func->ast.location,
                    _("function contains \"use strict\" must use simple parameters list"));
        }
    }

    /*Check the duplicated parameters.*/
    if (func->flags & RJS_AST_FUNC_FL_DUP_PARAM) {
#if ENABLE_ARROW_FUNC
        if (func->flags & RJS_AST_FUNC_FL_ARROW) {
            parse_error(rt, &func->ast.location,
                    _("arrow function cannot has duplicated parameters"));
        } else
#endif /*ENABLE_ARROW_FUNC*/
        if ((func->flags & RJS_AST_FUNC_FL_STRICT) || (func->flags & RJS_AST_FUNC_FL_METHOD)) {
            parse_error(rt, &func->ast.location,
                    _("strict mode function cannot has duplicated parameters"));
        }
    }

    /*Check if the "arguments" object is needed.*/
    if (func->flags & (
#if ENABLE_ARROW_FUNC
            RJS_AST_FUNC_FL_ARROW|
#endif /*ENABLE_ARROW_FUNC*/
            RJS_AST_FUNC_FL_SCRIPT|RJS_AST_FUNC_FL_EVAL|RJS_AST_FUNC_FL_MODULE)) {
        need_args = RJS_FALSE;
    } else if (func->flags & RJS_AST_FUNC_FL_ARGS_PARAM) {
        need_args = RJS_FALSE;
    } else if (!(func->flags & RJS_AST_FUNC_FL_EXPR_PARAM) && func->var_decl) {
        RJS_HashEntry *he;

        he = hash_lookup(rt, &func->var_decl->item_hash, rjs_s_arguments(rt), NULL);
        if (he) {
            RJS_AstDeclItem *di = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

            if ((di->type == RJS_AST_DECL_FUNCTION)
                    || (di->type == RJS_AST_DECL_LET)
                    || (di->type == RJS_AST_DECL_CONST)
                    || (di->type == RJS_AST_DECL_STRICT)
                    || (di->type == RJS_AST_DECL_CLASS))
                need_args = RJS_FALSE;
        }
    }

    if (need_args) {
        RJS_HashEntry *he, **phe;

        func->flags |= RJS_AST_FUNC_FL_NEED_ARGS;

        if ((func->flags & RJS_AST_FUNC_FL_STRICT)
                || !(func->flags & RJS_AST_FUNC_FL_SIMPLE_PARAM))
            func->flags |= RJS_AST_FUNC_FL_UNMAP_ARGS;

        if (func->var_decl) {
            /*Remove the "arguments" variable. It is already initialized.*/
            he = hash_lookup(rt, &func->var_decl->item_hash, rjs_s_arguments(rt), &phe);
            if (he) {
                RJS_AstDeclItem *di = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

                if (di->type == RJS_AST_DECL_VAR) {
                    rjs_hash_remove(&func->var_decl->item_hash, phe, rt);
                    rjs_list_remove(&di->ast.ln);
                }
            }
        }
    }

    /*Check if the parameters is defined in lexically declaration.*/
    if (func->param_decl != func->lex_decl) {
        RJS_AstDeclItem *di;

        rjs_list_foreach_c(&func->param_decl->item_list, di, RJS_AstDeclItem, ast.ln) {
            RJS_HashEntry *he;

            rjs_value_set_string(rt, tmp, di->he.key);
            he = hash_lookup(rt, &func->lex_decl->item_hash, tmp, NULL);
            if (he) {
                RJS_AstDeclItem *lex_di = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

                if ((lex_di->type == RJS_AST_DECL_LET)
                        || (lex_di->type == RJS_AST_DECL_CONST)
                        || (lex_di->type == RJS_AST_DECL_STRICT)
                        || (lex_di->type == RJS_AST_DECL_CLASS)) {
                    parse_error(rt, &di->ast.location, _("\"%s\" is already defined"),
                            rjs_string_to_enc_chars(rt, tmp, NULL, NULL));
                    parse_prev_define_note(rt, &lex_di->ast.location);
                }
            }
        }
    }

    if (parser->decl_stack == func->lex_decl)
        decl_pop(rt);

    if (parser->decl_stack == func->var_decl)
        decl_pop(rt);

    if (parser->decl_stack == func->param_decl)
        decl_pop(rt);

    parser->func_stack = func->bot;

    rjs_value_stack_restore(rt, top);
    return func;
}

/*Push a jump stack entry to break stack.*/
static void
break_push (RJS_Runtime *rt, RJS_AstJumpStack *js, RJS_Ast *stmt)
{
    RJS_AstFunc *func = func_top(rt);

    js->stmt = stmt;
    js->bot  = func->break_stack;
    func->break_stack = js;
}

/*Popup a jump stack entry from break stack.*/
static void
break_pop (RJS_Runtime *rt)
{
    RJS_AstFunc *func = func_top(rt);
    RJS_AstJumpStack *js   = func->break_stack;

    assert(js);

    func->break_stack = js->bot;
}

/*Push a jump stack entry to continue stack.*/
static void
continue_push (RJS_Runtime *rt, RJS_AstJumpStack *js, RJS_Ast *stmt)
{
    RJS_AstFunc *func = func_top(rt);

    js->stmt = stmt;
    js->bot  = func->continue_stack;
    func->continue_stack = js;
}

/*Popup a jump stack entry from continue stack.*/
static void
continue_pop (RJS_Runtime *rt)
{
    RJS_AstFunc      *func = func_top(rt);
    RJS_AstJumpStack *js   = func->continue_stack;

    assert(js);

    func->continue_stack = js->bot;
}

/*Initialize the parser.*/
static void
parser_init (RJS_Runtime *rt, RJS_Parser *parser, RJS_Input *input)
{
    parser->flags       = 0;
    parser->status      = 0;
    parser->last_line   = 1;
    parser->func_stack  = NULL;
    parser->class_stack = NULL;
    parser->decl_stack  = NULL;
    parser->no_strict_list_stack = NULL;
    parser->contains_list_stack  = NULL;
    parser->func_num    = 0;
    parser->decl_num    = 0;
    parser->value_entry_num   = 0;
    parser->binding_table_num = 0;
    parser->func_table_num    = 0;
    parser->prop_ref_num      = 0;

#if ENABLE_PRIV_NAME
    parser->bot_priv_env   = NULL;
    parser->priv_env_stack = NULL;
    parser->priv_env_num   = 0;
    parser->priv_id_num    = 0;

    rjs_list_init(&parser->priv_env_list);
    rjs_list_init(&parser->priv_id_ref_list);
#endif /*ENABLE_PRIV_NAME*/

    rjs_lex_init(rt, &parser->lex, input);
    rjs_token_init(rt, &parser->curr_token);
    rjs_token_init(rt, &parser->next_token);
    rjs_list_init(&parser->func_list);
    rjs_list_init(&parser->class_list);
    rjs_list_init(&parser->decl_list);
    rjs_list_init(&parser->value_entry_list);
    rjs_list_init(&parser->binding_table_list);
    rjs_list_init(&parser->func_table_list);
    rjs_list_init(&parser->prop_ref_list);
    rjs_hash_init(&parser->value_entry_hash);

#if ENABLE_MODULE
    parser->import_num  = 0;
    parser->local_export_num = 0;
    parser->indir_export_num = 0;
    parser->star_export_num  = 0;
    rjs_list_init(&parser->module_request_list);
    rjs_list_init(&parser->import_list);
    rjs_list_init(&parser->local_export_list);
    rjs_list_init(&parser->indir_export_list);
    rjs_list_init(&parser->star_export_list);
    hash_init(&parser->module_request_hash);
    hash_init(&parser->export_hash);
#endif /*ENABLE_MODULE*/

    rt->parser = parser;
}

/*Releaase the parser.*/
static void
parser_deinit (RJS_Runtime *rt)
{
    RJS_Parser *parser = rt->parser;

    rjs_token_deinit(rt, &parser->curr_token);
    rjs_token_deinit(rt, &parser->next_token);

    rjs_lex_deinit(rt, &parser->lex);
    rjs_hash_deinit(&parser->value_entry_hash, &rjs_hash_value_ops, rt);

#if ENABLE_MODULE
    hash_deinit(rt, &parser->module_request_hash);
    hash_deinit(rt, &parser->export_hash);
#endif /*ENABLE_MODULE*/

    rt->parser = NULL;
}

/*Check if the parser has error.*/
static RJS_Bool
parser_has_error (RJS_Runtime *rt)
{
    RJS_Parser *parser = rt->parser;

    return (parser->status & RJS_PARSE_ST_ERROR)
            || rjs_lex_error(&parser->lex)
            || rjs_input_error(parser->lex.input);
}

/*The current token.*/
static RJS_Token*
curr_token (RJS_Runtime *rt)
{
    RJS_Parser *parser = rt->parser;

    return &parser->curr_token;
}

/*Get a token from the input.*/
static RJS_Token*
get_token_flags (RJS_Runtime *rt, int flags)
{
    RJS_Parser *parser = rt->parser;
    int         old_flags;

    if (parser->status & RJS_PARSE_ST_CURR_TOKEN) {
        parser->status &= ~RJS_PARSE_ST_CURR_TOKEN;
        return &parser->curr_token;
    }

    parser->last_line = parser->curr_token.location.last_line;

    if (parser->status & RJS_PARSE_ST_NEXT_TOKEN) {
        parser->curr_token.type     = parser->next_token.type;
        parser->curr_token.flags    = parser->next_token.flags;
        parser->curr_token.location = parser->next_token.location;
        rjs_value_copy(rt, parser->curr_token.value, parser->next_token.value);

        parser->status &= ~RJS_PARSE_ST_NEXT_TOKEN;
    } else {
        old_flags = parser->lex.flags;
        parser->lex.flags |= flags;
        rjs_lex_get_token(rt, &parser->lex, &parser->curr_token);
        parser->lex.flags = old_flags;
    }

    return &parser->curr_token;
}

/*Push back the current token.*/
static void
unget_token (RJS_Runtime *rt)
{
    RJS_Parser *parser = rt->parser;

    assert(!(parser->status & RJS_PARSE_ST_CURR_TOKEN));

    parser->status |= RJS_PARSE_ST_CURR_TOKEN;
}

/*Get the next token from the input.*/
static RJS_Token*
next_token_flags (RJS_Runtime *rt, int flags)
{
    RJS_Parser *parser = rt->parser;
    int         old_flags;

    if (parser->status & RJS_PARSE_ST_NEXT_TOKEN)
        return &parser->next_token;

    old_flags = parser->lex.flags;
    parser->lex.flags |= flags;
    rjs_lex_get_token(rt, &parser->lex, &parser->next_token);
    parser->lex.flags = old_flags;

    parser->status |= RJS_PARSE_ST_NEXT_TOKEN;

    return &parser->next_token;
}

/*Get a token from the input.*/
static RJS_Token*
get_token (RJS_Runtime *rt)
{
    return get_token_flags(rt, 0);
}

/*Get a token from the input. And "/" or "/=" can be here.*/
static RJS_Token*
get_token_div (RJS_Runtime *rt)
{
    return get_token_flags(rt, RJS_LEX_FL_DIV);
}

/*Get the next token from the input.*/
static RJS_Token*
next_token (RJS_Runtime *rt)
{
    return next_token_flags(rt, 0);
}

/*Get the next token from the input. And "/" or "/=" can be here.*/
static RJS_Token*
next_token_div (RJS_Runtime *rt)
{
    return next_token_flags(rt, RJS_LEX_FL_DIV);
}

/*Check if the token is an identifier.*/
static RJS_Bool
is_identifier (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_Parser *parser = rt->parser;

    if (type != RJS_TOKEN_IDENTIFIER)
        return RJS_FALSE;

    if (flags & RJS_TOKEN_FL_RESERVED)
        return RJS_FALSE;

    if (parser->flags & RJS_PARSE_FL_STRICT) {
        if (flags & RJS_TOKEN_FL_STRICT_RESERVED)
            return RJS_FALSE;
    }

    return RJS_TRUE;
}

/*Check if the token is a binding identifier.*/
static RJS_Bool
is_binding_identifier (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_IdentifierType itype;

    if (type != RJS_TOKEN_IDENTIFIER)
        return RJS_FALSE;

    if (is_identifier(rt, type, flags))
        return RJS_TRUE;

    itype = flags & RJS_TOKEN_IDENTIFIER_MASK;

    if ((itype == RJS_IDENTIFIER_yield) || (itype == RJS_IDENTIFIER_await))
        return RJS_TRUE;

    return RJS_FALSE;
}

/*Checl if the token is an identifier reference.*/
static RJS_Bool
is_identifier_reference (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_Parser        *parser = rt->parser;
    RJS_IdentifierType itype;

    if (type != RJS_TOKEN_IDENTIFIER)
        return RJS_FALSE;

    if (is_identifier(rt, type, flags))
        return RJS_TRUE;

    itype = flags & RJS_TOKEN_IDENTIFIER_MASK;

    if (!(parser->flags & RJS_PARSE_FL_YIELD)) {
        if (itype == RJS_IDENTIFIER_yield)
            return RJS_TRUE;
    }

    if (!(parser->flags & RJS_PARSE_FL_AWAIT)) {
        if (itype == RJS_IDENTIFIER_await)
            return RJS_TRUE;
    }

    return RJS_FALSE;
}

/*Check if the token is the identifier.*/
static RJS_Bool
token_is_identifier (RJS_TokenType type, int flags, RJS_IdentifierType itype)
{
    if (type != RJS_TOKEN_IDENTIFIER)
        return RJS_FALSE;

    if (!(flags
            & (RJS_TOKEN_FL_RESERVED
            |RJS_TOKEN_FL_STRICT_RESERVED
            |RJS_TOKEN_FL_KNOWN_IDENTIFIER)))
        return RJS_FALSE;

    if ((flags & RJS_TOKEN_IDENTIFIER_MASK) != itype)
        return RJS_FALSE;

    if (flags & RJS_TOKEN_FL_ESCAPE)
        return RJS_FALSE;

    return RJS_TRUE;
}

#if ENABLE_ASYNC

/*Check if the token is an async function header.*/
static RJS_Bool
is_async_function (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_Token *tok = curr_token(rt);
    RJS_Token *ntok;

    if (!token_is_identifier(type, flags, RJS_IDENTIFIER_async))
        return RJS_FALSE;

    ntok = next_token_div(rt);

    if ((ntok->location.first_line == tok->location.last_line)
            && token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_function))
        return RJS_TRUE;

    return RJS_FALSE;
}

#if ENABLE_ARROW_FUNC

/*Check if the token is an async arrow function header.*/
static RJS_Bool
is_async_arrow (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_Token *tok = curr_token(rt);
    RJS_Token *ntok;

    if (!token_is_identifier(type, flags, RJS_IDENTIFIER_async))
        return RJS_FALSE;

    ntok = next_token_div(rt);

    if ((ntok->location.first_line == tok->location.last_line)
            && is_binding_identifier(rt, ntok->type, ntok->flags))
        return RJS_TRUE;

    return RJS_FALSE;
}

#endif /*ENABLE_ARROW_FUNC*/

/*Check if the token is an async method header.*/
static RJS_Bool
is_async_method (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_Token *tok  = curr_token(rt);
    RJS_Token *ntok;

    if (!token_is_identifier(type, flags, RJS_IDENTIFIER_async))
        return RJS_FALSE;

    ntok = next_token_div(rt);
    if (ntok->location.first_line != tok->location.last_line)
        return RJS_FALSE;

    switch (ntok->type) {
    case RJS_TOKEN_star:
    case RJS_TOKEN_STRING:
    case RJS_TOKEN_NUMBER:
    case RJS_TOKEN_lbracket:
    case RJS_TOKEN_PRIVATE_IDENTIFIER:
    case RJS_TOKEN_IDENTIFIER:
        return RJS_TRUE;
    default:
        break;
    }

    return RJS_FALSE;
}

#endif /*ENABLE_ASYNC*/

/*Check if the token is an accessor method header.*/
static RJS_Bool
is_accessor_method (RJS_Runtime *rt, RJS_TokenType type, int flags)
{
    RJS_Token *ntok;

    if (!token_is_identifier(type, flags, RJS_IDENTIFIER_get)
            && !token_is_identifier(type, flags, RJS_IDENTIFIER_set))
        return RJS_FALSE;

    ntok = next_token_div(rt);

    switch (ntok->type) {
    case RJS_TOKEN_STRING:
    case RJS_TOKEN_NUMBER:
    case RJS_TOKEN_lbracket:
    case RJS_TOKEN_PRIVATE_IDENTIFIER:
    case RJS_TOKEN_IDENTIFIER:
        return RJS_TRUE;
    default:
        break;
    }

    return RJS_FALSE;
}

/*Output parse message.*/
static void
parse_message_v (RJS_Runtime *rt, RJS_MessageType type, RJS_Location *loc,
        const char *fmt, va_list ap)
{
    RJS_Parser *parser = rt->parser;

    if (type == RJS_MESSAGE_ERROR)
        parser->status |= RJS_PARSE_ST_ERROR;

    rjs_message_v(rt, parser->lex.input, type, loc, fmt, ap);
}

/*Output parse error message.*/
static void
parse_error (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    parse_message_v(rt, RJS_MESSAGE_ERROR, loc, fmt, ap);
    va_end(ap);
}

/*Output parse warning message.*/
static void
parse_warning (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    parse_message_v(rt, RJS_MESSAGE_WARNING, loc, fmt, ap);
    va_end(ap);
}

/*Output parse note message.*/
static void
parse_note (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    parse_message_v(rt, RJS_MESSAGE_NOTE, loc, fmt, ap);
    va_end(ap);
}

/**
 * Output parse error message.
 * \param rt The current runtime.
 * \param loc Location where generate the message.
 * \param fmt Output format.
 * \param ... Arguments.
 */
void
rjs_parse_error (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    parse_message_v(rt, RJS_MESSAGE_ERROR, loc, fmt, ap);
    va_end(ap);
}

/**
 * Output parse warning message.
 * \param rt The current runtime.
 * \param loc Location where generate the message.
 * \param fmt Output format.
 * \param ... Arguments.
 */
void
rjs_parse_warning (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    parse_message_v(rt, RJS_MESSAGE_WARNING, loc, fmt, ap);
    va_end(ap);
}

/**
 * Output parse note message.
 * \param rt The current runtime.
 * \param loc Location where generate the message.
 * \param fmt Output format.
 * \param ... Arguments.
 */
void
rjs_parse_note (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    parse_message_v(rt, RJS_MESSAGE_NOTE, loc, fmt, ap);
    va_end(ap);
}

/*Output unexpected token error message.*/
static void
parse_unexpect_error (RJS_Runtime *rt, RJS_Location *loc, const char *name)
{
    RJS_Token *tok = curr_token(rt);

    parse_error(rt, loc, _("unexpected %s, expect %s"),
            rjs_token_type_get_name(tok->type, tok->flags), name);
}

/*Output unexpected token error message.*/
static void
parse_unexpect_token_error (RJS_Runtime *rt, RJS_Location *loc, RJS_TokenType type, int flags)
{
    RJS_Token *tok = curr_token(rt);

    parse_error(rt, loc, _("unexpected %s, expect %s"),
            rjs_token_type_get_name(tok->type, tok->flags),
            rjs_token_type_get_name(type, flags));
}

/*Output previous definition note message.*/
static void
parse_prev_define_note (RJS_Runtime *rt, RJS_Location *loc)
{
    parse_note(rt, loc, _("the previous definition is here"));
}

/*Check the binding identifier.*/
static RJS_Bool
check_binding_identifier (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *id)
{
    RJS_Parser *parser = rt->parser;

    if (rjs_string_equal(rt, id, rjs_s_arguments(rt))
                || rjs_string_equal(rt, id, rjs_s_eval(rt))
                || rjs_string_equal(rt, id, rjs_s_yield(rt)))
        no_strict_list_add(rt, RJS_AST_IdRef, id);

    if (parser->flags & RJS_PARSE_FL_MODULE) {
        if (rjs_string_equal(rt, id, rjs_s_await(rt))) {
            parse_error(rt, loc, _("`await\' cannot be used as binding identifier in module"));
            return RJS_FALSE;
        }
    }

    if (parser->flags & RJS_PARSE_FL_YIELD) {
        if (rjs_string_equal(rt, id, rjs_s_yield(rt))) {
            parse_error(rt, loc, _("`yield\' cannot be used as binding identifier here"));
            return RJS_FALSE;
        }
    }

    if (parser->flags & RJS_PARSE_FL_AWAIT) {
        if (rjs_string_equal(rt, id, rjs_s_await(rt))) {
            parse_error(rt, loc, _("`await\' cannot be used as binding identifier here"));
            return RJS_FALSE;
        }
    } else {
        if (rjs_string_equal(rt, id, rjs_s_await(rt)))
            contains_list_add(rt, RJS_AST_AwaitRef, loc);
    }

    return RJS_TRUE;
}

/*Check the identifier reference.*/
static RJS_Bool
check_identifier_reference (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *id)
{
    RJS_Parser *parser = rt->parser;

    if (parser->flags & RJS_PARSE_FL_STRICT) {
        if (rjs_string_equal(rt, id, rjs_s_yield(rt))) {
            parse_error(rt, loc, _("`yield\' cannot be used as identifier reference in strict mode"));
            return RJS_FALSE;
        }
    }

    if (parser->flags & RJS_PARSE_FL_MODULE) {
        if (rjs_string_equal(rt, id, rjs_s_await(rt))) {
            parse_error(rt, loc, _("`await\' cannot be used as identifier reference in module"));
            return RJS_FALSE;
        }
    }

    if (parser->flags & RJS_PARSE_FL_YIELD) {
        if (rjs_string_equal(rt, id, rjs_s_yield(rt))) {
            parse_error(rt, loc, _("`yield\' cannot be used as identifier reference here"));
            return RJS_FALSE;
        }
    }

    if (parser->flags & RJS_PARSE_FL_AWAIT) {
        if (rjs_string_equal(rt, id, rjs_s_await(rt))) {
            parse_error(rt, loc, _("`await\' cannot be used as identifier reference here"));
            return RJS_FALSE;
        }
    } else {
        if (rjs_string_equal(rt, id, rjs_s_await(rt)))
            contains_list_add(rt, RJS_AST_AwaitRef, loc);
    }

    if (rjs_string_equal(rt, id, rjs_s_arguments(rt))) {
        contains_list_add(rt, RJS_AST_ArgumentsRef, loc);
    }

    return RJS_TRUE;
}

/*Check the delete expression's operand.*/
static RJS_Bool
check_delete_operand_internal (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Parser *parser = rt->parser;
    RJS_Ast    *ast    = ast_get(rt, v);
    RJS_Bool    r;

    if (parser->flags & RJS_PARSE_FL_STRICT) {
        switch (ast->type) {
        case RJS_AST_Id:
        case RJS_AST_PrivMemberExpr:
        case RJS_AST_OptionalExpr:
            r = RJS_FALSE;
            break;
        case RJS_AST_ParenthesesExpr: {
            RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

            r = check_delete_operand_internal(rt, &ue->operand);
            break;
        }
        default:
            r = RJS_TRUE;
            break;
        }
    } else {
        r = RJS_TRUE;
    }

    return r;
}

/*Check the delete expression's operand.*/
static RJS_Bool
check_delete_operand (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Bool r;

    r = check_delete_operand_internal(rt, v);

    if (!r) {
        RJS_Ast *ast = ast_get(rt, v);

        parse_error(rt, &ast->location,
                _("the data cannot be deleted in strict mode"));
    }

    return r;
}

/*Check if the expression is an simple assignment target.*/
static RJS_Bool
check_simple_assi_target_internal (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Parser *parser = rt->parser;
    RJS_Ast    *ast    = ast_get(rt, v);
    RJS_Bool    r;

    switch (ast->type) {
    case RJS_AST_Id: {
        RJS_AstId *ir = (RJS_AstId*)ast;

        if ((parser->flags & RJS_PARSE_FL_STRICT)
                && (rjs_string_equal(rt, &ir->identifier->value, rjs_s_arguments(rt))
                || rjs_string_equal(rt, &ir->identifier->value, rjs_s_eval(rt))))
            r = RJS_FALSE;
        else
            r = RJS_TRUE;
        break;
    }
    case RJS_AST_MemberExpr:
    case RJS_AST_SuperMemberExpr:
    case RJS_AST_PrivMemberExpr:
        r = RJS_TRUE;
        break;
    case RJS_AST_ParenthesesExpr: {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        r = check_simple_assi_target_internal(rt, &ue->operand);
        break;
    }
    default:
        r = RJS_FALSE;
        break;
    }

    return r;
}

/*Check if the expression is an simple assignment target.*/
static RJS_Bool
check_simple_assi_target (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Bool r;

    r = check_simple_assi_target_internal(rt, v);

    if (!r) {
        RJS_Ast *ast = ast_get(rt, v);

        parse_error(rt, &ast->location,
                _("expression is not a simple assignment target"));
    }

    return r;
}

/*Check if the expression is valid.*/
static RJS_Bool
check_expr_ast (RJS_Runtime *rt, RJS_Ast *ast)
{
    RJS_Bool r = RJS_TRUE;

    if (!ast)
        return RJS_TRUE;

    switch (ast->type) {
    case RJS_AST_Array: {
        RJS_AstList *a = (RJS_AstList*)ast;
        RJS_Ast     *e;

        rjs_list_foreach_c(&a->list, e, RJS_Ast, ln) {
            r &= check_expr_ast(rt, e);
        }
        break;
    }
    case RJS_AST_Object: {
        RJS_AstList *o = (RJS_AstList*)ast;
        RJS_Ast     *p;

        rjs_list_foreach_c(&o->list, p, RJS_Ast, ln) {
            if ((p->type == RJS_AST_Prop) || (p->type == RJS_AST_SetProto)) {
                RJS_AstProp *prop = (RJS_AstProp*)p;

                if (!rjs_value_is_undefined(rt, &prop->init)) {
                    parse_error(rt, &p->location, _("illegal object property"));
                    r = RJS_FALSE;
                } else {
                    r &= check_expr(rt, &prop->name);
                    r &= check_expr(rt, &prop->value);
                    r &= check_expr(rt, &prop->init);
                }
            } else if (p->type == RJS_AST_ClassElem) {
                RJS_AstClassElem *ce = (RJS_AstClassElem*)p;

                r &= check_expr(rt, &ce->name);
            } else {
                r &= check_expr_ast(rt, p);
            }
        }
        break;
    }
    case RJS_AST_SpreadExpr:
    case RJS_AST_NotExpr:
    case RJS_AST_RevExpr:
    case RJS_AST_NegExpr:
    case RJS_AST_ToNumExpr:
    case RJS_AST_DelExpr:
    case RJS_AST_TypeOfExpr:
    case RJS_AST_VoidExpr:
    case RJS_AST_YieldExpr:
    case RJS_AST_YieldStarExpr:
    case RJS_AST_ParenthesesExpr:
    case RJS_AST_ImportExpr:
    case RJS_AST_AwaitExpr:
    case RJS_AST_OptionalExpr:
    case RJS_AST_OptionalBase: {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        r &= check_expr(rt, &ue->operand);
        break;
    }
    case RJS_AST_MemberExpr:
    case RJS_AST_PrivMemberExpr:
    case RJS_AST_SuperMemberExpr:
    case RJS_AST_AddExpr:
    case RJS_AST_SubExpr:
    case RJS_AST_MulExpr:
    case RJS_AST_DivExpr:
    case RJS_AST_ModExpr:
    case RJS_AST_ExpExpr:
    case RJS_AST_ShlExpr:
    case RJS_AST_ShrExpr:
    case RJS_AST_UShrExpr:
    case RJS_AST_LtExpr:
    case RJS_AST_GtExpr:
    case RJS_AST_LeExpr:
    case RJS_AST_GeExpr:
    case RJS_AST_EqExpr:
    case RJS_AST_NeExpr:
    case RJS_AST_StrictEqExpr:
    case RJS_AST_StrictNeExpr:
    case RJS_AST_InExpr:
    case RJS_AST_InstanceOfExpr:
    case RJS_AST_BitAndExpr:
    case RJS_AST_BitXorExpr:
    case RJS_AST_BitOrExpr:
    case RJS_AST_AndExpr:
    case RJS_AST_OrExpr:
    case RJS_AST_QuesExpr: {
        RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)ast;

        r &= check_expr(rt, &be->operand1);
        r &= check_expr(rt, &be->operand2); 
        break;
    }
    case RJS_AST_AssiExpr:
    case RJS_AST_AddAssiExpr:
    case RJS_AST_SubAssiExpr:
    case RJS_AST_MulAssiExpr:
    case RJS_AST_DivAssiExpr:
    case RJS_AST_ModAssiExpr:
    case RJS_AST_ExpAssiExpr:
    case RJS_AST_ShlAssiExpr:
    case RJS_AST_ShrAssiExpr:
    case RJS_AST_UShrAssiExpr:
    case RJS_AST_BitAndAssiExpr:
    case RJS_AST_BitXorAssiExpr:
    case RJS_AST_BitOrAssiExpr:
    case RJS_AST_AndAssiExpr:
    case RJS_AST_OrAssiExpr:
    case RJS_AST_QuesAssiExpr: {
        RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)ast;

        r &= check_expr(rt, &be->operand2);
        break;
    }
    case RJS_AST_CondExpr: {
        RJS_AstCondExpr *ce = (RJS_AstCondExpr*)ast;

        r &= check_expr(rt, &ce->cond);
        r &= check_expr(rt, &ce->true_value);
        r &= check_expr(rt, &ce->false_value);
        break;
    }
    case RJS_AST_CallExpr:
    case RJS_AST_SuperCallExpr:
    case RJS_AST_NewExpr: {
        RJS_AstCall *ce = (RJS_AstCall*)ast;
        RJS_Ast     *arg;

        r &= check_expr(rt, &ce->func);

        rjs_list_foreach_c(&ce->arg_list, arg, RJS_Ast, ln) {
            r &= check_expr_ast(rt, arg);
        }
        break;
    }
    case RJS_AST_CommaExpr: {
        RJS_AstList *l = (RJS_AstList*)ast;
        RJS_Ast     *item;

        rjs_list_foreach_c(&l->list, item, RJS_Ast, ln) {
            r &= check_expr_ast(rt, item);
        }
        break;
    }
    default:
        r = RJS_TRUE;
        break;
    }

    return r;
}

/*Check if the expression is valid.*/
static RJS_Bool
check_expr (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Ast *ast = ast_get(rt, v);

    return check_expr_ast(rt, ast);
}

/*Check the class element.*/
static RJS_Bool
check_class_element (RJS_Runtime *rt, RJS_Bool is_static, RJS_AstClassElem *ce)
{
    RJS_Bool  r         = RJS_TRUE;
    RJS_Ast  *ast       = ast_get(rt, &ce->name);
    RJS_Bool  is_constr = RJS_FALSE;
    RJS_Bool  is_proto  = RJS_FALSE;

    if (ce->computed)
        return r;

    if (ast->type == RJS_AST_ValueExpr) {
        RJS_AstValueExpr *ve = (RJS_AstValueExpr*)ast;

        if (rjs_value_is_string(rt, &ve->ve->value)) {
            if (rjs_string_equal(rt, &ve->ve->value, rjs_s_constructor(rt))) {
                is_constr = RJS_TRUE;
            } else if (is_static && rjs_string_equal(rt, &ve->ve->value, rjs_s_prototype(rt))) {
                is_proto = RJS_TRUE;
            }
        }
    }

    if (ce->type == RJS_AST_CLASS_ELEM_FIELD) {
        if (is_constr) {
            parse_error(rt, &ast->location, _("\"constructor\" cannot be used as field name"));
            r = RJS_FALSE;
        } else if (is_static && is_proto) {
            parse_error(rt, &ast->location, _("\"prototype\" cannot be used as static field name"));
            r = RJS_FALSE;
        }
    } else if ((ce->type == RJS_AST_CLASS_ELEM_GET) || (ce->type == RJS_AST_CLASS_ELEM_SET)) {
        if (!is_static && is_constr) {
            parse_error(rt, &ast->location, _("\"constructor\" cannot be used as special method"));
            r = RJS_FALSE;
        } else if (is_static && is_proto) {
            parse_error(rt, &ast->location, _("\"prototype\" cannot be used as static method"));
            r = RJS_FALSE;
        }
    } else if (ce->type == RJS_AST_CLASS_ELEM_METHOD) {
        int fflags = 0;

#if ENABLE_GENERATOR
        fflags |= RJS_AST_FUNC_FL_GENERATOR;
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
        fflags |= RJS_AST_FUNC_FL_ASYNC;
#endif /*ENABLE_ASYNC*/

        if (!is_static && is_constr && (ce->func->flags & fflags)) {
            parse_error(rt, &ast->location, _("\"constructor\" cannot be used as special method"));
            r = RJS_FALSE;
        } else if (is_static && is_proto) {
            parse_error(rt, &ast->location, _("\"prototype\" cannot be used as static method"));
            r = RJS_FALSE;
        }
    }

    return r;
}

/*Check if the expression can be converted to left hand expression.*/
static RJS_Bool
check_lh_expr (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Bool     r        = RJS_TRUE;
    RJS_Ast     *ast      = ast_get(rt, v);
    size_t       top      = rjs_value_stack_save(rt);
    RJS_Value   *tmp      = rjs_value_stack_push(rt);
    RJS_Bool     has_rest = RJS_FALSE;
    RJS_AstList *al;
    RJS_Ast     *e;

    switch (ast->type) {
    case RJS_AST_Array:
        al = (RJS_AstList*)ast;
        rjs_list_foreach_c(&al->list, e, RJS_Ast, ln) {
            if (has_rest)
                parse_error(rt, &e->location, _("array element cannot follow a rest array element"));

            switch (e->type) {
            case RJS_AST_Elision:
            case RJS_AST_LastElision:
                break;
            case RJS_AST_SpreadExpr: {
                RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)e;

                if (!check_lh_expr(rt, &ue->operand))
                    r = RJS_FALSE;

                has_rest = RJS_TRUE;
                break;
            }
            case RJS_AST_AssiExpr: {
                RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)e;

                if (!check_lh_expr(rt, &be->operand1))
                    r = RJS_FALSE;
                break;
            }
            default:
                rjs_value_set_gc_thing(rt, tmp, e);
                if (!check_lh_expr(rt, tmp))
                    r = RJS_FALSE;
                break;
            }
        }
        break;
    case RJS_AST_Object:
        al = (RJS_AstList*)ast;
        rjs_list_foreach_c(&al->list, e, RJS_Ast, ln) {
            if (has_rest)
                parse_error(rt, &e->location, _("array element cannot follow a rest array element"));

            switch (e->type) {
            case RJS_AST_SpreadExpr: {
                RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)e;

                if (!check_lh_expr(rt, &ue->operand))
                    r = RJS_FALSE;

                has_rest = RJS_TRUE;
                break;
            }
            case RJS_AST_Prop:
            case RJS_AST_SetProto: {
                RJS_AstProp *prop = (RJS_AstProp*)e;
                RJS_Ast     *vast = ast_get(rt, &prop->value);

                if (!vast) {
                    RJS_AstValueExpr *ve = ast_get(rt, &prop->name);

                    vast = (RJS_Ast*)id_new(rt, &prop->value, &ve->ast.location, &ve->ve->value);
                }

                if (vast->type == RJS_AST_AssiExpr) {
                    RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)vast;

                    if (!check_lh_expr(rt, &be->operand1))
                        r = RJS_FALSE;
                } else {
                    if (!check_lh_expr(rt, &prop->value))
                        r = RJS_FALSE;
                }
                break;
            }
            default:
                break;
            }
        }
        break;
    case RJS_AST_ObjectBinding:
    case RJS_AST_ArrayBinding:
    case RJS_AST_ParenthesesExpr:
        break;
    default:
        r = check_simple_assi_target(rt, v);
        break;
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_PRIV_NAME

/*Lookup the private identifier.*/
static RJS_Bool
priv_id_lookup (RJS_Runtime *rt, RJS_AstPrivEnv *stack, RJS_Value *id)
{
    RJS_Parser     *parser = rt->parser;
    RJS_AstPrivEnv *env;
    RJS_Result      r;

    for (env = stack; env; env = env->bot) {
        RJS_HashEntry *he;

        he = hash_lookup(rt, &env->priv_id_hash, id, NULL);
        if (he)
            return RJS_TRUE;
    }

    if (parser->bot_priv_env) {
        r = rjs_private_name_lookup(rt, id, parser->bot_priv_env, NULL);
        if (r)
            return r;
    }

    return RJS_FALSE;
}

/*Check the private environment list.*/
static RJS_Bool
check_priv_id_ref_list (RJS_Runtime *rt, RJS_AstPrivEnv *env, RJS_List *list)
{
    RJS_AstPrivIdRef *ref;
    RJS_Bool          b   = RJS_TRUE;

    rjs_list_foreach_c(list, ref, RJS_AstPrivIdRef, ast.ln) {
        if (!priv_id_lookup(rt, env, &ref->identifier)) {
            parse_error(rt, &ref->ast.location, _("private identifier \"%s\" is not defined"),
                    rjs_string_to_enc_chars(rt, &ref->identifier, NULL, NULL));
            b = RJS_FALSE;
        } else {
            prop_ref_new(rt, &ref->prop_ref, &ref->ast.location, ref->func, &ref->identifier);
        }
    }

    return b;
}

/*Check the private identifiers.*/
static RJS_Bool
check_priv_ids (RJS_Runtime *rt)
{
    RJS_AstClass *c;
    RJS_Parser   *parser = rt->parser;
    RJS_Bool      r      = RJS_TRUE;

    rjs_list_foreach_c(&parser->class_list, c, RJS_AstClass, ast.ln) {
        RJS_AstPrivEnv *env = c->priv_env;

        if (!check_priv_id_ref_list(rt, env, &c->priv_id_ref_list))
            r = RJS_FALSE;
    }

    if (parser->bot_priv_env) {
        if (!check_priv_id_ref_list(rt, NULL, &parser->priv_id_ref_list))
            r = RJS_FALSE;
    }

    return r;
}

#endif /*ENABLE_PRIV_NAME*/

/*Get the expected token.*/
static RJS_Result
get_token_expect (RJS_Runtime *rt, RJS_TokenType type)
{
    RJS_Token *tok = get_token(rt);
    RJS_Result r   = RJS_OK;

    if (tok->type != type) {
        parse_unexpect_token_error(rt, &tok->location, type, 0);
        r = RJS_ERR;
    }

    return r;
}

/*Get the expected identifier.*/
static RJS_Result
get_identifier_expect (RJS_Runtime *rt, RJS_IdentifierType itype)
{
    RJS_Token *tok = get_token(rt);
    RJS_Result r   = RJS_OK;

    if (!token_is_identifier(tok->type, tok->flags, itype)) {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER,
                RJS_TOKEN_FL_KNOWN_IDENTIFIER|itype);
        r = RJS_ERR;
    }

    return r;
}

/*Recover the error in switch statement.*/
static void
recover_switch (RJS_Runtime *rt)
{
    RJS_Token *tok = curr_token(rt);

    while (1) {
        if (tok->type == RJS_TOKEN_END)
            break;

        if ((tok->type == RJS_TOKEN_rbrace)
                || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_case)
                || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_default)) {
            unget_token(rt);
            break;
        }

        tok = get_token(rt);
    }
}

/*Recover the error in object.*/
static void
recover_object (RJS_Runtime *rt)
{
    RJS_Token *tok = curr_token(rt);

    while (1) {
        if ((tok->type == RJS_TOKEN_END)
                || (tok->type == RJS_TOKEN_comma))
            break;

        if (tok->type == RJS_TOKEN_rbrace) {
            unget_token(rt);
            break;
        }

        tok = get_token(rt);
    }
}

/*Recover the error in array.*/
static void
recover_array (RJS_Runtime *rt)
{
    RJS_Token *tok = curr_token(rt);

    while (1) {
        if ((tok->type == RJS_TOKEN_END)
                || (tok->type == RJS_TOKEN_comma))
            break;

        if (tok->type == RJS_TOKEN_rbracket) {
            unget_token(rt);
            break;
        }

        tok = get_token(rt);
    }
}

/*Recover the error in parameters.*/
static void
recover_params (RJS_Runtime *rt)
{
    RJS_Token *tok = curr_token(rt);

    while (1) {
        if ((tok->type == RJS_TOKEN_END)
                || (tok->type == RJS_TOKEN_comma))
            break;

        if (tok->type == RJS_TOKEN_rparenthese) {
            unget_token(rt);
            break;
        }

        tok = get_token(rt);
    }
}

/*Recover the error in template literal.*/
static void
recover_template (RJS_Runtime *rt)
{
    RJS_Token *tok = curr_token(rt);

    while (1) {
        if ((tok->type == RJS_TOKEN_END)
                || (tok->type == RJS_TOKEN_TEMPLATE_MIDDLE))
            break;

        if (tok->type == RJS_TOKEN_TEMPLATE_TAIL) {
            unget_token(rt);
            break;
        }

        tok = get_token(rt);
    }
}

/*Recover the error in statement.*/
static void
recover_stmt (RJS_Runtime *rt, RecoverType type)
{
    RJS_Token *tok  = curr_token(rt);
    int        line = tok->location.last_line;

    while (1) {
        if (type == RECOVER_BLOCK) {
            if (tok->type == RJS_TOKEN_rbrace) {
                unget_token(rt);
                break;
            }
        } else if (type == RECOVER_SWITCH) {
            if ((tok->type == RJS_TOKEN_rbrace)
                    || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_case)
                    || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_default)) {
                unget_token(rt);
                break;
            }
        }

        if (tok->type == RJS_TOKEN_semicolon)
            break;

        if (tok->type == RJS_TOKEN_END)
            break;

        tok = get_token(rt);

        if (tok->location.first_line != line) {
            unget_token(rt);
            break;
        }
    }
}

/*Semicolon auto insert.*/
static RJS_Result
auto_semicolon (RJS_Runtime *rt)
{
    RJS_Parser *parser = rt->parser;
    RJS_Token  *tok;

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_semicolon)
        return RJS_OK;

    if ((tok->type == RJS_TOKEN_END)
            || (tok->type == RJS_TOKEN_rbrace)
            || (tok->location.first_line != parser->last_line)) {
        unget_token(rt);
        return RJS_OK;
    }

    parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_semicolon, 0);
    return RJS_ERR;
}

/*Parse the block.*/
static RJS_Result
parse_block (RJS_Runtime *rt, RJS_Value *vb)
{
    RJS_Token    *tok;
    RJS_AstBlock *blk     = NULL;
    RJS_AstFunc  *func    = func_top(rt);
    RJS_Ast      *bot_blk = func->block_stack;
    size_t        top     = rjs_value_stack_save(rt);
    RJS_Value    *vstmt   = rjs_value_stack_push(rt);
    RJS_Result    r       = RJS_ERR;

    if ((r = get_token_expect(rt, RJS_TOKEN_lbrace)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    blk = ast_new(rt, vb, RJS_AST_Block, &tok->location);
    blk->decl = decl_push(rt);
    func->block_stack = &blk->ast;

    break_push(rt, &blk->break_js, &blk->ast);

    while (1) {
        RJS_Result r;

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            r = RJS_ERR;
            goto end;
        }

        unget_token(rt);
        r = parse_stmt_list_item(rt, vstmt);
        if (r == RJS_ERR) {
            recover_stmt(rt, RECOVER_BLOCK);
        } else {
            ast_list_append(rt, &blk->stmt_list, vstmt);
        }
    }

    loc_update_last_token(rt, &blk->ast.location);

    r = RJS_OK;
end:
    if (blk) {
        break_pop(rt);
        decl_pop(rt);
    }

    func->block_stack = bot_blk;

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the block statement.*/
static RJS_Result
parse_block_stmt (RJS_Runtime *rt, RJS_Value *vb)
{
    return parse_block(rt, vb);
}

/*Parse template liternal.*/
static RJS_Result
parse_template_literal (RJS_Runtime *rt, RJS_Value *vt, RJS_Bool is_tagged)
{
    RJS_Token            *tok;
    RJS_Result            r;
    RJS_AstTemplateEntry *te;
    RJS_PropertyDesc      pd;
    size_t                i;
    size_t                nitem  = 0;
    RJS_AstTemplate      *templ  = NULL;
    size_t                top    = rjs_value_stack_save(rt);
    RJS_Value            *tmp    = rjs_value_stack_push(rt);
    RJS_Value            *templv = rjs_value_stack_push(rt);
    RJS_Value            *rawv   = rjs_value_stack_push(rt);

    tok = get_token(rt);

    if ((tok->type != RJS_TOKEN_TEMPLATE) && (tok->type != RJS_TOKEN_TEMPLATE_HEAD)) {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_TEMPLATE, 0);
        r = RJS_ERR;
        goto end;
    }

    if (!is_tagged && (tok->flags & RJS_TOKEN_FL_INVALIE_ESCAPE))
        parse_error(rt, &tok->location, _("invalid escape sequence"));

    templ = ast_new(rt, vt, RJS_AST_Template, &tok->location);
    templ->ve = NULL;

    ast_list_append(rt, &templ->templ_list, tok->value);
    nitem ++;

    if (tok->type == RJS_TOKEN_TEMPLATE_HEAD) {
        while (1) {
            tok = get_token(rt);
            if ((tok->type == RJS_TOKEN_TEMPLATE_TAIL)
                    || (tok->type == RJS_TOKEN_END)) {
                parse_unexpect_error(rt, &tok->location, _("expression"));
                break;
            }

            unget_token(rt);
            r = parse_expr_in(rt, tmp);
            if (r == RJS_ERR) {
                recover_template(rt);
                continue;
            } else {
                check_expr(rt, tmp);
                ast_list_append(rt, &templ->expr_list, tmp);
            }

            tok = get_token(rt);

            if ((tok->type == RJS_TOKEN_TEMPLATE_MIDDLE)
                    || (tok->type == RJS_TOKEN_TEMPLATE_TAIL)) {
                if (!is_tagged && (tok->flags & RJS_TOKEN_FL_INVALIE_ESCAPE))
                    parse_error(rt, &tok->location, _("invalid escape sequence"));

                ast_list_append(rt, &templ->templ_list, tok->value);
                nitem ++;
            }

            if (tok->type == RJS_TOKEN_TEMPLATE_TAIL)
                break;
            if (tok->type != RJS_TOKEN_TEMPLATE_MIDDLE) {
                parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_TEMPLATE_MIDDLE, 0);
                recover_template(rt);
            }
        }

        loc_update_last_token(rt, &templ->ast.location);
    }

    rjs_array_new(rt, templv, nitem, NULL);
    rjs_array_new(rt, rawv, nitem, NULL);

    rjs_property_desc_init(rt, &pd);
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_ENUMERABLE;
    i = 0;

    rjs_list_foreach_c(&templ->templ_list, te, RJS_AstTemplateEntry, ast.ln) {
        rjs_value_copy(rt, pd.value, &te->str);
        rjs_define_property_or_throw_index(rt, templv, i, &pd);

        rjs_value_copy(rt, pd.value, &te->raw_str);
        rjs_define_property_or_throw_index(rt, rawv, i, &pd);

        i ++;
    }

    rjs_property_desc_deinit(rt, &pd);

    rjs_template_new(rt, templv, rawv);
    templ->ve = value_entry_add(rt, &templ->ast.location, templv);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Convert the assignment to binding and initializer.*/
static RJS_Result
assi_to_binding_init (RJS_Runtime *rt, RJS_AstBinaryExpr *assi, RJS_Value *b, RJS_Value *init, RJS_Bool is_binding)
{
    RJS_Result r;
    RJS_Ast   *e = ast_get(rt, &assi->operand1);

    if ((r = expr_to_binding(rt, e, b, is_binding)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, init, &assi->operand2);

    r = RJS_OK;
end:
    return r;
}

/*Convert the spread expression to rest binding.*/
static RJS_Result
spread_to_rest (RJS_Runtime *rt, RJS_AstUnaryExpr *ue, RJS_Value *vr, RJS_Bool is_binding)
{
    RJS_AstRest *rest;
    RJS_Ast     *e;
    RJS_Result   r;

    rest = ast_new(rt, vr, RJS_AST_Rest, &ue->ast.location);
    e    = ast_get(rt, &ue->operand);

    if ((r = expr_to_binding(rt, e, &rest->binding, is_binding)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    return r;
}

/*Convert the array to array binding pattern.*/
static RJS_Result
array_to_binding (RJS_Runtime *rt, RJS_AstList *a, RJS_Value *b, RJS_Bool is_binding)
{
    RJS_Result   r;
    RJS_Ast     *ast, *nast;
    RJS_AstList *ab;
    size_t       top      = rjs_value_stack_save(rt);
    RJS_Value   *tmp      = rjs_value_stack_push(rt);
    RJS_Bool     has_rest = RJS_FALSE;

    ab = ast_new(rt, b, RJS_AST_ArrayBinding, &a->ast.location);

    rjs_list_foreach_safe_c(&a->list, ast, nast, RJS_Ast, ln) {
        if (has_rest)
            parse_error(rt, &ast->location, _("rest element may not be followed by any element"));

        switch (ast->type) {
        case RJS_AST_LastElision:
            break;
        case RJS_AST_Elision:
            rjs_list_remove(&ast->ln);
            rjs_list_append(&ab->list, &ast->ln);
            break;
        case RJS_AST_SpreadExpr: {
            RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

            has_rest = RJS_TRUE;

            if ((r = spread_to_rest(rt, ue, tmp, is_binding)) == RJS_ERR)
                goto end;

            ast_list_append(rt, &ab->list, tmp);
            break;
        }
        default: {
            RJS_AstBindingElem *be;
            RJS_AstId          *id;
            RJS_Ast            *init;

            be = ast_new(rt, tmp, RJS_AST_BindingElem, &ast->location);
            ast_list_append(rt, &ab->list, tmp);

            if ((r = expr_to_binding_init(rt, ast, &be->binding, &be->init, is_binding)) == RJS_ERR)
                goto end;

            id   = id_get(rt, &be->binding);
            init = ast_get(rt, &be->init);

            if (init && id)
                func_set_name(rt, init, &id->ast.location, &id->identifier->value);
            break;
        }
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Convert the object to object binding pattern.*/
static RJS_Result
object_to_binding (RJS_Runtime *rt, RJS_AstList *o, RJS_Value *b, RJS_Bool is_binding)
{
    RJS_Result   r;
    RJS_Ast     *ast;
    RJS_AstList *ob;
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *tmp = rjs_value_stack_push(rt);

    ob = ast_new(rt, b, RJS_AST_ObjectBinding, &o->ast.location);

    rjs_list_foreach_c(&o->list, ast, RJS_Ast, ln) {
        switch (ast->type) {
        case RJS_AST_SpreadExpr: {
            RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

            if ((r = spread_to_rest(rt, ue, tmp, is_binding)) == RJS_ERR)
                goto end;

            ast_list_append(rt, &ob->list, tmp);
            break;
        }
        case RJS_AST_Prop:
        case RJS_AST_SetProto: {
            RJS_AstProp        *p = (RJS_AstProp*)ast;
            RJS_AstBindingProp *bp;
            RJS_AstId          *id;
            RJS_Ast            *expr, *init;

            bp = ast_new(rt, tmp, RJS_AST_BindingProp, &ast->location);
            ast_list_append(rt, &ob->list, tmp);

            rjs_value_copy(rt, &bp->name, &p->name);
            rjs_value_copy(rt, &bp->init, &p->init);

            expr = ast_get(rt, &p->value);
            if (expr) {
                if ((r = expr_to_binding_init(rt, expr, &bp->binding, &bp->init, is_binding)) == RJS_ERR)
                    goto end;
            } else {
                RJS_AstValueExpr *ve = ast_get(rt, &p->name);

                id_new(rt, &bp->binding, &ast->location, &ve->ve->value);
            }

            id   = id_get(rt, &bp->binding);
            init = ast_get(rt, &bp->init);
            if (init && id)
                func_set_name(rt, init, &id->ast.location, &id->identifier->value);
            break;
        }
        default:
            parse_error(rt, &ast->location, _("illegal binding property"));
            r = RJS_ERR;
            goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Convert the expression to binding.*/
static RJS_Result
expr_to_binding (RJS_Runtime *rt, RJS_Ast *e, RJS_Value *b, RJS_Bool is_binding)
{
    RJS_Result r;

    switch (e->type) {
    case RJS_AST_Id:
        if (is_binding) {
            RJS_AstId *id = (RJS_AstId*)e;

            check_binding_identifier(rt, &e->location, &id->identifier->value);
        }

        rjs_value_set_gc_thing(rt, b, e);
        break;
    case RJS_AST_MemberExpr:
    case RJS_AST_SuperMemberExpr:
    case RJS_AST_PrivMemberExpr:
    case RJS_AST_ArrayBinding:
    case RJS_AST_ObjectBinding:
        rjs_value_set_gc_thing(rt, b, e);
        break;
    case RJS_AST_Array: {
        RJS_AstList *a = (RJS_AstList*)e;

        if ((r = array_to_binding(rt, a, b, is_binding)) == RJS_ERR)
            goto end;
        break;
    }
    case RJS_AST_Object: {
        RJS_AstList *o = (RJS_AstList*)e;

        if ((r = object_to_binding(rt, o, b, is_binding)) == RJS_ERR)
            goto end;
        break;
    }
    default:
        parse_error(rt, &e->location, _("illegal binding"));
        r = RJS_ERR;
        goto end;
    }

    r = RJS_OK;
end:
    return r;
}

/*Convert the expression to binding and initializer.*/
static RJS_Result
expr_to_binding_init (RJS_Runtime *rt, RJS_Ast *e, RJS_Value *b, RJS_Value *init, RJS_Bool is_binding)
{
    RJS_Result r;

    if (e->type == RJS_AST_AssiExpr) {
        RJS_AstBinaryExpr *assi = (RJS_AstBinaryExpr*)e;

        r = assi_to_binding_init(rt, assi, b, init, is_binding);
    } else {
        r = expr_to_binding(rt, e, b, is_binding);
    }

    return r;
}

#if ENABLE_ARROW_FUNC

/*Convert arguments to parameters.*/
static RJS_Result
args_to_params (RJS_Runtime *rt, RJS_Location *loc, RJS_List *al, RJS_AstType type, RJS_Value *vp)
{
    RJS_AstArrowParams *pl;
    RJS_Ast            *ast, *nast;
    RJS_Result          r;
    RJS_AstBindingElem *be;
    size_t              top      = rjs_value_stack_save(rt);
    RJS_Value          *tmp      = rjs_value_stack_push(rt);
    RJS_Bool            has_rest = RJS_FALSE;

    pl = ast_new(rt, vp, type, loc);

    if (al) {
        rjs_list_foreach_safe_c(al, ast, nast, RJS_Ast, ln) {
            if (has_rest)
                parse_error(rt, &ast->location, _("rest parameter cannot be followed by any parameter"));

            switch (ast->type) {
            case RJS_AST_LastElision:
                break;
            case RJS_AST_Elision:
            case RJS_AST_Rest:
                rjs_list_append(&pl->param_list, &ast->ln);
                break;
            case RJS_AST_SpreadExpr: {
                RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

                if ((r = spread_to_rest(rt, ue, tmp, RJS_TRUE)) == RJS_ERR)
                    goto end;

                ast_list_append(rt, &pl->param_list, tmp);
                has_rest = RJS_TRUE;
                break;
            }
            default: {
                be = ast_new(rt, tmp, RJS_AST_BindingElem, &ast->location);
                ast_list_append(rt, &pl->param_list, tmp);

                if ((r = expr_to_binding_init(rt, ast, &be->binding, &be->init, RJS_TRUE)) == RJS_ERR)
                    goto end;
                break;
            }
            }
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_ARROW_FUNC*/

/*Parse parenthesized expression or parameters.*/
static RJS_Result
parse_parentheses_or_params (RJS_Runtime *rt, RJS_Value *ve, int prio)
{
    RJS_Token        *tok;
    RJS_Token        *ntok;
    RJS_Result        r;
    RJS_AstUnaryExpr *pe;
    RJS_AstList      *ce       = NULL;
    RJS_Bool          has_rest = RJS_FALSE;
    RJS_Bool          is_arrow = RJS_FALSE;
    size_t            top      = rjs_value_stack_save(rt);
    RJS_Value        *tmp      = rjs_value_stack_push(rt);

    no_strict_list_push(rt);
    contains_list_push(rt);

    if ((r = get_token_expect(rt, RJS_TOKEN_lparenthese)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    pe  = ast_new(rt, ve, RJS_AST_ParenthesesExpr, &tok->location);

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rparenthese)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rparenthese, 0);
            r = RJS_ERR;
            goto end;
        }

        if (!ce)
            ce = ast_new(rt, &pe->operand, RJS_AST_CommaExpr, &tok->location);

        if (tok->type == RJS_TOKEN_dot_dot_dot) {
            RJS_AstRest *rest;

            has_rest = RJS_TRUE;

            rest = ast_new(rt, tmp, RJS_AST_Rest, &tok->location);
            r    = parse_binding(rt, &rest->binding);
            if (r == RJS_OK) {
                loc_update_last_ast(rt, &rest->ast.location, &rest->binding);
                loc_update_last_ast(rt, &ce->ast.location, &rest->binding);
                rjs_list_append(&ce->list, &rest->ast.ln);
            }
        } else {
            unget_token(rt);
            r = parse_expr_in_prio(rt, PRIO_ASSI, tmp);
            if (r == RJS_OK) {
                loc_update_last_ast(rt, &ce->ast.location, tmp);
                ast_list_append(rt, &ce->list, tmp);
            }
        }

        if (r == RJS_ERR) {
            recover_params(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rparenthese)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `)\'"));
            recover_params(rt);
        } else if (has_rest) {
            parse_error(rt, &tok->location, _("`...\' cannot be followed with `,\'"));
        }
    }

    loc_update_last_token(rt, &pe->ast.location);

    ntok = next_token_div(rt);
#if ENABLE_ARROW_FUNC
    /*Check if the next token is "=>".*/
    if ((prio <= PRIO_ASSI) && (ntok->type == RJS_TOKEN_eq_gt)) {
        RJS_AstArrowParams *pl;

        /*Convert (...) to arrow parameters.*/
        if ((r = args_to_params(rt,
                &pe->ast.location,
                ce ? &ce->list : NULL,
                RJS_AST_ArrowParams, tmp)) == RJS_ERR)
            goto end;

        is_arrow = RJS_TRUE;
        pl = ast_get(rt, tmp);

        no_strict_list_save(rt, &pl->no_strict_list);
        contains_list_check(rt, CONTAINS_FL_AWAIT_EXPR|CONTAINS_FL_YIELD_EXPR);

        rjs_value_copy(rt, ve, tmp);
    } else
#endif /*ENABLE_ARROW_FUNC*/
    if (has_rest || !ce) {
        /*() or (...) must be an arrow parameter.*/
        parse_unexpect_token_error(rt, &ntok->location, RJS_TOKEN_eq_gt, 0);
        r = RJS_ERR;
        goto end;
    } else if (rjs_list_has_1_node(&ce->list)) {
        /*Only 1 expression in ().*/
        RJS_Ast *e = RJS_CONTAINER_OF(ce->list.next, RJS_Ast, ln);

        rjs_value_set_gc_thing(rt, &pe->operand, e);
    }

    r = RJS_OK;
end:
    if (!is_arrow)
        no_strict_list_pop(rt, RJS_FALSE, RJS_TRUE);

    contains_list_pop(rt, RJS_TRUE);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the array literal.*/
static RJS_Result
parse_array_literal (RJS_Runtime *rt, RJS_Value *va)
{
    RJS_Token   *tok;
    RJS_Result   r;
    RJS_AstList *arr;
    size_t       top      = rjs_value_stack_save(rt);
    RJS_Value   *tmp      = rjs_value_stack_push(rt);
    RJS_Bool     has_elem = RJS_FALSE;

    if ((r = get_token_expect(rt, RJS_TOKEN_lbracket)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    arr = ast_new(rt, va, RJS_AST_Array, &tok->location);

    while (1) {
        tok = get_token(rt);

        if (tok->type == RJS_TOKEN_rbracket) {
            if (has_elem) {
                RJS_Ast *e;

                e = ast_new(rt, tmp, RJS_AST_LastElision, &tok->location);
                rjs_list_append(&arr->list, &e->ln);
            }
            break;
        }

        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbracket, 0);
            r = RJS_ERR;
            goto end;
        }

        r = RJS_OK;
        if (tok->type == RJS_TOKEN_comma) {
            RJS_Ast *e;

            e = ast_new(rt, tmp, RJS_AST_Elision, &tok->location);
            rjs_list_append(&arr->list, &e->ln);
        } else {
            if (tok->type == RJS_TOKEN_dot_dot_dot) {
                RJS_AstUnaryExpr *ue;

                ue = ast_new(rt, tmp, RJS_AST_SpreadExpr, &tok->location);

                r = parse_expr_in_prio(rt, PRIO_ASSI, &ue->operand);
                if (r == RJS_OK) {
                    loc_update_last_ast(rt, &ue->ast.location, &ue->operand);
                    rjs_list_append(&arr->list, &ue->ast.ln);
                }
            } else {
                unget_token(rt);
                r = parse_expr_in_prio(rt, PRIO_ASSI, tmp);
                if (r == RJS_OK)
                    ast_list_append(rt, &arr->list, tmp);
            }

            if (r == RJS_ERR) {
                recover_array(rt);
                continue;
            }

            tok = get_token(rt);
            if (tok->type == RJS_TOKEN_rbracket)
                break;
            if (tok->type != RJS_TOKEN_comma) {
                parse_unexpect_error(rt, &tok->location, _("`,\' or `]\'"));
                recover_array(rt);
            }

            has_elem = RJS_TRUE;
        }
    }

    loc_update_last(&arr->ast.location, &tok->location);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the object literal.*/
static RJS_Result
parse_object_literal (RJS_Runtime *rt, RJS_Value *vo)
{
    RJS_Token   *tok, *ntok;
    RJS_Result   r;
    RJS_AstList *o;
    RJS_AstProp *prop;
    RJS_Parser  *parser    = rt->parser;
    size_t       top       = rjs_value_stack_save(rt);
    RJS_Value   *tmp1      = rjs_value_stack_push(rt);
    RJS_Value   *tmp2      = rjs_value_stack_push(rt);
    int          old_flags = parser->flags;

    if ((r = get_token_expect(rt, RJS_TOKEN_lbrace)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    o   = ast_new(rt, vo, RJS_AST_Object, &tok->location);

    while (1) {
        tok = get_token(rt);

        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            r = RJS_ERR;
            goto end;
        }

        r = RJS_OK;
        if (tok->type == RJS_TOKEN_dot_dot_dot) {
            RJS_AstUnaryExpr *ue;

            ue = ast_new(rt, tmp1, RJS_AST_SpreadExpr, &tok->location);

            r = parse_expr_in_prio(rt, PRIO_ASSI, &ue->operand);
            if (r == RJS_OK)
                rjs_list_append(&o->list, &ue->ast.ln);
        } else {
            ntok = next_token(rt);

            if ((ntok->type == RJS_TOKEN_comma) || (ntok->type == RJS_TOKEN_rbrace)) {
                if (!is_identifier_reference(rt, tok->type, tok->flags)) {
                    parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                    r = RJS_ERR;
                } else {
                    check_identifier_reference(rt, &tok->location, tok->value);

                    prop = ast_new(rt, tmp1, RJS_AST_Prop, &tok->location);

                    prop->computed = RJS_FALSE;

                    value_expr_new(rt, &prop->name, &tok->location, tok->value);

                    rjs_list_append(&o->list, &prop->ast.ln);
                }
            } else if (ntok->type == RJS_TOKEN_eq) {
                prop = ast_new(rt, tmp1, RJS_AST_Prop, &tok->location);

                prop->computed = RJS_FALSE;

                if (!is_identifier_reference(rt, tok->type, tok->flags)) {
                    parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                    r = RJS_ERR;
                } else {
                    check_identifier_reference(rt, &tok->location, tok->value);

                    value_expr_new(rt, &prop->name, &tok->location, tok->value);

                    get_token(rt);
                    r = parse_expr_in_prio(rt, PRIO_ASSI, &prop->init);
                    if (r == RJS_OK) {
                        loc_update_last_ast(rt, &prop->ast.location, &prop->init);
                        rjs_list_append(&o->list, &prop->ast.ln);
                    }
                }
            } else {
                RJS_Bool is_method = RJS_FALSE;

#if ENABLE_GENERATOR
                if (tok->type == RJS_TOKEN_star)
                    is_method = RJS_TRUE;
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
                if (is_async_method(rt, tok->type, tok->flags)
                        || is_accessor_method(rt, tok->type, tok->flags))
                    is_method = RJS_TRUE;
#endif /*ENABLE_ASYNC*/

                if (is_method) {
                    unget_token(rt);

                    r = parse_method(rt, RJS_FALSE, RJS_TRUE, tmp1);
                    if (r == RJS_OK)
                        ast_list_append(rt, &o->list, tmp1);
                } else {
                    RJS_Bool computed_pn = RJS_FALSE;

                    if (tok->type == RJS_TOKEN_lbracket)
                        computed_pn = RJS_TRUE;

                    unget_token(rt);

                    r = parse_class_element_name(rt, RJS_TRUE, RJS_FALSE, 0, tmp1);

                    if (r == RJS_OK) {
                        tok = get_token(rt);

                        if (tok->type == RJS_TOKEN_lparenthese) {
                            RJS_AstClassElem *m;
                            RJS_Ast          *cen = ast_get(rt, tmp1);

                            m = ast_new(rt, tmp2, RJS_AST_ClassElem, cen ? &cen->location : &tok->location);

                            m->is_static = RJS_FALSE;
                            m->computed  = computed_pn;
                            m->type      = RJS_AST_CLASS_ELEM_METHOD;

                            rjs_value_copy(rt, &m->name, tmp1);

                            parser->flags &= ~(RJS_PARSE_FL_AWAIT|RJS_PARSE_FL_YIELD);

                            unget_token(rt);
                            r = parse_method_params_body(rt, RJS_TRUE, m, 0);
                            if (r == RJS_OK)
                                rjs_list_append(&o->list, &m->ast.ln);
                        } else if (tok->type == RJS_TOKEN_colon) {
                            RJS_Ast *ast = ast_get(rt, tmp1);

                            prop = ast_new(rt, tmp2, RJS_AST_Prop, &ast->location);
                            prop->computed = computed_pn;
                            rjs_value_copy(rt, &prop->name, tmp1);

                            r = parse_expr_in_prio(rt, PRIO_ASSI, &prop->value);
                            if (r == RJS_OK) {
                                loc_update_last_ast(rt, &prop->ast.location, &prop->value);
                                rjs_list_append(&o->list, &prop->ast.ln);
                            }
                        } else {
                            parse_unexpect_error(rt, &tok->location, _("`:\' or `(\'"));
                            r = RJS_ERR;
                        }
                    }
                }
            }
        }

        if (r == RJS_ERR) {
            recover_object(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `}\'"));
            recover_object(rt);
        }
    }

    loc_update_last_token(rt, &o->ast.location);

    /*Check "__proto__"*/
    ntok = next_token_div(rt);
    if (ntok->type != RJS_TOKEN_eq) {
        RJS_Ast     *ast;
        RJS_Location proto_loc;
        RJS_Bool     has_proto = RJS_FALSE;

        rjs_list_foreach_c(&o->list, ast, RJS_Ast, ln) {
            RJS_AstProp      *prop;
            RJS_Ast          *nast;
            RJS_AstValueExpr *ve;

            if (ast->type != RJS_AST_Prop)
                continue;

            prop = (RJS_AstProp*)ast;
            if (prop->computed)
                continue;

            if (!ast_get(rt, &prop->value))
                continue;

            nast = ast_get(rt, &prop->name);
            if (!nast || (nast->type != RJS_AST_ValueExpr))
                continue;

            ve = (RJS_AstValueExpr*)nast;
            if (rjs_same_value(rt, &ve->ve->value, rjs_s___proto__(rt))) {
                /*Only 1 "__proto__" can be used.*/
                if (has_proto) {
                    parse_error(rt, &nast->location, _("\"__proto__\" is already defined"));
                    parse_prev_define_note(rt, &proto_loc);
                } else {
                    has_proto = RJS_TRUE;
                    proto_loc = nast->location;
                }

                /*Use "__proto__" to set prototype.*/
                ast->type = RJS_AST_SetProto;
            }
        }
    }

    r = RJS_OK;
end:
    parser->flags = old_flags;
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse arguments.*/
static RJS_Result
parse_arguments (RJS_Runtime *rt, RJS_List *list)
{
    RJS_Result   r;
    RJS_Token   *tok;
    size_t       top     = rjs_value_stack_save(rt);
    RJS_Value   *tmp     = rjs_value_stack_push(rt);
    RJS_Bool     has_arg = RJS_FALSE;

    if ((r = get_token_expect(rt, RJS_TOKEN_lparenthese)) == RJS_ERR)
        goto end;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rparenthese) {
            if (has_arg) {
                ast_new(rt, tmp, RJS_AST_LastElision, &tok->location);
                ast_list_append(rt, list, tmp);
            }
            break;
        }

        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rparenthese, 0);
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_dot_dot_dot) {
            RJS_AstUnaryExpr *ue;

            ue = ast_new(rt, tmp, RJS_AST_SpreadExpr, &tok->location);

            r = parse_expr_in_prio(rt, PRIO_ASSI, &ue->operand);
            if (r == RJS_OK) {
                loc_update_last_ast(rt, &ue->ast.location, &ue->operand);
                rjs_list_append(list, &ue->ast.ln);
            }
        } else {
            unget_token(rt);
            r = parse_expr_in_prio(rt, PRIO_ASSI, tmp);
            if (r == RJS_OK)
                ast_list_append(rt, list, tmp);
        }

        if (r == RJS_ERR) {
            recover_params(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rparenthese)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `)\'"));
            recover_params(rt);
        }

        has_arg = RJS_TRUE;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse super expression.*/
static RJS_Result
parse_super_expr (RJS_Runtime *rt, RJS_Value *ve)
{
    RJS_Result    r;
    RJS_Token    *tok;
    RJS_Location  loc;
    RJS_AstType   ref_type = RJS_AST_SuperPropRef;
    RJS_AstFunc  *func     = func_top(rt);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_super)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    loc = tok->location;

    tok = get_token(rt);
    switch (tok->type) {
    case RJS_TOKEN_dot: {
        RJS_AstBinaryExpr *bin;

        bin = ast_new(rt, ve, RJS_AST_SuperMemberExpr, &loc);

        tok = get_token(rt);
        if (tok->type != RJS_TOKEN_IDENTIFIER) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
            r = RJS_ERR;
            goto end;
        }

        prop_ref_new(rt, &bin->operand2, &tok->location, func, tok->value);

        loc_update_last_token(rt, &bin->ast.location);
        break;
    }
    case RJS_TOKEN_lbracket: {
        RJS_AstBinaryExpr *bin;

        bin = ast_new(rt, ve, RJS_AST_SuperMemberExpr, &loc);

        if ((r = parse_expr_in(rt, &bin->operand2)) == RJS_ERR)
            goto end;
        if ((r = get_token_expect(rt, RJS_TOKEN_rbracket)) == RJS_ERR)
            goto end;

        loc_update_last_token(rt, &bin->ast.location);
        break;
    }
    case RJS_TOKEN_lparenthese: {
        RJS_AstCall *call;

        call = ast_new(rt, ve, RJS_AST_SuperCallExpr, &loc);

        unget_token(rt);
        if ((r = parse_arguments(rt, &call->arg_list)) == RJS_ERR)
            goto end;

        tok = curr_token(rt);
        loc_update_last(&call->ast.location, &tok->location);

        ref_type = RJS_AST_SuperCallRef;
        break;
    }
    default:
        parse_unexpect_error(rt, &tok->location, _("`.\', `[\' or `(\'"));
        r = RJS_ERR;
        goto end;
    }

    /*Add the expression to contains list.*/
    contains_list_add(rt, ref_type, &loc);

    r = RJS_OK;
end:
    return r;
}

#if ENABLE_MODULE

/*Parse import expression.*/
static RJS_Result
parse_import_expr (RJS_Runtime *rt, RJS_Value *ve)
{
    RJS_Result   r;
    RJS_Token   *tok;
    RJS_Location loc;
    RJS_Parser  *parser = rt->parser;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_import)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    loc = tok->location;

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_dot) {
        RJS_Ast *ast;

        ast = ast_new(rt, ve, RJS_AST_ImportMetaExpr, &loc);

        if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_meta)) == RJS_ERR)
            goto end;

        loc_update_last_token(rt, &ast->location);

        if (!(parser->flags & RJS_PARSE_FL_MODULE)) {
            parse_error(rt, &ast->location, _("\"import.meta\" can only be used in module"));
            r = RJS_ERR;
            goto end;
        }
    } else if (tok->type == RJS_TOKEN_lparenthese) {
        RJS_AstUnaryExpr *ue;

        ue = ast_new(rt, ve, RJS_AST_ImportExpr, &loc);

        if ((r = parse_expr_in_prio(rt, PRIO_ASSI, &ue->operand)) == RJS_ERR)
            goto end;

        if ((r = get_token_expect(rt, RJS_TOKEN_rparenthese)) == RJS_ERR)
            goto end;

        loc_update_last_token(rt, &ue->ast.location);
    } else {
        parse_unexpect_error(rt, &tok->location, _("`.\' or `(\'"));
        r = RJS_ERR;
        goto end;
    }

    r = RJS_OK;
end:
    return r;
}

#endif /*ENABLE_MODULE*/

/*Parse new expression.*/
static RJS_Result
parse_new_expr (RJS_Runtime *rt, RJS_Value *ve)
{
    RJS_Result    r;
    RJS_Token    *tok;
    RJS_Location  loc;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_new)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    loc = tok->location;

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_dot) {
        RJS_Ast *ast;

        ast = ast_new(rt, ve, RJS_AST_NewTargetExpr, &loc);

        if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_target)) == RJS_ERR)
            goto end;

        loc_update_last_token(rt, &ast->location);

        /*Add new.target to contains list.*/
        contains_list_add(rt, RJS_AST_NewTargetRef, &ast->location);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_new)) {
        RJS_AstCall *ne;

        ne = ast_new(rt, ve, RJS_AST_NewExpr, &loc);

        unget_token(rt);
        if ((r = parse_expr_prio(rt, PRIO_NEW, &ne->func)) == RJS_ERR)
            goto end;

        loc_update_last_ast(rt, &ne->ast.location, &ne->func);
    } else {
        RJS_AstCall *ne;

        ne = ast_new(rt, ve, RJS_AST_NewExpr, &loc);

        unget_token(rt);
        if ((r = parse_expr_prio(rt, PRIO_MEMBER, &ne->func)) == RJS_ERR)
            goto end;

        loc_update_last_ast(rt, &ne->ast.location, &ne->func);

        tok = get_token(rt);
        unget_token(rt);
        if (tok->type == RJS_TOKEN_lparenthese) {
            if ((r = parse_arguments(rt, &ne->arg_list)) == RJS_ERR)
                goto end;

            loc_update_last_token(rt, &ne->ast.location);
        }
    }

    r = RJS_OK;
end:
    return r;
}

/*Parse unary expression.*/
static RJS_Result
parse_unary_expr (RJS_Runtime *rt, RJS_AstType type, RJS_Value *ve)
{
    RJS_Result        r;
    RJS_AstUnaryExpr *ue;
    RJS_Token        *tok;

    tok = curr_token(rt);
    ue  = ast_new(rt, ve, type, &tok->location);

    if ((r = parse_expr_prio(rt, PRIO_UNARY, &ue->operand)) == RJS_ERR)
        goto end;

    if ((type == RJS_AST_PreIncExpr) || (type == RJS_AST_PreDecExpr))
        check_simple_assi_target(rt, &ue->operand);

    loc_update_last_ast(rt, &ue->ast.location, &ue->operand);

    r = RJS_OK;
end:
    return r;
}

/*Parse update expression.*/
static RJS_Result
parse_update_expr (RJS_Runtime *rt, RJS_AstType type, RJS_Value *ve, RJS_Value *vop)
{
    RJS_AstUnaryExpr *ue;
    RJS_Ast          *ast;

    check_simple_assi_target(rt, vop);

    ast = ast_get(rt, vop);
    ue  = ast_new(rt, ve, type, &ast->location);

    rjs_value_copy(rt, &ue->operand, vop);

    loc_update_last_token(rt, &ue->ast.location);
    return RJS_OK;
}

/*Parse binary expression.*/
static RJS_Result
parse_binary_expr (RJS_Runtime *rt, Priority prio, RJS_AstType type, RJS_Value *ve, RJS_Value *vop1)
{
    RJS_AstBinaryExpr *be;
    RJS_Ast           *ast;
    RJS_Result         r;

    ast = ast_get(rt, vop1);
    be  = ast_new(rt, ve, type, &ast->location);

    rjs_value_copy(rt, &be->operand1, vop1);

    if ((r = parse_expr_prio(rt, prio, &be->operand2)) == RJS_ERR)
        goto end;

    loc_update_last_ast(rt, &be->ast.location, &be->operand2);

    r = RJS_OK;
end:
    return r;
}

/*Parse logic and expression.*/
static RJS_Result
parse_logic_and_expr (RJS_Runtime *rt, Priority prio, RJS_Value *ve, RJS_Value *vop1)
{
    RJS_Ast   *ast;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *re  = rjs_value_stack_push(rt);
    int        b   = -1;

    if ((r = parse_expr_prio(rt, prio, re)) == RJS_ERR)
        goto end;

    ast = ast_get(rt, vop1);

    if (ast->type == RJS_AST_ValueExpr) {
        RJS_AstValueExpr *v = (RJS_AstValueExpr*)ast;

        b = rjs_to_boolean(rt, &v->ve->value);
    } else if (ast->type == RJS_AST_True) {
        b = RJS_TRUE;
    } else if ((ast->type == RJS_AST_False) || (ast->type == RJS_AST_Null)) {
        b = RJS_FALSE;
    }

    if (b == RJS_TRUE) {
        rjs_value_copy(rt, ve, re);
    } else if (b == RJS_FALSE) {
        rjs_value_copy(rt, ve, vop1);
    } else {
        RJS_AstBinaryExpr *be;

        be  = ast_new(rt, ve, RJS_AST_AndExpr, &ast->location);

        rjs_value_copy(rt, &be->operand1, vop1);
        rjs_value_copy(rt, &be->operand2, re);

        loc_update_last_ast(rt, &be->ast.location, &be->operand2);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse logic or expression.*/
static RJS_Result
parse_logic_or_expr (RJS_Runtime *rt, Priority prio, RJS_Value *ve, RJS_Value *vop1)
{
    RJS_Ast   *ast;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *re  = rjs_value_stack_push(rt);
    int        b   = -1;

    if ((r = parse_expr_prio(rt, prio, re)) == RJS_ERR)
        goto end;

    ast = ast_get(rt, vop1);

    if (ast->type == RJS_AST_ValueExpr) {
        RJS_AstValueExpr *v = (RJS_AstValueExpr*)ast;

        b = rjs_to_boolean(rt, &v->ve->value);
    } else if (ast->type == RJS_AST_True) {
        b = RJS_TRUE;
    } else if ((ast->type == RJS_AST_False) || (ast->type == RJS_AST_Null)) {
        b = RJS_FALSE;
    }

    if (b == RJS_TRUE) {
        rjs_value_copy(rt, ve, vop1);
    } else if (b == RJS_FALSE) {
        rjs_value_copy(rt, ve, re);
    } else {
        RJS_AstBinaryExpr *be;

        be  = ast_new(rt, ve, RJS_AST_OrExpr, &ast->location);

        rjs_value_copy(rt, &be->operand1, vop1);
        rjs_value_copy(rt, &be->operand2, re);

        loc_update_last_ast(rt, &be->ast.location, &be->operand2);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse question expression.*/
static RJS_Result
parse_ques_expr (RJS_Runtime *rt, Priority prio, RJS_Value *ve, RJS_Value *vop1)
{
    RJS_Ast   *ast;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *re  = rjs_value_stack_push(rt);
    int        b   = -1;

    if ((r = parse_expr_prio(rt, prio, re)) == RJS_ERR)
        goto end;

    ast = ast_get(rt, vop1);

    if (ast->type == RJS_AST_ValueExpr) {
        RJS_AstValueExpr *v = (RJS_AstValueExpr*)ast;

        b = rjs_value_is_undefined(rt, &v->ve->value)
                || rjs_value_is_null(rt, &v->ve->value);
    } else if (ast->type == RJS_AST_Null) {
        b = RJS_TRUE;
    } else if (ast->type == RJS_AST_Id) {
        RJS_AstId *id = (RJS_AstId*)ast;

        b = rjs_string_equal(rt, &id->identifier->value, rjs_s_undefined(rt));
    }

    if (b == RJS_TRUE) {
        rjs_value_copy(rt, ve, re);
    } else if (r == RJS_FALSE) {
        rjs_value_copy(rt, ve, vop1);
    } else {
        RJS_AstBinaryExpr *be;

        be  = ast_new(rt, ve, RJS_AST_QuesExpr, &ast->location);

        rjs_value_copy(rt, &be->operand1, vop1);
        rjs_value_copy(rt, &be->operand2, re);

        loc_update_last_ast(rt, &be->ast.location, &be->operand2);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_GENERATOR

/*Parse yield expression.*/
static RJS_Result
parse_yield_expr (RJS_Runtime *rt, RJS_Value *ve)
{
    RJS_Parser       *parser    = rt->parser;
    int               old_flags = parser->flags;
    RJS_Result        r;
    RJS_Token        *tok;
    RJS_Location      loc;
    RJS_AstUnaryExpr *ue;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_yield)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    loc = tok->location;

    parser->flags |= RJS_PARSE_FL_YIELD;

    tok = get_token(rt);
    if (tok->location.first_line == parser->last_line) {
        if (tok->type == RJS_TOKEN_star) {
            ue = ast_new(rt, ve, RJS_AST_YieldStarExpr, &loc);

            if ((r = parse_expr_prio(rt, PRIO_ASSI, &ue->operand)) == RJS_ERR)
                goto end;
        } else if ((tok->type == RJS_TOKEN_plus)
                || (tok->type == RJS_TOKEN_minus)
                || (tok->type == RJS_TOKEN_plus_plus)
                || (tok->type == RJS_TOKEN_minus_minus)
                || (tok->type == RJS_TOKEN_exclamation)
                || (tok->type == RJS_TOKEN_tilde)
                || (tok->type == RJS_TOKEN_lparenthese)
                || (tok->type == RJS_TOKEN_lbrace)
                || (tok->type == RJS_TOKEN_lbracket)
                || (tok->type == RJS_TOKEN_NUMBER)
                || (tok->type == RJS_TOKEN_STRING)
                || (tok->type == RJS_TOKEN_REGEXP)
                || (tok->type == RJS_TOKEN_TEMPLATE)
                || (tok->type == RJS_TOKEN_TEMPLATE_HEAD)
                || (tok->type == RJS_TOKEN_IDENTIFIER)) {
            ue = ast_new(rt, ve, RJS_AST_YieldExpr, &loc);

            unget_token(rt);
            if ((r = parse_expr_prio(rt, PRIO_ASSI, &ue->operand)) == RJS_ERR)
                goto end;
        } else { 
            unget_token(rt);

            ue = ast_new(rt, ve, RJS_AST_YieldExpr, &loc);
        }

        loc_update_last_ast(rt, &ue->ast.location, &ue->operand);
    } else {
        unget_token(rt);

        ue = ast_new(rt, ve, RJS_AST_YieldExpr, &loc);
    }

    /*Add yield expression to contains list.*/
    contains_list_add(rt, RJS_AST_YieldExprRef, &ue->ast.location);

    r = RJS_OK;
end:
    parser->flags = old_flags;
    return r;
}

#endif /*ENABLE_GENERATOR*/

#if ENABLE_PRIV_NAME

/*Push a private environment.*/
static RJS_AstPrivEnv*
priv_env_push (RJS_Runtime *rt)
{
    RJS_Parser     *parser = rt->parser;
    size_t          top    = rjs_value_stack_save(rt);
    RJS_Value      *tmp    = rjs_value_stack_push(rt);
    RJS_AstPrivEnv *env;

    env = ast_new(rt, tmp, RJS_AST_PrivEnv, NULL);

    env->id = -1;

    rjs_list_init(&env->priv_id_list);
    hash_init(&env->priv_id_hash);

    env->bot = parser->priv_env_stack;
    parser->priv_env_stack = env;

    ast_list_append(rt, &parser->priv_env_list, tmp);

    rjs_value_stack_restore(rt, top);
    return env;
}

/*Popup the top private environment.*/
static void
priv_env_pop (RJS_Runtime *rt)
{
    RJS_Parser     *parser = rt->parser;
    RJS_AstPrivEnv *env    = parser->priv_env_stack;

    assert(env);

    parser->priv_env_stack = env->bot;
}

/*Add a private identifier.*/
static RJS_AstPrivId*
priv_id_new (RJS_Runtime *rt, RJS_Token *tok, RJS_Value *v, int flags)
{
    RJS_Parser     *parser = rt->parser;
    RJS_AstPrivEnv *env    = parser->priv_env_stack;
    RJS_HashEntry  *he, **phe;
    RJS_AstPrivId  *pid;

    assert(env);

    if (rjs_string_equal(rt, tok->value, rjs_s_hash_constructor(rt)))
        parse_error(rt, &tok->location, _("\"#constructor\" cannot be used as private identifier"));

    he = hash_lookup(rt, &env->priv_id_hash, tok->value, &phe);
    if (he) {
        int accessor_flags = PRIV_ID_FL_GET|PRIV_ID_FL_SET;
        int old_aflags, new_aflags;

        pid = RJS_CONTAINER_OF(he, RJS_AstPrivId, he);

        old_aflags = pid->flags & accessor_flags;
        new_aflags = flags & accessor_flags;

        if (old_aflags && new_aflags && !(old_aflags & new_aflags)) {
            if ((pid->flags & PRIV_ID_FL_STATIC) != (flags & PRIV_ID_FL_STATIC)) {
                parse_error(rt, &tok->location, _("static flag of \"%s\" mismatch"),
                        rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
                parse_prev_define_note(rt, &pid->ast.location);
            } else {
                pid->flags |= new_aflags;
            }
        } else {
            parse_error(rt, &tok->location, _("\"%s\" is already defined"),
                    rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
            parse_prev_define_note(rt, &pid->ast.location);
        }

        rjs_value_set_gc_thing(rt, v, pid);
    } else {
        pid = ast_new(rt, v, RJS_AST_PrivId, &tok->location);

        pid->flags = flags;
        rjs_value_copy(rt, &pid->identifier, tok->value);
        pid->ve = value_entry_add(rt, &tok->location, &pid->identifier);

        hash_insert(rt, &env->priv_id_hash, &pid->identifier, &pid->he, phe);
        rjs_list_append(&env->priv_id_list, &pid->ast.ln);
    }

    return pid;
}

/*ADd a private identifier reference.*/
static RJS_AstPrivIdRef*
priv_id_ref_new (RJS_Runtime *rt, RJS_Token *tok, RJS_Value *v)
{
    RJS_Parser       *parser = rt->parser;
    RJS_AstClass     *clazz  = parser->class_stack;
    RJS_AstPrivIdRef *pid;

    pid = ast_new(rt, v, RJS_AST_PrivIdRef, &tok->location);

    pid->func = func_top(rt);

    rjs_value_copy(rt, &pid->identifier, tok->value);

    if (!(parser->flags & RJS_PARSE_FL_CLASS)) {
        parse_error(rt, &tok->location, _("private identifier only can be used in class"));
    } else if (clazz) {
        rjs_list_append(&clazz->priv_id_ref_list, &pid->ast.ln);
    } else {
        rjs_list_append(&parser->priv_id_ref_list, &pid->ast.ln);
    }

    return pid;
}

#endif /*ENABLE_PRIV_NAME*/

/*Parse the dot member.*/
static RJS_Result
parse_dot_member_expr (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vb)
{
    RJS_Result         r    = RJS_ERR;
    RJS_AstFunc       *func = func_top(rt);
    RJS_Token         *tok;
    RJS_Ast           *ast;
    RJS_AstBinaryExpr *be;

    ast = ast_get(rt, vb);
    be  = ast_new(rt, ve, RJS_AST_MemberExpr, &ast->location);

    rjs_value_copy(rt, &be->operand1, vb);

    tok = get_token(rt);

    if (tok->type == RJS_TOKEN_IDENTIFIER) {
        prop_ref_new(rt, &be->operand2, &tok->location, func, tok->value);
#if ENABLE_PRIV_NAME
    } else if (tok->type == RJS_TOKEN_PRIVATE_IDENTIFIER) {
        be->ast.type = RJS_AST_PrivMemberExpr;

        priv_id_ref_new(rt, tok, &be->operand2);
#endif /*ENABLE_PRIV_NAME*/
    } else {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
        r = RJS_ERR;
        goto end;
    }

    loc_update_last_token(rt, &be->ast.location);

    r = RJS_OK;
end:
    return r;
}

/*Parse the bracket member.*/
static RJS_Result
parse_bracket_member_expr (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vb)
{
    RJS_Result         r = RJS_ERR;
    RJS_Ast           *ast;
    RJS_AstBinaryExpr *be;

    ast = ast_get(rt, vb);
    be  = ast_new(rt, ve, RJS_AST_MemberExpr, &ast->location);

    rjs_value_copy(rt, &be->operand1, vb);

    if ((r = parse_expr_in(rt, &be->operand2)) == RJS_ERR)
        goto end;

    if ((r = get_token_expect(rt, RJS_TOKEN_rbracket)) == RJS_ERR)
        goto end;

    loc_update_last_token(rt, &be->ast.location);

    r = RJS_OK;
end:
    return r;
}

/*Parse the conditional expression.*/
static RJS_Result
parse_cond_expr (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vc)
{
    RJS_Result       r   = RJS_ERR;
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *le  = rjs_value_stack_push(rt);
    RJS_Value       *re  = rjs_value_stack_push(rt);
    int              b   = -1;
    RJS_Ast         *ast;
    RJS_AstCondExpr *ce;

    if ((r = parse_expr_in_prio(rt, PRIO_ASSI, le)) == RJS_ERR)
        goto end;

    if ((r = get_token_expect(rt, RJS_TOKEN_colon)) == RJS_ERR)
        goto end;

    if ((r = parse_expr_prio(rt, PRIO_ASSI, re)) == RJS_ERR)
        goto end;

    ast = ast_get(rt, vc);

    if (ast->type == RJS_AST_ValueExpr) {
        RJS_AstValueExpr *v = (RJS_AstValueExpr*)ast;

        b = rjs_to_boolean(rt, &v->ve->value);
    } else if (ast->type == RJS_AST_True) {
        b = RJS_TRUE;
    } else if ((ast->type == RJS_AST_False) || (ast->type == RJS_AST_Null)) {
        b = RJS_FALSE;
    }

    if (b == RJS_TRUE) {
        rjs_value_copy(rt, ve, le);
    } else if (b == RJS_FALSE) {
        rjs_value_copy(rt, ve, re);
    } else {
        ce  = ast_new(rt, ve, RJS_AST_CondExpr, &ast->location);
        rjs_value_copy(rt, &ce->cond, vc);
        rjs_value_copy(rt, &ce->true_value, le);
        rjs_value_copy(rt, &ce->false_value, re);
        loc_update_last_ast(rt, &ce->ast.location, &ce->false_value);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Convert the expression to left hand expression.*/
static RJS_Result
expr_to_lh (RJS_Runtime *rt, RJS_Value *vi, RJS_Value *vo)
{
    RJS_Result  r;
    RJS_Ast    *ast;

    check_lh_expr(rt, vi);

    ast = ast_get(rt, vi);

    switch (ast->type) {
    case RJS_AST_Array: {
        RJS_AstList *l = (RJS_AstList*)ast;

        r = array_to_binding(rt, l, vo, RJS_FALSE);
        break;
    }
    case RJS_AST_Object: {
        RJS_AstList *l = (RJS_AstList*)ast;

        r = object_to_binding(rt, l, vo, RJS_FALSE);
        break;
    }
    case RJS_AST_ParenthesesExpr: {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        r = expr_to_lh(rt, &ue->operand, vo);
        break;
    }
    default:
        rjs_value_copy(rt, vo, vi);
        r = RJS_OK;
        break;
    }

    return r;
}

/*Parse assignment expression.*/
static RJS_Result
parse_assi_expr (RJS_Runtime *rt, RJS_AstType type, RJS_Value *ve, RJS_Value *vl)
{
    RJS_Result         r;
    RJS_Ast           *ast;
    RJS_AstBinaryExpr *be;
    size_t             top = rjs_value_stack_save(rt);
    RJS_Value         *tmp = rjs_value_stack_push(rt);
    RJS_AstId         *id  = NULL;

    if (type == RJS_AST_AssiExpr) {
        /* x = y */
        RJS_Ast *left = ast_get(rt, vl);

        if ((left->type != RJS_AST_Object) && (left->type != RJS_AST_Array)) {
            check_simple_assi_target(rt, vl);
            if (left->type == RJS_AST_Id)
                id = (RJS_AstId*)left;
        }

        r = expr_to_lh(rt, vl, tmp);
        if (r == RJS_OK)
            rjs_value_copy(rt, vl, tmp);
    } else {
        /* x += y */
        check_simple_assi_target(rt, vl);

        if ((type == RJS_AST_AndAssiExpr)
                || (type == RJS_AST_OrAssiExpr)
                || (type == RJS_AST_QuesAssiExpr)) {
            RJS_Ast *left = ast_get(rt, vl);

            if (left->type == RJS_AST_Id)
                id = (RJS_AstId*)left;
        }
    }

    ast = ast_get(rt, vl);
    be  = ast_new(rt, ve, type, &ast->location);

    rjs_value_copy(rt, &be->operand1, vl);

    if ((r = parse_expr_prio(rt, PRIO_ASSI, &be->operand2)) == RJS_ERR)
        goto end;

    loc_update_last_ast(rt, &be->ast.location, &be->operand2);

    /*Set function name.*/
    if (id) {
        ast = ast_get(rt, &be->operand2);
        func_set_name(rt, ast, &id->ast.location, &id->identifier->value);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_ARROW_FUNC

/*Parse arrow function.*/
static RJS_Result
parse_arrow_function (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vl)
{
    RJS_Parser     *parser    = rt->parser;
    int             old_flags = parser->flags;
    size_t          top       = rjs_value_stack_save(rt);
    RJS_Value      *tmp1      = rjs_value_stack_push(rt);
    RJS_Value      *tmp2      = rjs_value_stack_push(rt);
    RJS_Bool        il_pushed = RJS_FALSE;
    RJS_Token      *tok;
    RJS_Result      r;
    RJS_Ast        *ast;
    RJS_AstFunc    *func;
    RJS_AstFuncRef *fr;

    ast  = ast_get(rt, vl);
    func = func_push(rt, &ast->location);
    func->flags |= RJS_AST_FUNC_FL_ARROW|RJS_AST_FUNC_FL_EXPR;

    if ((ast->type == RJS_AST_ArrowParams)
            || (ast->type == RJS_AST_AsyncArrowParams)) {
        RJS_AstArrowParams *ap = (RJS_AstArrowParams*)ast;

#if ENABLE_ASYNC
        if (ast->type == RJS_AST_AsyncArrowParams)
            func->flags   |= RJS_AST_FUNC_FL_ASYNC;
#endif /*ENABLE_ASYNC*/

        no_strict_list_restore(rt, &ap->no_strict_list);
        il_pushed = RJS_TRUE;

        rjs_list_join(&func->param_list, &ap->param_list);
        rjs_list_init(&ap->param_list);
    } else {
        parse_error(rt, &ast->location, _("expect parameters before `=>\'"));
        r = RJS_ERR;
        goto end;
    }

    /*Create the function expression.*/
    fr = ast_new(rt, ve, RJS_AST_FuncExpr, &ast->location);
    fr->func        = func;
    fr->decl        = NULL;
    fr->binding_ref = NULL;

    parser->flags &= ~(RJS_PARSE_FL_YIELD
            |RJS_PARSE_FL_AWAIT);

    if (ast->type == RJS_AST_AsyncArrowParams)
        parser->flags |= RJS_PARSE_FL_AWAIT;

    tok = get_token(rt);
    unget_token(rt);

    if (tok->type == RJS_TOKEN_lbrace) {
        if ((r = parse_func_body(rt)) == RJS_ERR)
            goto end;
    } else {
        RJS_Ast         *ast_expr;
        RJS_AstExprStmt *ret_stmt;

        func_body(rt);

        if ((r = parse_expr_prio(rt, PRIO_ASSI, tmp1)) == RJS_ERR)
            goto end;

        check_expr(rt, tmp1);

        ast_expr = ast_get(rt, tmp1);
        ret_stmt = ast_new(rt, tmp2, RJS_AST_ReturnStmt, &ast_expr->location);

        rjs_value_copy(rt, &ret_stmt->expr, tmp1);

        rjs_list_append(&func->stmt_list, &ret_stmt->ast.ln);

        loc_update_last(&func->ast.location, &ast_expr->location);
    }

    loc_update_last(&fr->ast.location, &func->ast.location);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    parser->flags = old_flags;

    if (il_pushed)
        no_strict_list_pop(rt, RJS_TRUE, RJS_FALSE);

    func_pop(rt);
    return r;
}

#endif /*ENABLE_ARROW_FUNC*/

/*Parse comma expression.*/
static RJS_Result
parse_comma_expr (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vl)
{
    RJS_Result   r;
    RJS_Ast     *ast_left;
    RJS_AstList *ce;
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *tmp = rjs_value_stack_push(rt);

    ast_left = ast_get(rt, vl);
    if (ast_left->type == RJS_AST_CommaExpr) {
        ce = (RJS_AstList*)ast_left;

        rjs_value_copy(rt, ve, vl);
    } else {
        ce = ast_new(rt, ve, RJS_AST_CommaExpr, &ast_left->location);

        ast_list_append(rt, &ce->list, vl);
    }

    if ((r = parse_expr_prio(rt, PRIO_ASSI, tmp)) == RJS_ERR)
        goto end;

    ast_list_append(rt, &ce->list, tmp);
    loc_update_last_ast(rt, &ce->ast.location, tmp);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse call expression.*/
static RJS_Result
parse_call_expr (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vf, int prio)
{
    RJS_Result   r;
    RJS_AstCall *ce;
    RJS_Ast     *ast;
    RJS_Token   *tok;
    RJS_Parser  *parser    = rt->parser;
    size_t       top       = rjs_value_stack_save(rt);
#if ENABLE_ARROW_FUNC
    RJS_Value   *tmp       = rjs_value_stack_push(rt);
    RJS_Token   *ntok;
#endif /*ENABLE_ARROW_FUNC*/
    RJS_Bool     is_arrow  = RJS_FALSE;
    RJS_Bool     same_line = RJS_FALSE;

    tok = get_token(rt);

    if (tok->location.first_line == parser->last_line)
        same_line = RJS_TRUE;

    unget_token(rt);

    no_strict_list_push(rt);
    contains_list_push(rt);

    ast = ast_get(rt, vf);
    ce  = ast_new(rt, ve, RJS_AST_CallExpr, &ast->location);

    rjs_value_copy(rt, &ce->func, vf);

    if ((r = parse_arguments(rt, &ce->arg_list)) == RJS_ERR)
        goto end;

#if ENABLE_ARROW_FUNC
    /*Check if the next token is "=>".*/
    ntok = next_token_div(rt);
    if (same_line
            && (prio <= PRIO_ASSI)
            && (ntok->type == RJS_TOKEN_eq_gt)
            && (ntok->location.first_line == parser->last_line)) {
        ast = ast_get(rt, &ce->func);

        if (ast->type == RJS_AST_Id) {
            RJS_AstId *ir = (RJS_AstId*)ast;

            /*async arrow function.*/
            if (!(ir->flags & RJS_TOKEN_FL_ESCAPE)
                    && rjs_string_equal(rt, &ir->identifier->value, rjs_s_async(rt))) {
                RJS_AstArrowParams *pl;

                if ((r = args_to_params(rt, &ce->ast.location, &ce->arg_list, RJS_AST_AsyncArrowParams, tmp)) == RJS_ERR)
                    goto end;

                contains_list_check(rt, CONTAINS_FL_AWAIT_EXPR|CONTAINS_FL_YIELD_EXPR|CONTAINS_FL_AWAIT);

                rjs_value_copy(rt, ve, tmp);

                pl = ast_get(rt, tmp);
                no_strict_list_save(rt, &pl->no_strict_list);

                is_arrow = RJS_TRUE;
            }
        }
    }
#endif /*ENABLE_ARROW_FUNC*/

    r = RJS_OK;
end:
    if (!is_arrow)
        no_strict_list_pop(rt, RJS_FALSE, RJS_TRUE);

    contains_list_pop(rt, RJS_TRUE);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse identifier.*/
static RJS_Result
parse_identifier (RJS_Runtime *rt, RJS_Value *ve, int prio)
{
    RJS_Token          *tok, *ntok;
    RJS_AstArrowParams *pl;
    RJS_AstBindingElem *be;
    size_t              top = rjs_value_stack_save(rt);
    RJS_Value          *tmp = rjs_value_stack_push(rt);

    tok = get_token(rt);

    ntok = next_token_div(rt);
    if ((prio <= PRIO_ASSI) && (ntok->type == RJS_TOKEN_eq_gt)) {
        no_strict_list_push(rt);
        check_binding_identifier(rt, &tok->location, tok->value);

        pl = ast_new(rt, ve, RJS_AST_ArrowParams, &tok->location);
        be = ast_new(rt, tmp, RJS_AST_BindingElem, &tok->location);
        id_new(rt, &be->binding, &tok->location, tok->value);

        ast_list_append(rt, &pl->param_list, tmp);

        no_strict_list_save(rt, &pl->no_strict_list);
    } else {
        RJS_AstId *id;

        check_identifier_reference(rt, &tok->location, tok->value);

        id = id_new(rt, ve, &tok->location, tok->value);

        id->flags = tok->flags;
    }

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

#if ENABLE_ASYNC && ENABLE_ARROW_FUNC

/*Parse async arrow function's parameters.*/
static RJS_Result
parse_async_arrow_params (RJS_Runtime *rt, RJS_Value *ve, int prio)
{
    RJS_Result          r;
    RJS_Token          *tok, *ntok;
    RJS_AstArrowParams *pl;
    RJS_AstBindingElem *be;
    RJS_Location        loc;
    RJS_Parser         *parser    = rt->parser;
    int                 old_flags = parser->flags;
    size_t              top       = rjs_value_stack_save(rt);
    RJS_Value          *tmp       = rjs_value_stack_push(rt);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_async)) == RJS_ERR)
        goto end;
    tok = curr_token(rt);
    loc = tok->location;

    parser->flags |= RJS_PARSE_FL_AWAIT;

    tok = get_token_div(rt);
    if (tok->location.first_line != parser->last_line) {
        parse_error(rt, &tok->location, _("line terminator cannot be after `async\'"));
        r = RJS_ERR;
        goto end;
    }

    if (!is_binding_identifier(rt, tok->type, tok->flags)) {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
        r = RJS_ERR;
        goto end;
    }

    ntok = next_token_div(rt);
    if ((prio <= PRIO_ASSI) && (ntok->type == RJS_TOKEN_eq_gt)) {
        no_strict_list_push(rt);
        check_binding_identifier(rt, &tok->location, tok->value);

        pl = ast_new(rt, ve, RJS_AST_AsyncArrowParams, &loc);
        be = ast_new(rt, tmp, RJS_AST_BindingElem, &tok->location);
        id_new(rt, &be->binding, &tok->location, tok->value);

        ast_list_append(rt, &pl->param_list, tmp);

        no_strict_list_save(rt, &pl->no_strict_list);
    } else {
        /*The next token is not '=>', just return identifier "async"*/
        unget_token(rt);

        id_new(rt, ve, &loc, rjs_s_async(rt));
    }

    r = RJS_OK;
end:
    parser->flags = old_flags;
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

#endif /*ENABLE_ASYNC*/

/*Parse optional expression.*/
static RJS_Result
parse_opt_expr (RJS_Runtime *rt, RJS_Value *ve, RJS_Value *vb, int prio)
{
    RJS_Token         *tok;
    RJS_Result         r;
    RJS_AstUnaryExpr  *opt_expr, *base_expr;
    RJS_AstBinaryExpr *bin;
    RJS_AstFunc       *func = func_top(rt);
    size_t             top  = rjs_value_stack_save(rt);
    RJS_Value         *left = rjs_value_stack_push(rt);
    RJS_Value         *res  = rjs_value_stack_push(rt);

    tok = get_token_div(rt);

    base_expr = ast_new(rt, left, RJS_AST_OptionalBase, &tok->location);
    rjs_value_copy(rt, &base_expr->operand, vb);

    opt_expr = ast_new(rt, ve, RJS_AST_OptionalExpr, &tok->location);

    while (1) {
        switch (tok->type) {
        case RJS_TOKEN_lparenthese: {
            unget_token(rt);
            if ((r = parse_call_expr(rt, res, left, prio)) == RJS_ERR)
                goto end;
            break;
        }
        case RJS_TOKEN_lbracket:
            if ((r = parse_bracket_member_expr(rt, res, left)) == RJS_ERR)
                goto end;
            break;
#if ENABLE_PRIV_NAME
        case RJS_TOKEN_PRIVATE_IDENTIFIER:
            bin = ast_new(rt, res, RJS_AST_PrivMemberExpr, &tok->location);

            rjs_value_copy(rt, &bin->operand1, left);
            priv_id_ref_new(rt, tok, &bin->operand2);

            loc_update_last_token(rt, &bin->ast.location);
            break;
#endif /*ENABLE_PRIV_NAME*/
        case RJS_TOKEN_TEMPLATE:
        case RJS_TOKEN_TEMPLATE_HEAD: {
            RJS_AstTemplate *templ;

            unget_token(rt);
            if ((r = parse_template_literal(rt, res, RJS_TRUE)) == RJS_ERR)
                goto end;

            templ = (RJS_AstTemplate*)ast_get(rt, res);

            parse_error(rt, &templ->ast.location, _("template cannot be used in optional expression"));

            rjs_value_copy(rt, &templ->func, left);
            break;
        }
        default:
            if (tok->type == RJS_TOKEN_IDENTIFIER) {
                bin = ast_new(rt, res, RJS_AST_MemberExpr, &tok->location);

                rjs_value_copy(rt, &bin->operand1, left);
                prop_ref_new(rt, &bin->operand2, &tok->location, func, tok->value);

                loc_update_last_token(rt, &bin->ast.location);
            } else {
                parse_unexpect_error(rt, &tok->location, _("identifier, `(\' or `[\'"));
                r = RJS_ERR;
                goto end;
            }
            break;
        }

        rjs_value_copy(rt, left, res);

        tok = get_token_div(rt);
        switch (tok->type) {
        case RJS_TOKEN_dot:
            tok = get_token(rt);
            break;
        case RJS_TOKEN_lparenthese:
        case RJS_TOKEN_lbracket:
        case RJS_TOKEN_TEMPLATE:
        case RJS_TOKEN_TEMPLATE_HEAD:
            break;
        default:
            unget_token(rt);
            goto opt_end;
        }
    }
opt_end:
    rjs_value_copy(rt, &opt_expr->operand, left);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the expression with pririty.*/
static RJS_Result
parse_expr_prio (RJS_Runtime *rt, Priority prio, RJS_Value *ve)
{
    RJS_Token    *tok;
    RJS_Result    r         = RJS_ERR;
    int           last_prio = PRIO_HIGHEST;
    RJS_Parser   *parser    = rt->parser;
    size_t        top       = rjs_value_stack_save(rt);
    RJS_Value    *tmp1      = rjs_value_stack_push(rt);
    RJS_Value    *tmp2      = rjs_value_stack_push(rt);

    tok = get_token(rt);
    switch (tok->type) {
    case RJS_TOKEN_NUMBER: {
        RJS_Number n = rjs_value_get_number(rt, tok->value);

        if (n == 0)
            ast_new(rt, tmp1, RJS_AST_Zero, &tok->location);
        else if (n == 1)
            ast_new(rt, tmp1, RJS_AST_One, &tok->location);
        else
            value_expr_new(rt, tmp1, &tok->location, tok->value);
        break;
    }
    case RJS_TOKEN_STRING: {
        RJS_AstValueExpr *ve;

        ve = value_expr_new(rt, tmp1, &tok->location, tok->value);
        ve->flags = tok->flags;

        if (tok->flags & RJS_TOKEN_FL_LEGACY_ESCAPE)
            no_strict_list_add(rt, RJS_AST_StrRef, tok->value);
        break;
    }
    case RJS_TOKEN_REGEXP: {
        RJS_AstValueExpr *ve;

        ve = value_expr_new(rt, tmp1, &tok->location, tok->value);
        ve->flags = tok->flags;
        break;
    }
#if ENABLE_PRIV_NAME
    case RJS_TOKEN_PRIVATE_IDENTIFIER: {
        RJS_Token *ntok;

        priv_id_ref_new(rt, tok, tmp1);

        ntok = next_token_div(rt);

        if (!(parser->flags & RJS_PARSE_FL_IN)
                    || !token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_in)) {
            parse_unexpect_error(rt, &tok->location, _("`in\'"));
        } else if (prio > PRIO_REL) {
            parse_error(rt, &tok->location, _("unexpected relational expression"));
        }
        break;
    }
#endif /*ENABLE_PRIV_NAME*/
    case RJS_TOKEN_TEMPLATE:
    case RJS_TOKEN_TEMPLATE_HEAD:
        unget_token(rt);
        if ((r = parse_template_literal(rt, tmp1, RJS_FALSE)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_lparenthese:
        unget_token(rt);
        if ((r = parse_parentheses_or_params(rt, tmp1, prio)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_lbracket:
        unget_token(rt);
        if ((r = parse_array_literal(rt, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_lbrace:
        unget_token(rt);
        if ((r = parse_object_literal(rt, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_plus:
        last_prio = PRIO_UNARY;

        if (last_prio < prio)
            parse_error(rt, &tok->location, _("unexpected unary expression"));

        if ((r = parse_unary_expr(rt, RJS_AST_ToNumExpr, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_minus:
        last_prio = PRIO_UNARY;

        if (last_prio < prio)
            parse_error(rt, &tok->location, _("unexpected unary expression"));

        if ((r = parse_unary_expr(rt, RJS_AST_NegExpr, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_tilde:
        last_prio = PRIO_UNARY;

        if (last_prio < prio)
            parse_error(rt, &tok->location, _("unexpected unary expression"));

        if ((r = parse_unary_expr(rt, RJS_AST_RevExpr, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_exclamation:
        last_prio = PRIO_UNARY;

        if (last_prio < prio)
            parse_error(rt, &tok->location, _("unexpected unary expression"));

        if ((r = parse_unary_expr(rt, RJS_AST_NotExpr, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_plus_plus:
        last_prio = PRIO_UPDATE;

        if (last_prio < prio)
            parse_error(rt, &tok->location, _("unexpected update expression"));

        if ((r = parse_unary_expr(rt, RJS_AST_PreIncExpr, tmp1)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_minus_minus:
        last_prio = PRIO_UPDATE;

        if (last_prio < prio)
            parse_error(rt, &tok->location, _("unexpected update expression"));

        if ((r = parse_unary_expr(rt, RJS_AST_PreDecExpr, tmp1)) == RJS_ERR)
            goto end;
        break;
    default:
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_delete)) {
            RJS_AstUnaryExpr *ue;

            last_prio = PRIO_UNARY;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected unary expression"));

            if ((r = parse_unary_expr(rt, RJS_AST_DelExpr, tmp1)) == RJS_ERR)
                goto end;

            ue = ast_get(rt, tmp1);
            check_delete_operand(rt, &ue->operand);
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_void)) {
            last_prio = PRIO_UNARY;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected unary expression"));

            if ((r = parse_unary_expr(rt, RJS_AST_VoidExpr, tmp1)) == RJS_ERR)
                goto end;
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_typeof)) {
            last_prio = PRIO_UNARY;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected unary expression"));

            if ((r = parse_unary_expr(rt, RJS_AST_TypeOfExpr, tmp1)) == RJS_ERR)
                goto end;
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_this)) {
            ast_new(rt, tmp1, RJS_AST_This, &tok->location);
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_null)) {
            ast_new(rt, tmp1, RJS_AST_Null, &tok->location);
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_true)) {
            ast_new(rt, tmp1, RJS_AST_True, &tok->location);
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_false)) {
            ast_new(rt, tmp1, RJS_AST_False, &tok->location);
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_super)) {
            last_prio = PRIO_LH;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected left hand expression"));

            unget_token(rt);
            if ((r = parse_super_expr(rt, tmp1)) == RJS_ERR)
                goto end;
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_new)) {
            last_prio = PRIO_NEW;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected new expression"));

            unget_token(rt);
            if ((r = parse_new_expr(rt, tmp1)) == RJS_ERR)
                goto end;
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_function)) {
            unget_token(rt);
            if ((r = parse_hoistable_decl(rt, RJS_TRUE, tmp1)) == RJS_ERR)
                goto end;
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_class)) {
            unget_token(rt);
            if ((r = parse_class_decl(rt, RJS_TRUE, tmp1)) == RJS_ERR)
                goto end;
        }
#if ENABLE_ASYNC
        else if ((parser->flags & RJS_PARSE_FL_AWAIT)
                && token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_await)) {
            RJS_AstFunc *func = func_top(rt);
            RJS_Ast     *ae;

            last_prio = PRIO_UNARY;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected unary expression"));

            if (func->flags & RJS_AST_FUNC_FL_MODULE)
                func->flags |= RJS_AST_FUNC_FL_ASYNC;

            if ((r = parse_unary_expr(rt, RJS_AST_AwaitExpr, tmp1)) == RJS_ERR)
                goto end;

            /*Add yield expression to contains list.*/
            ae = ast_get(rt, tmp1);
            if (ae)
                contains_list_add(rt, RJS_AST_AwaitExprRef, &ae->location);
        } else if (is_async_function(rt, tok->type, tok->flags)) {
            unget_token(rt);
            if ((r = parse_hoistable_decl(rt, RJS_TRUE, tmp1)) == RJS_ERR)
                goto end;
        }
#if ENABLE_ARROW_FUNC
        else if (is_async_arrow(rt, tok->type, tok->flags)) {
            unget_token(rt);
            if ((r = parse_async_arrow_params(rt, tmp1, prio)) == RJS_ERR)
                goto end;
        }
#endif /*ENABLE_ARROW_FUNC*/
#endif /*ENABLE_ASYNC*/
#if ENABLE_MODULE
        else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_import)) {
            RJS_Ast *ast;

            unget_token(rt);
            if ((r = parse_import_expr(rt, tmp1)) == RJS_ERR)
                goto end;

            ast = ast_get(rt, tmp1);

            if (ast->type == RJS_AST_ImportExpr) {
                last_prio = PRIO_LH;

                if (last_prio < prio)
                    parse_error(rt, &tok->location, _("unexpected left hand expression"));
            }
        }
#endif /*ENABLE_MODULE*/
#if ENABLE_GENERATOR
        else if ((parser->flags & RJS_PARSE_FL_YIELD)
                && token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_yield)
                && (prio <= PRIO_ASSI)) {
            last_prio = PRIO_ASSI;

            if (last_prio < prio)
                parse_error(rt, &tok->location, _("unexpected assignment expression"));

            unget_token(rt);
            if ((r = parse_yield_expr(rt, tmp1)) == RJS_ERR)
                goto end;
        }
#endif /*ENABLE_GENERATOR*/
        else if (is_identifier_reference(rt, tok->type, tok->flags)) {
            unget_token(rt);
            if ((r = parse_identifier(rt, tmp1, prio)) == RJS_ERR)
                goto end;
        }
#if ENABLE_PRIV_NAME
        else if ((parser->flags & RJS_PARSE_FL_IN) && (tok->type == RJS_TOKEN_PRIVATE_IDENTIFIER)) {
            RJS_Token *ntok = next_token_div(rt);

            if (!token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_in)) {
                parse_unexpect_token_error(rt, &ntok->location, RJS_TOKEN_IDENTIFIER, RJS_IDENTIFIER_in);
                r = RJS_ERR;
                goto end;
            }

            priv_id_ref_new(rt, tok, tmp1);
        }
#endif /*ENABLE_PRIV_NAME*/
        else {
            parse_unexpect_error(rt, &tok->location, _("expression"));
            r = RJS_ERR;
            goto end;
        }
        break;
    }

    while (1) {
        tok = get_token_div(rt);
        switch (tok->type) {
        case RJS_TOKEN_dot:
            if (last_prio < PRIO_LH)
                goto ok;

            last_prio = PRIO_LH;

            if ((r = parse_dot_member_expr(rt, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_lbracket:
            if (last_prio < PRIO_LH)
                goto ok;

            last_prio = PRIO_LH;

            if ((r = parse_bracket_member_expr(rt, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_lparenthese:
            if ((last_prio < PRIO_LH) || (prio > PRIO_LH))
                goto ok;

            last_prio = PRIO_LH;

            unget_token(rt);
            if ((r = parse_call_expr(rt, tmp2, tmp1, prio)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_TEMPLATE:
        case RJS_TOKEN_TEMPLATE_HEAD: {
            RJS_AstTemplate *templ;

            if (last_prio < PRIO_LH)
                goto ok;

            last_prio = PRIO_LH;

            unget_token(rt);
            if ((r = parse_template_literal(rt, tmp2, RJS_TRUE)) == RJS_ERR)
                goto end;

            templ = ast_get(rt, tmp2);
            rjs_value_copy(rt, &templ->func, tmp1);
            break;
        }
        case RJS_TOKEN_ques_dot: {
            if (last_prio < PRIO_LH)
                goto ok;

            last_prio = PRIO_LH;

            if ((r = parse_opt_expr(rt, tmp2, tmp1, prio)) == RJS_ERR)
                goto end;
            break;
        }
        case RJS_TOKEN_plus:
            if ((last_prio < PRIO_ADD) || (prio > PRIO_ADD))
                goto ok;

            last_prio = PRIO_ADD;

            if ((r = parse_binary_expr(rt, PRIO_MUL, RJS_AST_AddExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_minus:
            if ((last_prio < PRIO_ADD) || (prio > PRIO_ADD))
                goto ok;

            last_prio = PRIO_ADD;

            if ((r = parse_binary_expr(rt, PRIO_MUL, RJS_AST_SubExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_star:
            if ((last_prio < PRIO_MUL) || (prio > PRIO_MUL))
                goto ok;

            last_prio = PRIO_MUL;

            if ((r = parse_binary_expr(rt, PRIO_EXP, RJS_AST_MulExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_slash:
            if ((last_prio < PRIO_MUL) || (prio > PRIO_MUL))
                goto ok;

            last_prio = PRIO_MUL;

            if ((r = parse_binary_expr(rt, PRIO_EXP, RJS_AST_DivExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_percent:
            if ((last_prio < PRIO_MUL) || (prio > PRIO_MUL))
                goto ok;

            last_prio = PRIO_MUL;

            if ((r = parse_binary_expr(rt, PRIO_EXP, RJS_AST_ModExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_star_star:
            if ((last_prio < PRIO_UPDATE) || (prio > PRIO_EXP))
                goto ok;

            last_prio = PRIO_EXP;

            if ((r = parse_binary_expr(rt, PRIO_EXP, RJS_AST_ExpExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_lt:
            if ((last_prio < PRIO_REL) || (prio > PRIO_REL))
                goto ok;

            last_prio = PRIO_REL;

            if ((r = parse_binary_expr(rt, PRIO_SHIFT, RJS_AST_LtExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_gt:
            if ((last_prio < PRIO_REL) || (prio > PRIO_REL))
                goto ok;

            last_prio = PRIO_REL;

            if ((r = parse_binary_expr(rt, PRIO_SHIFT, RJS_AST_GtExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_lt_eq:
            if ((last_prio < PRIO_REL) || (prio > PRIO_REL))
                goto ok;

            last_prio = PRIO_REL;

            if ((r = parse_binary_expr(rt, PRIO_SHIFT, RJS_AST_LeExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_gt_eq:
            if ((last_prio < PRIO_REL) || (prio > PRIO_REL))
                goto ok;

            last_prio = PRIO_REL;

            if ((r = parse_binary_expr(rt, PRIO_SHIFT, RJS_AST_GeExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_eq_eq:
            if ((last_prio < PRIO_EQ) || (prio > PRIO_EQ))
                goto ok;

            last_prio = PRIO_EQ;

            if ((r = parse_binary_expr(rt, PRIO_REL, RJS_AST_EqExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_exclamation_eq:
            if ((last_prio < PRIO_EQ) || (prio > PRIO_EQ))
                goto ok;

            last_prio = PRIO_EQ;

            if ((r = parse_binary_expr(rt, PRIO_REL, RJS_AST_NeExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_eq_eq_eq:
            if ((last_prio < PRIO_EQ) || (prio > PRIO_EQ))
                goto ok;

            last_prio = PRIO_EQ;

            if ((r = parse_binary_expr(rt, PRIO_REL, RJS_AST_StrictEqExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_exclamation_eq_eq:
            if ((last_prio < PRIO_EQ) || (prio > PRIO_EQ))
                goto ok;

            last_prio = PRIO_EQ;

            if ((r = parse_binary_expr(rt, PRIO_REL, RJS_AST_StrictNeExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_lt_lt:
            if ((last_prio < PRIO_SHIFT) || (prio > PRIO_SHIFT))
                goto ok;

            last_prio = PRIO_SHIFT;

            if ((r = parse_binary_expr(rt, PRIO_ADD, RJS_AST_ShlExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_gt_gt:
            if ((last_prio < PRIO_SHIFT) || (prio > PRIO_SHIFT))
                goto ok;

            last_prio = PRIO_SHIFT;

            if ((r = parse_binary_expr(rt, PRIO_ADD, RJS_AST_ShrExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_gt_gt_gt:
            if ((last_prio < PRIO_SHIFT) || (prio > PRIO_SHIFT))
                goto ok;

            last_prio = PRIO_SHIFT;

            if ((r = parse_binary_expr(rt, PRIO_ADD, RJS_AST_UShrExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_amp:
            if ((last_prio < PRIO_BAND) || (prio > PRIO_BAND))
                goto ok;

            last_prio = PRIO_BAND;

            if ((r = parse_binary_expr(rt, PRIO_EQ, RJS_AST_BitAndExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_caret:
            if ((last_prio < PRIO_BXOR) || (prio > PRIO_BXOR))
                goto ok;

            last_prio = PRIO_BXOR;

            if ((r = parse_binary_expr(rt, PRIO_BAND, RJS_AST_BitXorExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_pipe:
            if ((last_prio < PRIO_BOR) || (prio > PRIO_BOR))
                goto ok;

            last_prio = PRIO_BOR;

            if ((r = parse_binary_expr(rt, PRIO_BXOR, RJS_AST_BitOrExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_amp_amp:
            if ((last_prio < PRIO_AND) || (prio > PRIO_AND))
                goto ok;

            last_prio = PRIO_AND;

            if ((r = parse_logic_and_expr(rt, PRIO_BOR, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_pipe_pipe:
            if ((last_prio < PRIO_OR) || (prio > PRIO_OR))
                goto ok;

            last_prio = PRIO_OR;

            if ((r = parse_logic_or_expr(rt, PRIO_AND, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_ques_ques:
            if (((last_prio < PRIO_BOR) && (last_prio != PRIO_QUES)) || (prio > PRIO_QUES))
                goto ok;

            last_prio = PRIO_QUES;

            if ((r = parse_ques_expr(rt, PRIO_BOR, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_ques:
            if ((last_prio < PRIO_QUES) || (prio > PRIO_COND))
                goto ok;

            last_prio = PRIO_COND;

            if ((r = parse_cond_expr(rt, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_AssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_plus_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_AddAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_minus_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_SubAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_star_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_MulAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_slash_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_DivAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_star_star_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_ExpAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_percent_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_ModAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_pipe_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_BitOrAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_amp_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_BitAndAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_caret_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_BitXorAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_lt_lt_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_ShlAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_gt_gt_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_ShrAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_gt_gt_gt_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_UShrAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_amp_amp_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_AndAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_pipe_pipe_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_OrAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_ques_ques_eq:
            if ((last_prio < PRIO_LH) || (prio > PRIO_ASSI))
                goto ok;

            last_prio = PRIO_ASSI;

            if ((r = parse_assi_expr(rt, RJS_AST_QuesAssiExpr, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        case RJS_TOKEN_comma:
            if (prio >= PRIO_COMMA)
                goto ok;

            last_prio = PRIO_COMMA;

            if ((r = parse_comma_expr(rt, tmp2, tmp1)) == RJS_ERR)
                goto end;
            break;
        default:
            if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_instanceof)) {
                if ((last_prio < PRIO_REL) || (prio > PRIO_REL))
                    goto ok;

                last_prio = PRIO_REL;

                if ((r = parse_binary_expr(rt, PRIO_SHIFT, RJS_AST_InstanceOfExpr, tmp2, tmp1)) == RJS_ERR)
                    goto end;
            } else if ((parser->flags & RJS_PARSE_FL_IN)
                    && token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_in)) {
                if ((last_prio < PRIO_REL) || (prio > PRIO_REL))
                    goto ok;

                last_prio = PRIO_REL;

                if ((r = parse_binary_expr(rt, PRIO_SHIFT, RJS_AST_InExpr, tmp2, tmp1)) == RJS_ERR)
                    goto end;
            } else if (tok->location.first_line == parser->last_line) {
                switch (tok->type) {
                case RJS_TOKEN_plus_plus:
                    if ((last_prio < PRIO_LH) || (prio > PRIO_UPDATE))
                        goto ok;

                    last_prio = PRIO_UPDATE;

                    if ((r = parse_update_expr(rt, RJS_AST_PostIncExpr, tmp2, tmp1)) == RJS_ERR)
                        goto end;
                    break;
                case RJS_TOKEN_minus_minus:
                    if ((last_prio < PRIO_LH) || (prio > PRIO_UPDATE))
                        goto ok;

                    last_prio = PRIO_UPDATE;

                    if ((r = parse_update_expr(rt, RJS_AST_PostDecExpr, tmp2, tmp1)) == RJS_ERR)
                        goto end;
                    break;
#if ENABLE_ARROW_FUNC
                case RJS_TOKEN_eq_gt:
                    if (prio > PRIO_ASSI)
                        goto ok;

                    last_prio = PRIO_ASSI;

                    if ((r = parse_arrow_function(rt, tmp2, tmp1)) == RJS_ERR)
                        goto end;
                    break;
#endif /*ENABLE_ARROW_FUNC*/
                default:
                    goto ok;
                }
            } else {
                goto ok;
            }
            break;
        }

        rjs_value_copy(rt, tmp1, tmp2);
    }

ok:
    rjs_value_copy(rt, ve, tmp1);
    unget_token(rt);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the expression.*/
static RJS_Result
parse_expr (RJS_Runtime *rt, RJS_Value *ve)
{
    return parse_expr_prio(rt, PRIO_LOWEST, ve);
}

/*Parse the expression with in flag and priority.*/
static RJS_Result
parse_expr_in_prio (RJS_Runtime *rt, Priority prio, RJS_Value *ve)
{
    RJS_Result  r;
    RJS_Parser *parser    = rt->parser;
    int         old_flags = parser->flags;

    parser->flags |= RJS_PARSE_FL_IN;
    r = parse_expr_prio(rt, prio, ve);
    parser->flags = old_flags;

    return r;
}

/*Parse the expression with in flag.*/
static RJS_Result
parse_expr_in (RJS_Runtime *rt, RJS_Value *ve)
{
    return parse_expr_in_prio(rt, PRIO_LOWEST, ve);
}

/*Parse ( expression ) */
static RJS_Result
parse_parentheses_expr (RJS_Runtime *rt, RJS_Value *ve)
{
    RJS_Result    r = RJS_ERR;
    RJS_Token    *tok;
    RJS_Location  loc;
    RJS_Ast      *ast;

    if ((r = get_token_expect(rt, RJS_TOKEN_lparenthese)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    loc = tok->location;

    if ((r = parse_expr_in(rt, ve)) == RJS_ERR)
        goto end;

    check_expr(rt, ve);

    if ((r = get_token_expect(rt, RJS_TOKEN_rparenthese)) == RJS_ERR)
        goto end;

    ast = ast_get(rt, ve);

    ast->location.first_line   = loc.first_line;
    ast->location.first_column = loc.first_column;

    loc_update_last_token(rt, &ast->location);

    r = RJS_OK;
end:
    return r;
}

/*Parse array binding pattern.*/
static RJS_Result
parse_array_binding_pattern (RJS_Runtime *rt, RJS_Value *vb)
{
    RJS_Token   *tok;
    RJS_AstList *ab;
    RJS_Result   r        = RJS_ERR;
    size_t       top      = rjs_value_stack_save(rt);
    RJS_Value   *tmp      = rjs_value_stack_push(rt);
    RJS_Bool     has_rest = RJS_FALSE;

    tok = curr_token(rt);
    ab  = ast_new(rt, vb, RJS_AST_ArrayBinding, &tok->location);

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbracket)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbracket, 0);
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_comma) {
            RJS_Ast *ast;

            ast = ast_new(rt, tmp, RJS_AST_Elision, &tok->location);
            rjs_list_append(&ab->list, &ast->ln);

            continue;
        } else if (tok->type == RJS_TOKEN_dot_dot_dot) {
            RJS_AstRest *rest;

            has_rest = RJS_TRUE;

            rest = ast_new(rt, tmp, RJS_AST_Rest, &tok->location);

            r = parse_binding(rt, &rest->binding);
            if (r == RJS_OK) {
                loc_update_last_ast(rt, &rest->ast.location, &rest->binding);
                rjs_list_append(&ab->list, &rest->ast.ln);
            }
        } else {
            RJS_AstBindingElem *be;

            unget_token(rt);

            be = ast_new(rt, tmp, RJS_AST_BindingElem, &tok->location);

            r = parse_binding_element(rt, &be->binding, &be->init);
            if (r == RJS_OK) {
                if (!loc_update_last_ast(rt, &be->ast.location, &be->init))
                    loc_update_last_ast(rt, &be->ast.location, &be->binding);

                rjs_list_append(&ab->list, &be->ast.ln);
            }
        }

        if (r == RJS_ERR) {
            recover_array(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbracket)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `]\'"));
            recover_array(rt);
        } else if (has_rest) {
            parse_error(rt, &tok->location, _("`...\' must be the last element"));
        }
    }

    loc_update_last_token(rt, &ab->ast.location);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse object binding pattern.*/
static RJS_Result
parse_object_binding_pattern (RJS_Runtime *rt, RJS_Value *vb)
{
    RJS_Token   *tok, *ntok;
    RJS_AstList *ob;
    RJS_Result   r   = RJS_ERR;
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *tmp = rjs_value_stack_push(rt);

    tok = curr_token(rt);
    ob  = ast_new(rt, vb, RJS_AST_ObjectBinding, &tok->location);

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            r = RJS_ERR;
            goto end;
        }

        r = RJS_OK;
        if (tok->type == RJS_TOKEN_dot_dot_dot) {
            RJS_AstRest *rest;

            rest = ast_new(rt, tmp, RJS_AST_Rest, &tok->location);

            tok = get_token(rt);
            if (!is_binding_identifier(rt, tok->type, tok->flags)) {
                parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                r = RJS_ERR;
            } else {
                check_binding_identifier(rt, &tok->location, tok->value);

                id_new(rt, &rest->binding, &tok->location, tok->value);

                rjs_list_append(&ob->list, &rest->ast.ln);
            }
        } else {
            RJS_Bool            is_prop = RJS_TRUE;
            RJS_AstBindingProp *prop;

            prop = ast_new(rt, tmp, RJS_AST_BindingProp, &tok->location);

            switch (tok->type) {
            case RJS_TOKEN_STRING:
            case RJS_TOKEN_NUMBER:
            case RJS_TOKEN_lbracket:
                unget_token(rt);
                r = parse_property_name(rt, &prop->name);
                break;
            default:
                if (tok->type == RJS_TOKEN_IDENTIFIER) {
                    ntok = next_token_div(rt);

                    if (ntok->type != RJS_TOKEN_colon)
                        is_prop = RJS_FALSE;

                    value_expr_new(rt, &prop->name, &tok->location, tok->value);
                } else {
                    parse_unexpect_error(rt, &tok->location, _("property name"));
                    r = RJS_ERR;
                }
                break;
            }

            if (r == RJS_OK) {
                if (is_prop) {
                    r = get_token_expect(rt, RJS_TOKEN_colon);

                    if (r == RJS_OK)
                        r = parse_binding_element(rt, &prop->binding, &prop->init);
                } else {
                    if (!is_binding_identifier(rt, tok->type, tok->flags)) {
                        parse_unexpect_error(rt, &tok->location, _("binding identifier"));
                        r = RJS_ERR;
                    } else {
                        check_binding_identifier(rt, &tok->location, tok->value);

                        id_new(rt, &prop->binding, &tok->location, tok->value);

                        tok = get_token(rt);
                        if (tok->type == RJS_TOKEN_eq) {
                            r = parse_expr_in_prio(rt, PRIO_ASSI, &prop->init);
                            if (r == RJS_OK) {
                                RJS_AstValueExpr *ve;
                                RJS_Ast          *init;

                                check_expr(rt, &prop->init);

                                ve   = value_expr_get(rt, &prop->name);
                                init = ast_get(rt, &prop->init);
                                if (ve && init)
                                    func_set_name(rt, init, &ve->ast.location, &ve->ve->value);
                            }
                        } else {
                            unget_token(rt);
                        }
                    }
                }
            }

            if (r == RJS_OK) {
                rjs_list_append(&ob->list, &prop->ast.ln);

                if (!loc_update_last_ast(rt, &prop->ast.location, &prop->init))
                    if (!loc_update_last_ast(rt, &prop->ast.location, &prop->binding))
                        loc_update_last_ast(rt, &prop->ast.location, &prop->name);
            }
        }

        if (r == RJS_ERR) {
            recover_object(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `}\'"));
            recover_object(rt);
        }
    }

    loc_update_last_token(rt, &ob->ast.location);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse binding.*/
static RJS_Result
parse_binding (RJS_Runtime *rt, RJS_Value *vb)
{
    RJS_Token *tok;
    RJS_Result r = RJS_ERR;

    if (!(tok = get_token(rt)))
        goto end;

    switch (tok->type) {
    case RJS_TOKEN_lbracket:
        if ((r = parse_array_binding_pattern(rt, vb)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_lbrace:
        if ((r = parse_object_binding_pattern(rt, vb)) == RJS_ERR)
            goto end;
        break;
    default: {
        if (!is_binding_identifier(rt, tok->type, tok->flags)) {
            parse_unexpect_error(rt, &tok->location, _("binding identifier or binding pattern"));
            goto end;
        }

        check_binding_identifier(rt, &tok->location, tok->value);
        id_new(rt, vb, &tok->location, tok->value);
        break;
    }
    }

    r = RJS_OK;
end:
    return r;
}

/*Parse binding element.*/
static RJS_Result
parse_binding_element (RJS_Runtime *rt, RJS_Value *vb, RJS_Value *vinit)
{
    RJS_Token *tok;
    RJS_Result r = RJS_ERR;

    if ((r = parse_binding(rt, vb)) == RJS_ERR)
        goto end;

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_eq) {
        RJS_Ast *ast;
        RJS_Ast *init;

        if ((r = parse_expr_in_prio(rt, PRIO_ASSI, vinit)) == RJS_ERR)
            goto end;

        check_expr(rt, vinit);

        ast  = ast_get(rt, vb);
        init = ast_get(rt, vinit);
        if (init && (ast->type == RJS_AST_Id)) {
            RJS_AstId *id = (RJS_AstId*)ast;

            func_set_name(rt, init, &ast->location, &id->identifier->value);
        }
    } else {
        unget_token(rt);
    }

    r = RJS_OK;
end:
    return r;
}

/*Check the bound name is not "let".*/
static void
let_bound_name_check (RJS_Runtime *rt, RJS_Value *vr)
{
    RJS_Ast *ast = ast_get(rt, vr);

    switch (ast->type) {
    case RJS_AST_Id: {
        RJS_AstId *ir = (RJS_AstId*)ast;

        if (rjs_string_equal(rt, &ir->identifier->value, rjs_s_let(rt)))
            parse_error(rt, &ast->location, _("\"let\" cannot be used as a declaration name"));
        break;
    }
    case RJS_AST_ArrayBinding:
    case RJS_AST_ObjectBinding: {
        RJS_AstList *l = (RJS_AstList*)ast;
        RJS_Ast     *e;

        rjs_list_foreach_c(&l->list, e, RJS_Ast, ln) {
            switch (e->type) {
            case RJS_AST_BindingElem: {
                RJS_AstBindingElem *be = (RJS_AstBindingElem*)e;

                let_bound_name_check(rt, &be->binding);
                break;
            }
            case RJS_AST_BindingProp: {
                RJS_AstBindingProp *bp = (RJS_AstBindingProp*)e;

                let_bound_name_check(rt, &bp->binding);
                break;
            }
            case RJS_AST_Rest: {
                RJS_AstRest *rest = (RJS_AstRest*)e;

                let_bound_name_check(rt, &rest->binding);
                break;
            }
            default:
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}

/*Parse the declaration item.*/
static RJS_Result
parse_decl_item (RJS_Runtime *rt, RJS_AstType decl_type, RJS_Value *vd, RJS_Bool in_for)
{
    RJS_Token          *tok;
    RJS_Result          r;
    RJS_Bool            has_init = RJS_FALSE;
    RJS_AstBindingElem *be;

    if (!in_for && (decl_type == RJS_AST_ConstDecl))
        has_init = RJS_TRUE;

    tok = next_token(rt);
    if (!in_for && ((tok->type == RJS_TOKEN_lbrace) || (tok->type == RJS_TOKEN_lbracket)))
        has_init = RJS_TRUE;

    be = ast_new(rt, vd, RJS_AST_BindingElem, &tok->location);

    if ((r = parse_binding(rt, &be->binding)) == RJS_ERR)
        goto end;

    if ((decl_type == RJS_AST_LetDecl) || (decl_type == RJS_AST_ConstDecl)) {
        let_bound_name_check(rt, &be->binding);
    }

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_eq) {
        RJS_AstId *id;
        RJS_Ast   *ast;

        if ((r = parse_expr_prio(rt, PRIO_ASSI, &be->init)) == RJS_ERR)
            goto end;

        check_expr(rt, &be->init);

        id  = id_get(rt, &be->binding);
        ast = ast_get(rt, &be->init);
        if (id && ast)
            func_set_name(rt, ast, &id->ast.location, &id->identifier->value);
    } else {
        if (has_init) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_eq, 0);
            r = RJS_ERR;
            goto end;
        } else {
            unget_token(rt);
        }
    }

    if (!loc_update_last_ast(rt, &be->ast.location, &be->init))
        loc_update_last_ast(rt, &be->ast.location, &be->binding);

    r = RJS_OK;
end:
    return r;
}

/*Add declaration in binding*/
static void
binding_lex (RJS_Runtime *rt, RJS_AstDeclType dtype, RJS_Value *v)
{
    RJS_Ast *ast = ast_get(rt, v);

    switch (ast->type) {
    case RJS_AST_Id: {
        RJS_AstId *ir = (RJS_AstId*)ast;

        decl_item_add(rt, dtype, &ir->ast.location, &ir->identifier->value, NULL);
        break;
    }
    case RJS_AST_ObjectBinding:
    case RJS_AST_ArrayBinding: {
        RJS_AstList *l = (RJS_AstList*)ast;

        binding_element_list_lex(rt, dtype, &l->list);
        break;
    }
    default:
        assert(0);
    }
}

/*Add declaration in binding elements.*/
static void
binding_element_list_lex (RJS_Runtime *rt, RJS_AstDeclType dtype, RJS_List *list)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(list, ast, RJS_Ast, ln) {
        switch (ast->type) {
        case RJS_AST_BindingElem: {
            RJS_AstBindingElem *be = (RJS_AstBindingElem*)ast;

            binding_lex(rt, dtype, &be->binding);
            break;
        }
        case RJS_AST_BindingProp: {
            RJS_AstBindingProp *bp = (RJS_AstBindingProp*)ast;

            binding_lex(rt, dtype, &bp->binding);
            break;
        }
        case RJS_AST_Rest: {
            RJS_AstRest *rest = (RJS_AstRest*)ast;

            binding_lex(rt, dtype, &rest->binding);
            break;
        }
        case RJS_AST_Elision:
        case RJS_AST_LastElision:
            break;
        default:
            assert(0);
        }
    }
}

/*Parse the declaration list.*/
static RJS_Result
parse_decl_list (RJS_Runtime *rt, RJS_AstType type, RJS_Value *vs, RJS_Bool in_for)
{
    RJS_Result   r;
    RJS_Token   *tok;
    RJS_AstList *list;
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *tmp = rjs_value_stack_push(rt);

    tok  = curr_token(rt);
    list = ast_new(rt, vs, type, &tok->location);

    if ((r = parse_decl_item(rt, type, tmp, in_for)) == RJS_ERR)
        goto end;

    ast_list_append(rt, &list->list, tmp);
    loc_update_last_ast(rt, &list->ast.location, tmp);

    while (1) {
        tok = get_token(rt);
        if (tok->type != RJS_TOKEN_comma) {
            unget_token(rt);
            break;
        }

        if ((r = parse_decl_item(rt, type, tmp, in_for)) == RJS_ERR)
            goto end;

        ast_list_append(rt, &list->list, tmp);
        loc_update_last_ast(rt, &list->ast.location, tmp);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the variable statement.*/
static RJS_Result
parse_var_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Result   r         = RJS_ERR;
    RJS_Parser  *parser    = rt->parser;
    int          old_flags = parser->flags;
    RJS_AstList *list;

    parser->flags |= RJS_PARSE_FL_IN;

    /*Parse the statement.*/
    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_var)) == RJS_ERR)
        goto end;

    if ((r = parse_decl_list(rt, RJS_AST_VarDecl, vs, RJS_FALSE)) == RJS_ERR)
        goto end;

    /*Add the variables to lexical declarations.*/
    list = ast_get(rt, vs);
    binding_element_list_lex(rt, RJS_AST_DECL_VAR, &list->list);

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    parser->flags = old_flags;
    return r;
}

/*Parse the if else statement.*/
static RJS_Result
parse_if_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token     *tok;
    RJS_Result     r = RJS_ERR;
    RJS_AstIfStmt *stmt;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_if)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_IfStmt, &tok->location);

    if ((r = parse_parentheses_expr(rt, &stmt->cond)) == RJS_ERR)
        goto end;

    break_push(rt, &stmt->break_js, &stmt->ast);
    r = parse_stmt(rt, &stmt->if_stmt);
    break_pop(rt);
    if (r == RJS_ERR)
        goto end;

    loc_update_last_ast(rt, &stmt->ast.location, &stmt->if_stmt);

    tok = get_token(rt);
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_else)) {
        if ((r = parse_stmt(rt, &stmt->else_stmt)) == RJS_ERR)
            goto end;

        loc_update_last_ast(rt, &stmt->ast.location, &stmt->else_stmt);
    } else {
        unget_token(rt);
    }

    r = RJS_OK;
end:
    return r;
}

/*Parse the do while statement.*/
static RJS_Result
parse_do_while_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token       *tok;
    RJS_Result       r = RJS_ERR;
    RJS_AstLoopStmt *stmt;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_do)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_DoWhileStmt, &tok->location);

    break_push(rt, &stmt->break_js, &stmt->ast);
    continue_push(rt, &stmt->continue_js, &stmt->ast);

    r = parse_stmt(rt, &stmt->loop_stmt);

    break_pop(rt);
    continue_pop(rt);

    if (r == RJS_ERR)
        goto end;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_while)) == RJS_ERR)
        goto end;

    if ((r = parse_parentheses_expr(rt, &stmt->cond)) == RJS_ERR)
        goto end;

    loc_update_last_ast(rt, &stmt->ast.location, &stmt->cond);

    tok = get_token(rt);
    if (tok->type != RJS_TOKEN_semicolon)
        unget_token(rt);

    r = RJS_OK;
end:
    return r;
}

/*Parse the while statement.*/
static RJS_Result
parse_while_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Result       r = RJS_ERR;
    RJS_Token       *tok;
    RJS_AstLoopStmt *stmt;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_while)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_WhileStmt, &tok->location);

    if ((r = parse_parentheses_expr(rt, &stmt->cond)) == RJS_ERR)
        goto end;

    break_push(rt, &stmt->break_js, &stmt->ast);
    continue_push(rt, &stmt->continue_js, &stmt->ast);

    r = parse_stmt(rt, &stmt->loop_stmt);

    break_pop(rt);
    continue_pop(rt);

    if (r == RJS_ERR)
        goto end;

    loc_update_last_ast(rt, &stmt->ast.location, &stmt->loop_stmt);

    r = RJS_OK;
end:
    return r;
}

/*Parse the for statement.*/
static RJS_Result
parse_for_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token      *tok, *ntok;
    RJS_AstForStmt *stmt        = NULL;
    RJS_Parser     *parser      = rt->parser;
    RJS_Result      r           = RJS_ERR;
    RJS_Bool        is_decl     = RJS_FALSE;
    RJS_Bool        is_async_of = RJS_FALSE;
    RJS_AstDeclType dtype       = RJS_AST_DECL_LET;
    size_t          top         = rjs_value_stack_save(rt);
    RJS_Value      *tmp         = rjs_value_stack_push(rt);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_for)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_ForStmt, &tok->location);

    stmt->decl = NULL;

#if ENABLE_ASYNC
    if (parser->flags & RJS_PARSE_FL_AWAIT) {
        tok = get_token(rt);
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_await)) {
            RJS_AstFunc *func = func_top(rt);

            if (func->flags & RJS_AST_FUNC_FL_MODULE)
                func->flags |= RJS_AST_FUNC_FL_ASYNC;

            stmt->ast.type = RJS_AST_AwaitForOfStmt;
        } else {
            unget_token(rt);
        }
    }
#endif /*ENABLE_ASYNC*/

    if ((r = get_token_expect(rt, RJS_TOKEN_lparenthese)) == RJS_ERR)
        goto end;

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_semicolon) {
        unget_token(rt);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_var)) {
        if ((r = parse_decl_list(rt, RJS_AST_VarDecl, &stmt->init, RJS_TRUE)) == RJS_ERR)
            goto end;

        is_decl = RJS_TRUE;
        dtype   = RJS_AST_DECL_VAR;
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_const)) {
        if ((r = parse_decl_list(rt, RJS_AST_ConstDecl, &stmt->init, RJS_TRUE)) == RJS_ERR)
            goto end;

        is_decl = RJS_TRUE;
        dtype   = RJS_AST_DECL_STRICT;
    } else {
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_let)) {
            ntok = next_token_div(rt);
            switch (ntok->type) {
            case RJS_TOKEN_lbrace:
            case RJS_TOKEN_lbracket:
                is_decl = RJS_TRUE;
                dtype   = RJS_AST_DECL_LET;
                break;
            default:
                if (is_binding_identifier(rt, ntok->type, ntok->flags)) {
                    is_decl = RJS_TRUE;
                    dtype   = RJS_AST_DECL_LET;
                }
                break;
            }
        }

        if (is_decl) {
            if ((r = parse_decl_list(rt, RJS_AST_LetDecl, &stmt->init, RJS_TRUE)) == RJS_ERR)
                goto end;
        } else {
            RJS_AstExprStmt *es;
            RJS_Token       *ntok;

            ntok = next_token(rt);
            if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_async)
                    && token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_of))
                is_async_of = RJS_TRUE;

            unget_token(rt);

            es = ast_new(rt, &stmt->init, RJS_AST_ExprStmt, &tok->location);

            if ((r = parse_expr(rt, &es->expr)) == RJS_ERR)
                goto end;

            loc_update_last_ast(rt, &es->ast.location, &es->expr);
        }
    }

    if (is_decl) {
        RJS_AstList *l = ast_get(rt, &stmt->init);

        /*Initialize the declaration.*/
        if (dtype != RJS_AST_DECL_VAR)
            stmt->decl = decl_push(rt);

        binding_element_list_lex(rt, dtype, &l->list);
    }

    tok = get_token(rt);

    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_in)
            || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_of)) {
        RJS_Ast *ast = ast_get(rt, &stmt->init);

        if (!ast) {
            parse_error(rt, &tok->location, _("expect a left hand expression before `in\' or `of\'"));
        } else if ((ast->type == RJS_AST_VarDecl)
                || (ast->type == RJS_AST_LetDecl)
                || (ast->type == RJS_AST_ConstDecl)) {
            RJS_AstList *l = (RJS_AstList*)ast;

            if (!rjs_list_has_1_node(&l->list)) {
                parse_error(rt, &ast->location, _("expect ony 1 declaratin before `in\' or `of\'"));
            } else {
                RJS_AstBindingElem *be;

                be = RJS_CONTAINER_OF(l->list.next, RJS_AstBindingElem, ast.ln);

                if (ast_get(rt, &be->init)) {
                    parse_error(rt, &ast->location, _("declaratin before `in\' or `of\' cannot has initializer"));
                }
            }
        } else {
            RJS_AstExprStmt *es = ast_get(rt, &stmt->init);

            /*Left hand expression must look ahead not async of.*/
            if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_of)
                    && (stmt->ast.type != RJS_AST_AwaitForOfStmt)
                    && is_async_of) {
                parse_error(rt, &es->ast.location, _("`async\' is disallowed when followed by `of\' in for-of statement"));
            }

            expr_to_lh(rt, &es->expr, tmp);
            rjs_value_copy(rt, &es->expr, tmp);
        }
    } else if (!is_decl) {
        RJS_AstExprStmt *es = ast_get(rt, &stmt->init);

        if (es)
            check_expr(rt, &es->expr);
    }

    if (tok->type == RJS_TOKEN_semicolon) {

        if (stmt->ast.type == RJS_AST_AwaitForOfStmt)
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, RJS_IDENTIFIER_of);

        tok = get_token(rt);
        if (tok->type != RJS_TOKEN_semicolon) {
            unget_token(rt);

            if ((r = parse_expr_in(rt, &stmt->cond)) == RJS_ERR)
                goto end;

            check_expr(rt, &stmt->cond);

            if ((r = get_token_expect(rt, RJS_TOKEN_semicolon)) == RJS_ERR)
                goto end;
        }

        tok = get_token(rt);
        if (tok->type != RJS_TOKEN_rparenthese) {
            unget_token(rt);

            if ((r = parse_expr_in(rt, &stmt->step)) == RJS_ERR)
                goto end;

            check_expr(rt, &stmt->step);
        } else {
            unget_token(rt);
        }
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_in)) {
        if (stmt->ast.type == RJS_AST_AwaitForOfStmt)
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, RJS_IDENTIFIER_of);

        stmt->ast.type = RJS_AST_ForInStmt;

        if ((r = parse_expr_in(rt, &stmt->cond)) == RJS_ERR)
            goto end;

        check_expr(rt, &stmt->cond);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_of)) {
        if (stmt->ast.type == RJS_AST_ForStmt)
            stmt->ast.type = RJS_AST_ForOfStmt;

        if ((r = parse_expr_in_prio(rt, PRIO_ASSI, &stmt->cond)) == RJS_ERR)
            goto end;

        check_expr(rt, &stmt->cond);
    } else {
        parse_unexpect_error(rt, &tok->location, _("`in\', `of\' or `;\'"));
        r = RJS_ERR;
        goto end;
    }

    if ((r = get_token_expect(rt, RJS_TOKEN_rparenthese)) == RJS_ERR)
            goto end;

    break_push(rt, &stmt->break_js, &stmt->ast);
    continue_push(rt, &stmt->continue_js, &stmt->ast);

    r = parse_stmt(rt, &stmt->loop_stmt);

    break_pop(rt);
    continue_pop(rt);

    if (r == RJS_ERR)
        goto end;

    /*Check if the bound names is redefined in statement.*/
    if (is_decl && (dtype != RJS_AST_DECL_VAR)) {
        RJS_Ast *ast = ast_get(rt, &stmt->loop_stmt);

        if (ast && (ast->type == RJS_AST_Block)) {
            RJS_AstBlock    *blk = (RJS_AstBlock*)ast;
            RJS_AstDeclItem *di;

            rjs_list_foreach_c(&stmt->decl->item_list, di, RJS_AstDeclItem, ast.ln) {
                if (di->type != RJS_AST_DECL_VAR) {
                    RJS_HashEntry *he;

                    rjs_value_set_string(rt, tmp, di->he.key);

                    he = hash_lookup(rt, &blk->decl->item_hash, tmp, NULL);
                    if (he) {
                        RJS_AstDeclItem *ndi = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

                        parse_error(rt, &ndi->ast.location, _("\"%s\" is already defined"),
                                rjs_string_to_enc_chars(rt, tmp, NULL, NULL));
                        parse_prev_define_note(rt, &di->ast.location);
                    }
                }
            }
        }
    }

    loc_update_last_ast(rt, &stmt->ast.location, &stmt->loop_stmt);

    r = RJS_OK;
end:
    if (stmt) {
        if (parser->decl_stack) {
            if (parser->decl_stack == stmt->decl)
                decl_pop(rt);
        }
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse statement list in switch.*/
static RJS_Result
parse_switch_stmt_list (RJS_Runtime *rt, RJS_AstCase *cc)
{
    RJS_Token   *tok;
    RJS_Result   r;
    RJS_Bool     loop = RJS_TRUE;
    size_t       top  = rjs_value_stack_save(rt);
    RJS_Value   *tmp  = rjs_value_stack_push(rt);

    while (loop) {
        tok = get_token(rt);
        if ((tok->type == RJS_TOKEN_rbrace)
                || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_case)
                || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_default)) {
            unget_token(rt);
            loop = RJS_FALSE;
        } else {
            unget_token(rt);
            r = parse_stmt_list_item(rt, tmp);
            if (r == RJS_ERR) {
                recover_stmt(rt, RECOVER_SWITCH);
            } else {
                ast_list_append(rt, &cc->stmt_list, tmp);
            }
        }
    }

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Parse case clause.*/
static RJS_Result
parse_case_clause (RJS_Runtime *rt, RJS_AstCase *cc)
{
    RJS_Result  r = RJS_ERR;

    if ((r = parse_expr_in(rt, &cc->cond)) == RJS_ERR)
        goto end;

    check_expr(rt, &cc->cond);

    loc_update_last_ast(rt, &cc->ast.location, &cc->cond);

    if ((r = get_token_expect(rt, RJS_TOKEN_colon)) == RJS_ERR)
        goto end;

    r = parse_switch_stmt_list(rt, cc);
end:
    return r;
}

/*Parse default clause.*/
static RJS_Result
parse_default_clause (RJS_Runtime *rt, RJS_AstCase *cc)
{
    RJS_Result r = RJS_ERR;

    if ((r = get_token_expect(rt, RJS_TOKEN_colon)) == RJS_ERR)
        goto end;

    r = parse_switch_stmt_list(rt, cc);
end:
    return r;
}

/*Parse the switch statement.*/
static RJS_Result
parse_switch_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token         *tok;
    RJS_AstSwitchStmt *stmt;
    RJS_AstCase       *cc;
    RJS_Location       def_loc;
    RJS_Bool           has_def = RJS_FALSE;
    RJS_Result         r       = RJS_ERR;
    RJS_AstFunc       *func    = func_top(rt);
    RJS_Ast           *bot_blk = func->block_stack;
    size_t             top     = rjs_value_stack_save(rt);
    RJS_Value         *tmp     = rjs_value_stack_push(rt);
    RJS_Bool           pushed  = RJS_FALSE;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_switch)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_SwitchStmt, &tok->location);

    stmt->decl = NULL;

    if ((r = parse_parentheses_expr(rt, &stmt->cond)) == RJS_ERR)
        goto end;

    if ((r = get_token_expect(rt, RJS_TOKEN_lbrace)) == RJS_ERR)
        goto end;

    break_push(rt, &stmt->break_js, &stmt->ast);
    stmt->decl = decl_push(rt);
    func->block_stack = &stmt->ast;
    pushed = RJS_TRUE;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            r = RJS_ERR;
            goto end;
        }

        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_case)) {
            cc = ast_new(rt, tmp, RJS_AST_Case, &tok->location);
            ast_list_append(rt, &stmt->case_list, tmp);

            r  = parse_case_clause(rt, cc);
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_default)) {
            if (has_def) {
                parse_error(rt, &tok->location, _("`default\' is already used in switch block"));
                parse_prev_define_note(rt, &def_loc);
            } else {
                has_def = RJS_TRUE;
                def_loc = tok->location;
            }

            cc = ast_new(rt, tmp, RJS_AST_Case, &tok->location);
            ast_list_append(rt, &stmt->case_list, tmp);

            r  = parse_default_clause(rt, cc);
        } else {
            parse_unexpect_error(rt, &tok->location, _("`case\', `default\' or `}\'"));
            r = RJS_ERR;
        }

        if (r == RJS_ERR)
            recover_switch(rt);
    }

    loc_update_last_token(rt, &stmt->ast.location);

    r = RJS_OK;
end:
    if (pushed) {
        decl_pop(rt);
        break_pop(rt);
    }

    func->block_stack = bot_blk;

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the with statement.*/
static RJS_Result
parse_with_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token       *tok;
    RJS_Parser      *parser = rt->parser;
    RJS_Result       r      = RJS_ERR;
    RJS_AstWithStmt *stmt;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_with)) == RJS_ERR)
        goto end;

    if (parser->flags & RJS_PARSE_FL_STRICT) {
        tok = curr_token(rt);
        parse_error(rt, &tok->location, _("`with\' statement cannot be used in strict mode"));
        r = RJS_ERR;
        goto end;
    }

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_WithStmt, &tok->location);

    if ((r = parse_parentheses_expr(rt, &stmt->with_expr)) == RJS_ERR)
        goto end;

    stmt->decl = decl_push(rt);

    break_push(rt, &stmt->break_js, &stmt->ast);
    r = parse_stmt(rt, &stmt->with_stmt);
    break_pop(rt);

    decl_pop(rt);

    if (r == RJS_ERR)
        goto end;

    loc_update_last_ast(rt, &stmt->ast.location, &stmt->with_stmt);

    r = RJS_OK;
end:
    return r;
}

/*Parse the throw statement.*/
static RJS_Result
parse_throw_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token       *tok;
    RJS_Parser      *parser = rt->parser;
    RJS_Result       r      = RJS_ERR;
    RJS_AstExprStmt *stmt;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_throw)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_ThrowStmt, &tok->location);

    tok = get_token(rt);
    if (tok->location.first_line != parser->last_line) {
        parse_error(rt, &tok->location, _("line terminator cannot be after `throw\'"));
        r = RJS_ERR;
        goto end;
    }

    unget_token(rt);
    if ((r = parse_expr_in(rt, &stmt->expr)) == RJS_ERR)
        goto end;

    check_expr(rt, &stmt->expr);

    loc_update_last_ast(rt, &stmt->ast.location, &stmt->expr);

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    return r;
}

/*Parse the try statement.*/
static RJS_Result
parse_try_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token      *tok;
    RJS_AstTryStmt *stmt;
    size_t          top = rjs_value_stack_save(rt);
    RJS_Value      *tmp = rjs_value_stack_push(rt);
    RJS_Result      r   = RJS_ERR;

    /*Try block.*/
    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_try)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_TryStmt, &tok->location);

    stmt->catch_decl = NULL;

    break_push(rt, &stmt->break_js, &stmt->ast);
    r = parse_block(rt, &stmt->try_block);
    break_pop(rt);
    if (r == RJS_ERR)
        goto end;

    /*Catch block.*/
    tok = get_token(rt);
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_catch)) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_lparenthese) {
            if ((r = parse_binding(rt, &stmt->catch_binding)) == RJS_ERR)
                goto end;

            if ((r = get_token_expect(rt, RJS_TOKEN_rparenthese)) == RJS_ERR)
                goto end;

            stmt->catch_decl = decl_push(rt);

            binding_lex(rt, RJS_AST_DECL_LET, &stmt->catch_binding);
        } else {
            unget_token(rt);
        }

        r = parse_block(rt, &stmt->catch_block);

        decl_pop(rt);

        if (r == RJS_ERR)
            goto end;

        /*Check if the catch parameter is redefined in block.*/
        if (stmt->catch_decl) {
            RJS_AstBlock *blk = ast_get(rt, &stmt->catch_block);

            if (blk && blk->decl) {
                RJS_AstDeclItem *di;

                rjs_list_foreach_c(&stmt->catch_decl->item_list, di, RJS_AstDeclItem, ast.ln) {
                    if (di->type != RJS_AST_DECL_VAR) {
                        RJS_HashEntry *he;

                        rjs_value_set_string(rt, tmp, di->he.key);

                        he = hash_lookup(rt, &blk->decl->item_hash, tmp, NULL);
                        if (he) {
                            RJS_AstDeclItem *ndi = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

                            parse_error(rt, &ndi->ast.location, _("\"%s\" is already defined"),
                                    rjs_string_to_enc_chars(rt, tmp, NULL, NULL));
                            parse_prev_define_note(rt, &di->ast.location);
                        }
                    }
                }
            }
        }

        loc_update_last_ast(rt, &stmt->ast.location, &stmt->catch_block);

        tok = get_token(rt);
    } else if (!token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_finally)) {
        parse_unexpect_error(rt, &tok->location, _("`catch\' or `finally\'"));
        goto end;
    }

    /*Finally block.*/
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_finally)) {
        if ((r = parse_block(rt, &stmt->final_block)) == RJS_ERR)
            goto end;

        loc_update_last_ast(rt, &stmt->ast.location, &stmt->final_block);
    } else {
        unget_token(rt);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Lookup the statement with the label.*/
static RJS_Ast*
labelled_stmt_lookup (RJS_Runtime *rt, RJS_Value *id)
{
    RJS_AstFunc      *func = func_top(rt);
    RJS_AstLabelStmt *ls;
    RJS_Ast          *stmt = NULL;

    for (ls = func->label_stack; ls; ls = ls->bot) {
        if (rjs_same_value(rt, &ls->identifier, id))
            break;
    }

    if (!ls)
        return NULL;

    while (1) {
        RJS_Ast *ast;

        if (rjs_value_is_undefined(rt, &ls->stmt))
            break;

        ast = ast_get(rt, &ls->stmt);
        if (ast->type == RJS_AST_LabelStmt) {
            ls = (RJS_AstLabelStmt*)ast;
        } else {
            stmt = ast;
            break;
        }
    }

    return stmt;
}

/*Parse the break statement.*/
static RJS_Result
parse_break_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token        *tok;
    RJS_AstJumpStmt  *stmt;
    RJS_Parser       *parser = rt->parser;
    RJS_Result        r      = RJS_ERR;
    RJS_AstFunc      *func   = func_top(rt);
    RJS_AstJumpStack *js     = NULL;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_break)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_BreakStmt, &tok->location);

    tok = get_token(rt);
    if ((tok->location.first_line == parser->last_line)
            && is_identifier_reference(rt, tok->type, tok->flags)) {
        RJS_Ast *dest;

        dest = labelled_stmt_lookup(rt, tok->value);
        if (!dest) {
            parse_error(rt, &tok->location, _("cannot find label \"%s\""),
                    rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
        } else if (dest != &stmt->ast) {
            for (js = func->break_stack; js; js = js->bot) {
                if (js->stmt == dest)
                    break;
            }

            if (!js) {
                parse_error(rt, &tok->location, _("cannot jump to label \"%s\""),
                        rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
            }
        } else {
            /*label: break label*/
        }

        loc_update_last_token(rt, &stmt->ast.location);
    } else {
        unget_token(rt);

        js = func->break_stack;
        while (js) {
            if ((js->stmt->type == RJS_AST_DoWhileStmt)
                    || (js->stmt->type == RJS_AST_WhileStmt)
                    || (js->stmt->type == RJS_AST_ForStmt)
                    || (js->stmt->type == RJS_AST_ForInStmt)
                    || (js->stmt->type == RJS_AST_ForOfStmt)
                    || (js->stmt->type == RJS_AST_AwaitForOfStmt)
                    || (js->stmt->type == RJS_AST_SwitchStmt))
                break;

            js = js->bot;
        }

        if (!js)
            parse_error(rt, &stmt->ast.location, _("`break\' cannot be here"));
    }

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    stmt->dest = js;

    r = RJS_OK;
end:
    return r;
}

/*Parse the continue statement.*/
static RJS_Result
parse_continue_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token        *tok;
    RJS_AstJumpStmt  *stmt;
    RJS_AstFunc      *func   = func_top(rt);
    RJS_Parser       *parser = rt->parser;
    RJS_Result        r      = RJS_ERR;
    RJS_AstJumpStack *js     = NULL;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_continue)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_ContinueStmt, &tok->location);

    tok = get_token(rt);
    if ((tok->location.first_line == parser->last_line)
            && is_identifier_reference(rt, tok->type, tok->flags)) {
        RJS_Ast *dest;

        dest = labelled_stmt_lookup(rt, tok->value);
        if (!dest) {
            parse_error(rt, &tok->location, _("cannot find label \"%s\""),
                    rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
        } else {
            for (js = func->continue_stack; js; js = js->bot) {
                if (js->stmt == dest)
                    break;
            }

            if (!js) {
                parse_error(rt, &tok->location, _("cannot jump to label \"%s\""),
                        rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
            }
        }

        loc_update_last_token(rt, &stmt->ast.location);
    } else {
        unget_token(rt);
        
        js = func->continue_stack;
        if (!js)
            parse_error(rt, &stmt->ast.location, _("`continue\' cannot be here"));
    }

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    stmt->dest = js;

    r = RJS_OK;
end:
    return r;
}

/*Parse the return statement.*/
static RJS_Result
parse_return_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token        *tok;
    RJS_AstExprStmt  *stmt;
    RJS_Parser       *parser = rt->parser;
    RJS_Result        r      = RJS_ERR;
    size_t            top    = rjs_value_stack_save(rt);
    RJS_Value        *expr   = rjs_value_stack_push(rt);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_return)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    stmt = ast_new(rt, vs, RJS_AST_ReturnStmt, &tok->location);

    if (!(parser->flags & RJS_PARSE_FL_RETURN)) {
        tok = curr_token(rt);
        parse_error(rt, &tok->location, _("return statement cannot be here"));
        goto end;
    }

    tok = get_token(rt);
    if ((tok->location.first_line == parser->last_line) && (tok->type != RJS_TOKEN_semicolon)) {
        RJS_Ast *ast;

        unget_token(rt);

        if ((r = parse_expr_in(rt, expr)) == RJS_ERR)
            goto end;

        while (1) {
            ast = ast_get(rt, expr);
            if (ast->type == RJS_AST_ParenthesesExpr) {
                RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

                rjs_value_copy(rt, expr, &ue->operand);
            } else {
                break;
            }
        }

        rjs_value_copy(rt, &stmt->expr, expr);
        check_expr(rt, &stmt->expr);

        loc_update_last_ast(rt, &stmt->ast.location, &stmt->expr);
    } else {
        unget_token(rt);
    }

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the debugger statement.*/
static RJS_Result
parse_debugger_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token  *tok;
    RJS_Result  r = RJS_ERR;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_debugger)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    ast_new(rt, vs, RJS_AST_DebuggerStmt, &tok->location);

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    return r;
}

/*Parse the expression statement.*/
static RJS_Result
parse_expr_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token       *tok;
    RJS_AstExprStmt *stmt;
    RJS_Token       *ntok = NULL;
    RJS_Result       r    = RJS_ERR;

    tok = get_token(rt);
    if ((tok->type == '{')
            || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_class)
            || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_function)) {
        parse_error(rt, &tok->location, _("expect an expression statement here"));
        r = RJS_ERR;
        goto end;
    }

    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_let)) {
        ntok = next_token_div(rt);
        if (ntok->type == RJS_TOKEN_lbracket) {
            parse_error(rt, &tok->location, _("expect an expression statement here"));
            r = RJS_ERR;
            goto end;
        }
    }

    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_async)) {
        ntok = next_token_div(rt);
        if (token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_function)
                && (tok->location.last_line == ntok->location.first_line)) {
            parse_error(rt, &tok->location, _("expect an expression statement here"));
            r = RJS_ERR;
            goto end;
        }
    }

    stmt = ast_new(rt, vs, RJS_AST_ExprStmt, &tok->location);

    unget_token(rt);
    if ((r = parse_expr_in(rt, &stmt->expr)) == RJS_ERR)
        goto end;

    check_expr(rt, &stmt->expr);

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    return r;
}

/*Parse the labelled statement.*/
static RJS_Result
parse_labelled_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token        *tok;
    RJS_AstLabelStmt *stmt, *bot;
    RJS_AstFunc      *func   = func_top(rt);
    RJS_Result        r      = RJS_ERR;
    RJS_Bool          pushed = RJS_FALSE;

    tok = get_token(rt);
    if (!is_identifier_reference(rt, tok->type, tok->type)) {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
        goto end;
    }

    check_identifier_reference(rt, &tok->location, tok->value);

    stmt = ast_new(rt, vs, RJS_AST_LabelStmt, &tok->location);

    rjs_value_copy(rt, &stmt->identifier, tok->value);

    for (bot = func->label_stack; bot; bot = bot->bot) {
        if (rjs_same_value(rt, &stmt->identifier, &bot->identifier)) {
            parse_error(rt, &tok->location, _("label \"%s\" is already defined"),
                    rjs_string_to_enc_chars(rt, &stmt->identifier, NULL, NULL));
            parse_prev_define_note(rt, &bot->ast.location);
        }
    }

    stmt->bot         = func->label_stack;
    func->label_stack = stmt;
    pushed            = RJS_TRUE;

    if ((r = get_token_expect(rt, RJS_TOKEN_colon)) == RJS_ERR)
        goto end;

    tok = next_token(rt);
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_function)) {
        parse_error(rt, &tok->location, _("label cannot be used before the function declaration"));
        r = parse_hoistable_decl(rt, RJS_FALSE, &stmt->stmt);
    } else {
        r = parse_stmt(rt, &stmt->stmt);
    }
end:
    if (pushed)
        func->label_stack = stmt->bot;

    return r;
}

/*Parse a statement.*/
static RJS_Result
parse_stmt (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token *tok;
    RJS_Result r = RJS_OK;

    tok = get_token(rt);

    if (tok->type == RJS_TOKEN_semicolon) {
        ast_new(rt, vs, RJS_AST_EmptyStmt, &tok->location);
    } else if (tok->type == RJS_TOKEN_lbrace) {
        unget_token(rt);
        r = parse_block_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_var)) {
        unget_token(rt);
        r = parse_var_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_if)) {
        unget_token(rt);
        r = parse_if_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_do)) {
        unget_token(rt);
        r = parse_do_while_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_while)) {
        unget_token(rt);
        r = parse_while_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_for)) {
        unget_token(rt);
        r = parse_for_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_switch)) {
        unget_token(rt);
        r = parse_switch_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_with)) {
        unget_token(rt);
        r = parse_with_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_throw)) {
        unget_token(rt);
        r = parse_throw_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_try)) {
        unget_token(rt);
        r = parse_try_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_break)) {
        unget_token(rt);
        r = parse_break_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_continue)) {
        unget_token(rt);
        r = parse_continue_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_return)) {
        unget_token(rt);
        r = parse_return_stmt(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_debugger)) {
        unget_token(rt);
        r = parse_debugger_stmt(rt, vs);
    } else {
        RJS_Bool matched = RJS_FALSE;

        if (is_identifier_reference(rt, tok->type, tok->flags)) {
            RJS_Token *ntok = next_token_div(rt);

            if (ntok->type == RJS_TOKEN_colon) {
                unget_token(rt);
                r = parse_labelled_stmt(rt, vs);
                matched = RJS_TRUE;
            }
        }

        if (!matched) {
            unget_token(rt);
            r = parse_expr_stmt(rt, vs);
        }
    }

    return r;
}

/*Parse the lexical declaration.*/
static RJS_Result
parse_lexical_decl (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token       *tok;
    RJS_AstType      type;
    RJS_AstDeclType  dtype;
    RJS_AstList     *list;
    RJS_Result       r = RJS_ERR;

    /*Parse the statement.*/
    tok = get_token(rt);
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_let)) {
        type  = RJS_AST_LetDecl;
        dtype = RJS_AST_DECL_LET;
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_const)) {
        type  = RJS_AST_ConstDecl;
        dtype = RJS_AST_DECL_CONST;
    } else {
        parse_unexpect_error(rt, &tok->location, _("`let\' or `const\'"));
        goto end;
    }

    if ((r = parse_decl_list(rt, type, vs, RJS_FALSE)) == RJS_ERR)
        goto end;

    /*Add the variables to lexical declarations.*/
    list = ast_get(rt, vs);
    binding_element_list_lex(rt, dtype, &list->list);

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    return r;
}

/*Check if the item is the directive prologue.*/
static RJS_Bool
is_directive_prologue (RJS_Runtime *rt, RJS_Value *item)
{
    RJS_Ast          *ast = ast_get(rt, item);
    RJS_AstExprStmt  *es  = (RJS_AstExprStmt*)ast;
    RJS_Ast          *expr;
    RJS_AstValueExpr *ve;

    if (!ast)
        return RJS_FALSE;

    if (ast->type != RJS_AST_ExprStmt)
        return RJS_FALSE;

    expr = ast_get(rt, &es->expr);
    if (!expr)
        return RJS_FALSE;

    if (expr->type != RJS_AST_ValueExpr)
        return RJS_FALSE;

    ve = (RJS_AstValueExpr*)expr;
    if (!rjs_value_is_string(rt, &ve->ve->value))
        return RJS_FALSE;

    if (!(ve->flags & RJS_TOKEN_FL_ESCAPE)
            && rjs_string_equal(rt, &ve->ve->value, rjs_s_use_strict(rt))) {
        RJS_AstFunc *func   = func_top(rt);
        RJS_Parser  *parser = rt->parser;

        func->flags   |= RJS_AST_FUNC_FL_STRICT|RJS_AST_FUNC_FL_USE_STRICT;
        parser->flags |= RJS_PARSE_FL_STRICT;
    }

    return RJS_TRUE;
}

/*Parse function body.*/
static RJS_Result
parse_func_body (RJS_Runtime *rt)
{
    RJS_Token   *tok;
    RJS_Result   r;
    RJS_AstFunc *func         = func_top(rt);
    RJS_Parser  *parser       = rt->parser;
    int          old_flags    = parser->flags;
    size_t       top          = rjs_value_stack_save(rt);
    RJS_Value   *tmp          = rjs_value_stack_push(rt);
    RJS_Bool     check_direct = RJS_TRUE;

    if ((r = get_token_expect(rt, RJS_TOKEN_lbrace)) == RJS_ERR)
        goto end;

    parser->flags |= RJS_PARSE_FL_RETURN;

    func_body(rt);

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            break;
        }

        unget_token(rt);
        r = parse_stmt_list_item(rt, tmp);
        if (r == RJS_ERR) {
            recover_stmt(rt, RECOVER_BLOCK);
        } else {
            ast_list_append(rt, &func->stmt_list, tmp);

            if (check_direct) {
                if (!is_directive_prologue(rt, tmp))
                    check_direct = RJS_FALSE;
            }
        }
    }

    loc_update_last_token(rt, &func->ast.location);

    r = RJS_OK;
end:
    parser->flags = old_flags;
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse formal parameters.*/
static RJS_Result
parse_params (RJS_Runtime *rt)
{
    RJS_Token   *tok;
    RJS_Result   r;
    RJS_AstFunc *func     = func_top(rt);
    size_t       top      = rjs_value_stack_save(rt);
    RJS_Value   *tmp      = rjs_value_stack_push(rt);
    RJS_Bool     has_rest = RJS_FALSE;

    if ((r = get_token_expect(rt, RJS_TOKEN_lparenthese)) == RJS_ERR)
        goto end;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rparenthese)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rparenthese, 0);
            r = RJS_ERR;
            goto end;
        }

        if (tok->type == RJS_TOKEN_dot_dot_dot) {
            RJS_AstRest *rest;

            has_rest = RJS_TRUE;

            rest = ast_new(rt, tmp, RJS_AST_Rest, &tok->location);
            r = parse_binding(rt, &rest->binding);
            if (r == RJS_OK) {
                loc_update_last_ast(rt, &rest->ast.location, &rest->binding);
                ast_list_append(rt, &func->param_list, tmp);
            }
        } else {
            RJS_AstBindingElem *be;

            be = ast_new(rt, tmp, RJS_AST_BindingElem, &tok->location);

            unget_token(rt);
            r = parse_binding_element(rt, &be->binding, &be->init);
            if (r == RJS_OK) {
                if (!loc_update_last_ast(rt, &be->ast.location, &be->init))
                    loc_update_last_ast(rt, &be->ast.location, &be->binding);

                ast_list_append(rt, &func->param_list, tmp);
            }
        }

        if (r == RJS_ERR) {
            recover_params(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rparenthese)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `)\'"));
            recover_params(rt);
        } else if (has_rest) {
            parse_error(rt, &tok->location, _("`...\' cannot be followed with `,\'"));
        }
    }

    loc_update_last_token(rt, &func->ast.location);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the hoistable declaration.*/
static RJS_Result
parse_hoistable_decl (RJS_Runtime *rt, RJS_Bool is_expr, RJS_Value *vr)
{
    RJS_Token       *tok;
    RJS_Result       r;
    RJS_Location     id_loc;
    RJS_Parser      *parser    = rt->parser;
    int              old_flags = parser->flags;
    int              new_flags = 0;
    int              cflags    = 0;
    RJS_AstDecl     *decl      = NULL;
    RJS_AstFunc     *func;
    RJS_AstFuncRef  *fr;

    contains_list_push(rt);

    if (is_expr)
        decl = decl_push(rt);

    tok  = get_token(rt);
    func = func_push(rt, &tok->location);

    no_strict_list_push(rt);

    if (is_expr)
        func->flags |= RJS_AST_FUNC_FL_EXPR;

    fr = ast_new(rt, vr, is_expr ? RJS_AST_FuncExpr : RJS_AST_FuncDecl, &func->ast.location);
    fr->func        = func;
    fr->decl        = decl;
    fr->binding_ref = NULL;

#if ENABLE_ASYNC
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_async)) {
        tok = get_token(rt);
        if (tok->location.first_line != parser->last_line) {
            parse_error(rt, &tok->location, _("line terminator cannot be after `async\'"));
        }

        new_flags   |= RJS_PARSE_FL_AWAIT;
        func->flags |= RJS_AST_FUNC_FL_ASYNC;
        cflags      |= CONTAINS_FL_AWAIT_EXPR;
    }
#endif /*ENABLE_ASYNC*/

    if (!token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_function)) {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, RJS_IDENTIFIER_function);
        r = RJS_ERR;
        goto end;
    }

    tok = get_token(rt);

#if ENABLE_GENERATOR
    if (tok->type == RJS_TOKEN_star) {
        new_flags   |= RJS_PARSE_FL_YIELD;
        func->flags |= RJS_AST_FUNC_FL_GENERATOR;
        cflags      |= CONTAINS_FL_YIELD_EXPR;

        tok = get_token(rt);
    }
#endif /*ENABLE_GENERATOR*/

    if (is_expr) {
        parser->flags &= ~(RJS_PARSE_FL_YIELD|RJS_PARSE_FL_AWAIT);
        parser->flags |= new_flags;
    }

    if (!is_binding_identifier(rt, tok->type, tok->flags)) {
        if (!is_expr) {
            if (!(parser->flags & RJS_PARSE_FL_DEFAULT)) {
                parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
            } else if (parser->flags & RJS_PARSE_FL_DEFAULT) {
                func->binding_name = value_entry_add(rt, &func->ast.location, rjs_s_star_default_star(rt));
                func->name         = value_entry_add(rt, &func->ast.location, rjs_s_default(rt));
                id_loc     = func->ast.location;
            }
        }
        unget_token(rt);
    } else {
        check_binding_identifier(rt, &tok->location, tok->value);

        func->name         = value_entry_add(rt, &tok->location, tok->value);
        func->binding_name = func->name;

        id_loc = tok->location;
    }

    parser->flags &= ~(RJS_PARSE_FL_YIELD|RJS_PARSE_FL_AWAIT);
    parser->flags |= new_flags;

    /*Parse the parameters.*/
    if ((r = parse_params(rt)) == RJS_ERR)
        goto end;

    contains_list_check(rt, cflags|CONTAINS_FL_SUPER_PROP|CONTAINS_FL_SUPER_CALL);
    contains_list_pop(rt, RJS_FALSE);
    contains_list_push(rt);

    /*Parse the function body.*/
    if ((r = parse_func_body(rt)) == RJS_ERR)
        goto end;

    contains_list_check(rt, CONTAINS_FL_SUPER_CALL|CONTAINS_FL_SUPER_PROP);

    r = RJS_OK;
end:
    no_strict_list_pop(rt, RJS_TRUE, RJS_FALSE);
    func_pop(rt);
    contains_list_pop(rt, RJS_FALSE);

    if ((r == RJS_OK) && func->binding_name) {
        if (!is_expr) {
            RJS_AstDeclItem *di;
            RJS_AstFunc     *bot_func = parser->func_stack;

            if (bot_func) {
                di = decl_item_add(rt, RJS_AST_DECL_FUNCTION, &id_loc, &func->binding_name->value, NULL);
                func_decl_ref_new(rt, di, func);
            }
        } else {
            decl_item_add(rt, RJS_AST_DECL_CONST, &id_loc, &func->binding_name->value, NULL);
            fr->binding_ref = binding_ref_new(rt, decl, &id_loc, &func->binding_name->value);
        }
    }

    if (is_expr)
        decl_pop(rt);

    parser->flags = old_flags;

    return r;
}

/*Parse class static block.*/
static RJS_Result
parse_class_static_block (RJS_Runtime *rt, RJS_AstClass *c)
{
    RJS_Token        *tok;
    RJS_Result        r;
    RJS_AstFunc      *func      = NULL;
    RJS_AstClassElem *ce        = NULL;
    RJS_Parser       *parser    = rt->parser;
    int               old_flags = parser->flags;
    size_t            top       = rjs_value_stack_save(rt);
    RJS_Value        *tmp       = rjs_value_stack_push(rt);

    contains_list_push(rt);

    if ((r = get_token_expect(rt, RJS_TOKEN_lbrace)) == RJS_ERR)
        goto end;

    tok  = curr_token(rt);
    func = func_push(rt, &tok->location);

    func_body(rt);

    parser->flags &= ~(RJS_PARSE_FL_YIELD|RJS_PARSE_FL_RETURN);
    parser->flags |= RJS_PARSE_FL_AWAIT;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            r = RJS_ERR;
            goto end;
        }

        unget_token(rt);
        r = parse_stmt_list_item(rt, tmp);
        if (r == RJS_ERR) {
            recover_stmt(rt, RECOVER_BLOCK);
        } else {
            ast_list_append(rt, &func->stmt_list, tmp);
        }
    }

    loc_update_last_token(rt, &func->ast.location);

    ce = ast_new(rt, tmp, RJS_AST_ClassElem, &func->ast.location);
    ce->is_static = RJS_TRUE;
    ce->type      = RJS_AST_CLASS_ELEM_BLOCK;
    ce->func      = func;

    ast_list_append(rt, &c->elem_list, tmp);

    contains_list_check(rt, CONTAINS_FL_SUPER_CALL|CONTAINS_FL_AWAIT_EXPR|CONTAINS_FL_ARGUMENTS);

    r = RJS_OK;
end:
    if (func)
        func_pop(rt);

    contains_list_pop(rt, RJS_FALSE);

    parser->flags = old_flags;
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the property name.*/
static RJS_Result
parse_property_name (RJS_Runtime *rt, RJS_Value *vn)
{
    RJS_Token   *tok;
    RJS_Ast     *ast;
    RJS_Location loc;
    RJS_Result   r;

    tok = get_token(rt);
    switch (tok->type) {
    case RJS_TOKEN_STRING:
    case RJS_TOKEN_NUMBER:
        value_expr_new(rt, vn, &tok->location, tok->value);
        break;
    case RJS_TOKEN_lbracket:
        loc = tok->location;

        if ((r = parse_expr_in_prio(rt, PRIO_ASSI, vn)) == RJS_ERR)
            goto end;

        ast = ast_get(rt, vn);
        loc_update_first(&ast->location, &loc);

        check_expr(rt, vn);

        if ((r = get_token_expect(rt, RJS_TOKEN_rbracket)) == RJS_ERR)
            goto end;

        loc_update_last_token(rt, &ast->location);
        break;
    default:
        if (tok->type != RJS_TOKEN_IDENTIFIER) {
            parse_unexpect_error(rt, &tok->location, _("class element name"));
            r = RJS_ERR;
            goto end;
        } else {
            value_expr_new(rt, vn, &tok->location, tok->value);
        }
        break;
    }

    r = RJS_OK;
end:
    return r;
}

/*Parse the class element name.*/
static RJS_Result
parse_class_element_name (RJS_Runtime *rt, RJS_Bool in_obj, RJS_Bool is_static,
        int flags, RJS_Value *vn)
{
    RJS_Result  r;

#if ENABLE_PRIV_NAME
    RJS_Token  *tok;

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_PRIVATE_IDENTIFIER) {
        if (in_obj) {
            parse_error(rt, &tok->location, _("%s cannot be used in object literal"),
                    rjs_token_type_get_name(tok->type, tok->flags));
        } else {
            int priv_flags = 0;

            if (is_static)
                priv_flags |= PRIV_ID_FL_STATIC;
            if (flags & RJS_AST_CLASS_ELEM_FL_GET)
                priv_flags |= PRIV_ID_FL_GET;
            if (flags & RJS_AST_CLASS_ELEM_FL_SET)
                priv_flags |= PRIV_ID_FL_SET;

            priv_id_new(rt, tok, vn, priv_flags);
        }
    } else {
        unget_token(rt);
        if ((r = parse_property_name(rt, vn)) == RJS_ERR)
            goto end;
    }
#else /*!ENABLE_PRIV_NAME*/
    if ((r = parse_property_name(rt, vn)) == RJS_ERR)
        goto end;
#endif /*ENABLE_PRIV_NAME*/

    r = RJS_OK;
end:
    return r;
}

/*Check if the method is a constructor.*/
static RJS_Bool
method_is_constructor (RJS_Runtime *rt, RJS_AstClassElem *m)
{
    RJS_Ast          *ast = ast_get(rt, &m->name);
    RJS_AstValueExpr *ve;

    if (m->computed || m->is_static)
        return RJS_FALSE;

    if (!ast)
        return RJS_FALSE;

    if (ast->type != RJS_AST_ValueExpr)
        return RJS_FALSE;

    ve = (RJS_AstValueExpr*)ast;

    if ((rjs_value_is_string(rt, &ve->ve->value))
            && rjs_string_equal(rt, &ve->ve->value, rjs_s_constructor(rt)))
        return RJS_TRUE;

    return RJS_FALSE;
}

/*Parse the method's parameters and body.*/
static RJS_Result
parse_method_params_body (RJS_Runtime *rt, RJS_Bool in_obj, RJS_AstClassElem *m, int flags)
{
    RJS_Result  r;
    RJS_Parser *parser        = rt->parser;
    int         old_flags     = parser->flags;
    int         params_cflags = 0;
    int         body_cflags   = 0;

    contains_list_push(rt);

    params_cflags |= CONTAINS_FL_SUPER_CALL;
    body_cflags   |= CONTAINS_FL_SUPER_CALL;

    if (!in_obj) {
        RJS_AstClass *clazz = parser->class_stack;

        assert(clazz);

        if (!m->is_static
                && method_is_constructor(rt, m)
                && !rjs_value_is_undefined(rt, &clazz->extends)) {
            body_cflags &= ~CONTAINS_FL_SUPER_CALL;
        }
    }

    m->func = func_push(rt, &m->ast.location);
    m->func->flags |= flags;
    m->func->flags |= RJS_AST_FUNC_FL_METHOD;

    no_strict_list_push(rt);

#if ENABLE_ASYNC
    if (parser->flags & RJS_PARSE_FL_AWAIT) {
        m->func->flags |= RJS_AST_FUNC_FL_ASYNC;
        params_cflags  |= CONTAINS_FL_AWAIT_EXPR;
    }
#endif /*ENABLE_ASYNC*/

#if ENABLE_GENERATOR
    if (parser->flags & RJS_PARSE_FL_YIELD) {
        m->func->flags |= RJS_AST_FUNC_FL_GENERATOR;
        params_cflags  |= CONTAINS_FL_YIELD_EXPR;
    }
#endif /*ENABLE_GENERATOR*/

    if ((r = parse_params(rt)) == RJS_ERR)
        goto end;

    contains_list_check(rt, params_cflags);
    contains_list_pop(rt, RJS_FALSE);
    contains_list_push(rt);

    loc_update_last(&m->ast.location, &m->func->ast.location);

    /*Parameters check.*/
    if (m->type == RJS_AST_CLASS_ELEM_GET) {
        if (!rjs_list_is_empty(&m->func->param_list))
            parse_error(rt, &m->ast.location, _("getter cannot has any parameter"));
    } else if (m->type == RJS_AST_CLASS_ELEM_SET) {
        if (rjs_list_is_empty(&m->func->param_list)
                || (m->func->param_list.next != m->func->param_list.prev))
            parse_error(rt, &m->ast.location, _("setter must has 1 parameter"));
    }

    if ((r = parse_func_body(rt)) == RJS_ERR)
        goto end;

    loc_update_last(&m->ast.location, &m->func->ast.location);

    if (!in_obj)
        check_class_element(rt, m->is_static, m);

    if (body_cflags)
        contains_list_check(rt, body_cflags);

    r = RJS_OK;
end:
    no_strict_list_pop(rt, RJS_TRUE, RJS_FALSE);
    func_pop(rt);
    contains_list_pop(rt, RJS_FALSE);
    parser->flags = old_flags;
    return r;
}

/*Parse the method.*/
static RJS_Result
parse_method (RJS_Runtime *rt, RJS_Bool is_static, RJS_Bool in_obj, RJS_Value *vm)
{
    RJS_Token        *tok;
    RJS_Result        r;
    RJS_AstClassElem *ce;
    RJS_Parser       *parser      = rt->parser;
    int               old_flags   = parser->flags;
    int               new_flags   = 0;
    int               cen_flags   = RJS_AST_CLASS_ELEM_FL_OTHER;
    int               func_flags  = 0;

    tok = get_token(rt);

    ce = ast_new(rt, vm, RJS_AST_ClassElem, &tok->location);

    ce->type      = RJS_AST_CLASS_ELEM_METHOD;
    ce->is_static = is_static;
    ce->computed  = RJS_FALSE;

#if ENABLE_GENERATOR
    if (tok->type == RJS_TOKEN_star) {
        new_flags |= RJS_PARSE_FL_YIELD;
    } else
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
    if (is_async_method(rt, tok->type, tok->flags)) {
        new_flags |= RJS_PARSE_FL_AWAIT;

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_star) {
            new_flags |= RJS_PARSE_FL_YIELD;
        } else {
            unget_token(rt);
        }
    } else
#endif /*ENABLE_ASYNC*/
    if (is_accessor_method(rt, tok->type, tok->flags)) {
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_get)) {
            ce->type   = RJS_AST_CLASS_ELEM_GET;
            cen_flags  = RJS_AST_CLASS_ELEM_FL_GET;
            func_flags = RJS_AST_FUNC_FL_GET;
        } else {
            ce->type   = RJS_AST_CLASS_ELEM_SET;
            cen_flags  = RJS_AST_CLASS_ELEM_FL_SET;
            func_flags = RJS_AST_FUNC_FL_SET;
        }
    } else {
        unget_token(rt);
    }

    tok = get_token(rt);
    if (tok->type == RJS_TOKEN_lbracket)
        ce->computed = RJS_TRUE;
    unget_token(rt);

    if ((r = parse_class_element_name(rt, in_obj, is_static, cen_flags,
            &ce->name)) == RJS_ERR)
        goto end;

    parser->flags &= ~(RJS_PARSE_FL_AWAIT|RJS_PARSE_FL_YIELD);
    parser->flags |= new_flags;

    r = parse_method_params_body(rt, in_obj, ce, func_flags);
end:
    parser->flags = old_flags;
    return r;
}

/*Parse the class element.*/
static RJS_Result
parse_class_element (RJS_Runtime *rt, RJS_Bool is_static, RJS_AstClass *c)
{
    RJS_Token  *tok;
    RJS_Result  r;
    RJS_Ast    *ast;
    RJS_Parser *parser      = rt->parser;
    int         old_flags   = parser->flags;
    RJS_Bool    is_method   = RJS_FALSE;
    RJS_Bool    computed_pn = RJS_FALSE;
    size_t      top         = rjs_value_stack_save(rt);
    RJS_Value  *ve          = rjs_value_stack_push(rt);
    RJS_Value  *tmp         = rjs_value_stack_push(rt);
    RJS_Value  *vstmt       = rjs_value_stack_push(rt);

    tok = get_token(rt);

#if ENABLE_GENERATOR
    if (tok->type == RJS_TOKEN_star)
        is_method = RJS_TRUE;
#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC
    if (is_async_method(rt, tok->type, tok->flags) || is_accessor_method(rt, tok->type, tok->flags))
        is_method = RJS_TRUE;
#endif /*ENABLE_ASYNC*/

    if (is_method) {
        unget_token(rt);

        if ((r = parse_method(rt, is_static, RJS_FALSE, ve)) == RJS_ERR)
            goto end;
    } else {
        if (tok->type == RJS_TOKEN_lbracket)
            computed_pn = RJS_TRUE;

        unget_token(rt);

        if ((r = parse_class_element_name(rt, RJS_FALSE, is_static,
                RJS_AST_CLASS_ELEM_FL_OTHER, tmp)) == RJS_ERR)
            goto end;

        tok = get_token(rt);

        if (tok->type == RJS_TOKEN_lparenthese) {
            RJS_AstClassElem *m;
            RJS_Ast          *cen = ast_get(rt, tmp);

            m = ast_new(rt, ve, RJS_AST_ClassElem, cen ? &cen->location : &tok->location);

            m->type      = RJS_AST_CLASS_ELEM_METHOD;
            m->is_static = is_static;
            m->computed  = computed_pn;
            rjs_value_copy(rt, &m->name, tmp);

            parser->flags &= ~(RJS_PARSE_FL_AWAIT|RJS_PARSE_FL_YIELD);

            unget_token(rt);
            if ((r = parse_method_params_body(rt, RJS_FALSE, m, 0)) == RJS_ERR)
                goto end;
        } else if (tok->type == RJS_TOKEN_eq) {
            RJS_AstClassElem *field;
            RJS_AstFunc      *func;
            RJS_AstExprStmt  *es;
            RJS_Ast          *init;

            contains_list_push(rt);

            ast   = ast_get(rt, tmp);
            field = ast_new(rt, ve, RJS_AST_ClassElem, &ast->location);

            field->computed  = computed_pn;
            field->is_static = is_static;
            field->is_af     = RJS_FALSE;
            field->type      = RJS_AST_CLASS_ELEM_FIELD;
            field->func      = NULL;
            rjs_value_copy(rt, &field->name, tmp);

            func = func_push(rt, &tok->location);
            func_body(rt);

            func->flags |= RJS_AST_FUNC_FL_CLASS_FIELD_INIT;

            es = ast_new(rt, vstmt, RJS_AST_ReturnStmt, &func->ast.location);
            r  = parse_expr_in_prio(rt, PRIO_ASSI, &es->expr);

            contains_list_check(rt, CONTAINS_FL_SUPER_CALL|CONTAINS_FL_ARGUMENTS);

            func_pop(rt);
            contains_list_pop(rt, RJS_FALSE);

            if (r == RJS_ERR)
                goto end;

            check_expr(rt, &es->expr);

            init = ast_get(rt, &es->expr);
            if (init) {
                if (init->type == RJS_AST_FuncExpr) {
                    RJS_AstFuncRef *fr = (RJS_AstFuncRef*)init;

                    if (!fr->func->name)
                        field->is_af = RJS_TRUE;
                } else if (init->type == RJS_AST_ClassExpr) {
                    RJS_AstClassRef *cr = (RJS_AstClassRef*)init;

                    if (!cr->clazz->name)
                        field->is_af = RJS_TRUE;
                }
            }

            ast_list_append(rt, &func->stmt_list, vstmt);

            loc_update_last_ast(rt, &field->ast.location, &es->expr);

            check_class_element(rt, is_static, field);

            if ((r = auto_semicolon(rt)) == RJS_ERR)
                goto end;

            field->func = func;
        } else {
            RJS_AstClassElem *field;

            ast   = ast_get(rt, tmp);
            field = ast_new(rt, ve, RJS_AST_ClassElem, &ast->location);

            field->computed  = computed_pn;
            field->is_static = is_static;
            field->is_af     = RJS_FALSE;
            field->type      = RJS_AST_CLASS_ELEM_FIELD;
            field->func      = NULL;
            rjs_value_copy(rt, &field->name, tmp);

            check_class_element(rt, is_static, field);

            unget_token(rt);
            if ((r = auto_semicolon(rt)) == RJS_ERR)
                goto end;
        }
    }

    /*Check if the element is the constructor.*/
    ast = ast_get(rt, ve);
    if (ast->type == RJS_AST_ClassElem) {
        RJS_AstClassElem *m = (RJS_AstClassElem*)ast;

        if (method_is_constructor(rt, m)) {
            if (c->constructor) {
                parse_error(rt, &m->ast.location, _("\"constructor\" is already defined"));
                parse_prev_define_note(rt, &c->constructor->ast.location);
            } else {
                c->constructor = m;

                if (m->func) {
                    m->func->flags |= RJS_AST_FUNC_FL_CLASS_CONSTR;

                    if (!rjs_value_is_undefined(rt, &c->extends))
                        m->func->flags |= RJS_AST_FUNC_FL_DERIVED;
                }
            }
        }
    }

    ast_list_append(rt, &c->elem_list, ve);

    r = RJS_OK;
end:
    parser->flags = old_flags;
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse the class declaration.*/
static RJS_Result
parse_class_decl (RJS_Runtime *rt, RJS_Bool is_expr, RJS_Value *vc)
{
    RJS_Token       *tok, *ntok;
    RJS_Result       r;
    RJS_AstClassRef *cr;
    RJS_Location     id_loc;
    RJS_AstClass    *c         = NULL;
    RJS_Parser      *parser    = rt->parser;
    int              old_flags = parser->flags;
    size_t           top       = rjs_value_stack_save(rt);
    RJS_Value       *tmp       = rjs_value_stack_push(rt);
    RJS_Bool         pushed    = RJS_FALSE;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_class)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);

    no_strict_list_push(rt);

    parser->flags |= RJS_PARSE_FL_STRICT;

    tok = curr_token(rt);
    c   = ast_new(rt, tmp, RJS_AST_Class, &tok->location);

    c->constructor  = NULL;
    c->name         = NULL;
    c->binding_name = NULL;

    ast_list_append(rt, &parser->class_list, tmp);

    tok = get_token(rt);

    c->decl = decl_push(rt);

    if (!is_binding_identifier(rt, tok->type, tok->flags)) {
        if (!(parser->flags & RJS_PARSE_FL_DEFAULT) && !is_expr) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
        } else if (parser->flags & RJS_PARSE_FL_DEFAULT) {
            id_loc          = c->ast.location;
            c->name         = value_entry_add(rt, &c->ast.location, rjs_s_default(rt));
            c->binding_name = value_entry_add(rt, &c->ast.location, rjs_s_star_default_star(rt));
        }

        unget_token(rt);
    } else {
        check_binding_identifier(rt, &tok->location, tok->value);

        c->name         = value_entry_add(rt, &tok->location, tok->value);
        c->binding_name = c->name;

        id_loc = tok->location;

        decl_item_add(rt, RJS_AST_DECL_CONST, &id_loc, tok->value, NULL);
    }

    tok = get_token(rt);
    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_extends)) {
        if ((r = parse_expr_prio(rt, PRIO_LH, &c->extends)) == RJS_ERR)
            goto end;

        check_expr(rt, &c->extends);
    } else {
        unget_token(rt);
    }

    /*Push the class to the stack.*/
    c->bot = parser->class_stack;
    parser->class_stack = c;

    parser->flags |= RJS_PARSE_FL_CLASS;

#if ENABLE_PRIV_NAME
    c->priv_env =  priv_env_push(rt);
#endif /*ENABLE_PRIV_NAME*/

    pushed = RJS_TRUE;

    if ((r = get_token_expect(rt, RJS_TOKEN_lbrace)) == RJS_ERR)
        goto end;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type == RJS_TOKEN_END) {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_rbrace, 0);
            r = RJS_ERR;
            goto end;
        }

        r = RJS_OK;
        if (tok->type == RJS_TOKEN_semicolon) {
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_static)) {
            ntok = next_token(rt);

            switch (ntok->type) {
            case RJS_TOKEN_lbrace:
                r = parse_class_static_block(rt, c);
                break;
            case RJS_TOKEN_PRIVATE_IDENTIFIER:
            case RJS_TOKEN_STRING:
            case RJS_TOKEN_NUMBER:
            case RJS_TOKEN_lbracket:
            case RJS_TOKEN_star:
                r = parse_class_element(rt, RJS_TRUE, c);
                break;
            default:
                if (ntok->type == RJS_TOKEN_IDENTIFIER) {
                    r = parse_class_element(rt, RJS_TRUE, c);
                } else {
                    unget_token(rt);
                    r = parse_class_element(rt, RJS_FALSE, c);
                }
                break;
            }
        } else {
            unget_token(rt);
            r = parse_class_element(rt, RJS_FALSE, c);
        }

        if (r == RJS_ERR)
            recover_stmt(rt, RECOVER_CLASS);
    }

    loc_update_last_token(rt, &c->ast.location);

    cr = ast_new(rt, vc, is_expr ? RJS_AST_ClassExpr : RJS_AST_ClassDecl, &c->ast.location);
    cr->clazz = c;

    if (c->constructor && c->constructor->func) {
        /*Set the constructor's name.*/
        RJS_AstFunc *func = c->constructor->func;

        func->name         = c->name;
        func->ast.location = c->ast.location;

        if (is_expr)
            func->flags |= RJS_AST_FUNC_FL_EXPR;
    }

    r = RJS_OK;
end:
    if (c) {
        /*Popup the class.*/
        if (pushed) {
            no_strict_list_pop(rt, RJS_FALSE, RJS_FALSE);

#if ENABLE_PRIV_NAME
            priv_env_pop(rt);
#endif /*ENABLE_PRIV_NAME*/

            parser->class_stack = c->bot;
        }

        /*Popup the declaration.*/
        if (c->decl)
            decl_pop(rt);
    }

    parser->flags = old_flags;

    if (r == RJS_OK) {
        if (!is_expr && c->binding_name)
            decl_item_add(rt, RJS_AST_DECL_CLASS, &id_loc, &c->binding_name->value, NULL);
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Parse a declaration.*/
static RJS_Result
parse_decl (RJS_Runtime *rt, RJS_Value *vd)
{
    RJS_Token  *tok;
    RJS_Result  r;
    RJS_Parser *parser    = rt->parser;
    int         old_flags = parser->flags;

    tok = get_token(rt);
    unget_token(rt);

    parser->flags &= ~RJS_PARSE_FL_DEFAULT;

    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_const)
            || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_let)) {
        parser->flags |= RJS_PARSE_FL_IN;
        r = parse_lexical_decl(rt, vd);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_class)) {
        r = parse_class_decl(rt, RJS_FALSE, vd);
    } else {
        r = parse_hoistable_decl(rt, RJS_FALSE, vd);
    }

    parser->flags = old_flags;
    return r;
}

/*Parse a statement list item.*/
static RJS_Result
parse_stmt_list_item (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token *tok, *ntok;
    RJS_Result r;

    tok = get_token(rt);

    if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_const)
            || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_function)
            || token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_class)) {
        unget_token(rt);
        r = parse_decl(rt, vs);
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_let)) {
        ntok = next_token_div(rt);
        unget_token(rt);
        if ((ntok->type == RJS_TOKEN_lbrace)
                || (ntok->type == RJS_TOKEN_lbracket)
                || is_binding_identifier(rt, ntok->type, ntok->flags)) {
            r = parse_decl(rt, vs);
        } else {
            r = parse_stmt(rt, vs);
        }
    } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_async)) {
        ntok = next_token_div(rt);
        unget_token(rt);
        if (token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_function)
                && (tok->location.last_line == ntok->location.first_line)) {
            r = parse_decl(rt, vs);
        } else {
            r = parse_stmt(rt, vs);
        }
    } else {
        unget_token(rt);
        r = parse_stmt(rt, vs);
    }

    return r;
}

#if ENABLE_MODULE

/*Add a new module request entry.*/
static RJS_AstModuleRequest*
module_request_add (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *name)
{
    RJS_Parser           *parser = rt->parser;
    size_t                top    = rjs_value_stack_save(rt);
    RJS_Value            *tmp    = rjs_value_stack_push(rt);
    RJS_HashEntry        *he, **phe;
    RJS_AstModuleRequest *mr;

    he = hash_lookup(rt, &parser->module_request_hash, name, &phe);
    if (he) {
        mr = RJS_CONTAINER_OF(he, RJS_AstModuleRequest, he);
        goto end;
    }

    mr = ast_new(rt, tmp, RJS_AST_ModuleRequest, loc);

    mr->name = value_entry_add(rt, loc, name);
    mr->id   = parser->module_request_hash.entry_num;

    hash_insert(rt, &parser->module_request_hash, name, &mr->he, phe);
    ast_list_append(rt, &parser->module_request_list, tmp);
end:
    rjs_value_stack_restore(rt, top);
    return mr;
}

/*Add an import entry to the hash table.*/
static void
hash_add_import (RJS_Runtime *rt, RJS_Hash *hash, RJS_AstImport *ie)
{
    RJS_HashEntry *he, **phe;

    he = hash_lookup(rt, hash, &ie->local_name->value, &phe);
    if (he) {
        RJS_AstImport *old;

        old = RJS_CONTAINER_OF(he, RJS_AstImport, he);

        parse_error(rt, &ie->ast.location, _("import name \"%s\" is already defined"),
                rjs_string_to_enc_chars(rt, &ie->local_name->value, NULL, NULL));
        parse_prev_define_note(rt, &old->ast.location);
    } else {
        hash_insert(rt, hash, &ie->local_name->value, &ie->he, phe);
    }
}

/*Allocate a new import entry.*/
static RJS_AstImport*
import_new (RJS_Runtime *rt, RJS_Location *loc, RJS_List *list)
{
    RJS_AstImport *ie;
    size_t         top = rjs_value_stack_save(rt);
    RJS_Value     *tmp = rjs_value_stack_push(rt);

    ie = ast_new(rt, tmp, RJS_AST_Import, loc);

    ie->module      = NULL;
    ie->import_name = NULL;
    ie->local_name  = NULL;

    ast_list_append(rt, list, tmp);

    rjs_value_stack_restore(rt, top);

    return ie;
}

/*Parse namespace import.*/
static RJS_Result
parse_ns_import (RJS_Runtime *rt, RJS_List *list, RJS_Hash *hash)
{
    RJS_Token     *tok;
    RJS_AstImport *ie;
    RJS_Result     r = RJS_ERR;

    tok = curr_token(rt);
    ie  = import_new(rt, &tok->location, list);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_as)) == RJS_ERR)
        goto end;

    tok = get_token(rt);
    if (!is_binding_identifier(rt, tok->type, tok->flags)) {
        parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
        r = RJS_ERR;
        goto end;
    }

    check_binding_identifier(rt, &tok->location, tok->value);

    ie->local_name = value_entry_add(rt, &tok->location, tok->value);

    hash_add_import(rt, hash, ie);

    r = RJS_OK;
end:
    return r;
}

/*Parse name imports.*/
static RJS_Result
parse_name_imports (RJS_Runtime *rt, RJS_List *list, RJS_Hash *hash)
{
    RJS_Token     *tok, *ntok;
    RJS_AstImport *ie;
    RJS_Result     r = RJS_ERR;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;

        ie = import_new(rt, &tok->location, list);

        ntok = next_token(rt);
        r    = RJS_OK;

        if (token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_as)) {
            if ((tok->type == RJS_TOKEN_STRING)
                    || (tok->type == RJS_TOKEN_IDENTIFIER)) {
                if (tok->type == RJS_TOKEN_STRING) {
                    if (tok->flags & RJS_TOKEN_FL_UNPAIRED_SURROGATE) {
                        parse_error(rt, &tok->location,
                                _("export name cannot contains unpaired surrogate character"));
                    }
                }
                ie->import_name = value_entry_add(rt, &tok->location, tok->value);
            } else {
                parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                r = RJS_ERR;
            }

            if (r == RJS_OK) {
                get_token(rt);

                tok = get_token(rt);
                if (!is_binding_identifier(rt, tok->type, tok->flags)) {
                    parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                    r = RJS_ERR;
                } else {
                    check_binding_identifier(rt, &tok->location, tok->value);

                    ie->local_name = value_entry_add(rt, &tok->location, tok->value);

                    hash_add_import(rt, hash, ie);

                    loc_update_last_token(rt, &ie->ast.location);
                }
            }
        } else {
            if (!is_binding_identifier(rt, tok->type, tok->flags)) {
                parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                r = RJS_ERR;
            } else {
                check_binding_identifier(rt, &tok->location, tok->value);

                ie->import_name = value_entry_add(rt, &tok->location, tok->value);
                ie->local_name  = ie->import_name;

                hash_add_import(rt, hash, ie);
            }
        }

        if (r == RJS_ERR) {
            recover_object(rt);
            continue;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `}\'"));
            recover_object(rt);
        }
    }

    return RJS_OK;
}

/*Parse from clause.*/
static RJS_AstModuleRequest*
parse_from_clause (RJS_Runtime *rt)
{
    RJS_Token            *tok;
    RJS_Result            r;
    RJS_AstModuleRequest *mr = NULL;

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_from)) == RJS_ERR)
        goto end;

    if ((r = get_token_expect(rt, RJS_TOKEN_STRING)) == RJS_ERR)
        goto end;

    tok = curr_token(rt);
    mr  = module_request_add(rt, &tok->location, tok->value);
end:
    return mr;
}

/*Parse import declaration.*/
static RJS_Result
parse_import_decl (RJS_Runtime *rt)
{
    RJS_Token     *tok;
    RJS_AstList   *elist;
    RJS_AstImport *ie;
    RJS_Hash       import_hash;
    RJS_Parser    *parser   = rt->parser;
    RJS_Result     r        = RJS_ERR;
    RJS_Bool       has_from = RJS_TRUE;
    size_t         top      = rjs_value_stack_save(rt);
    RJS_Value     *vlist    = rjs_value_stack_push(rt);

    hash_init(&import_hash);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_import)) == RJS_ERR)
        goto end;

    tok   = curr_token(rt);
    elist = ast_new(rt, vlist, RJS_AST_List, &tok->location);

    tok = get_token(rt);
    switch (tok->type) {
    case RJS_TOKEN_STRING:
        module_request_add(rt, &tok->location, tok->value);

        has_from = RJS_FALSE;
        break;
    case RJS_TOKEN_star:
        if ((r = parse_ns_import(rt, &elist->list, &import_hash)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_lbrace:
        if ((r = parse_name_imports(rt, &elist->list, &import_hash)) == RJS_ERR)
            goto end;
        break;
    default:
        if (is_binding_identifier(rt, tok->type, tok->flags)) {
            check_binding_identifier(rt, &tok->location, tok->value);

            ie = import_new(rt, &tok->location, &elist->list);

            ie->import_name = value_entry_add(rt, &tok->location, rjs_s_default(rt));
            ie->local_name  = value_entry_add(rt, &tok->location, tok->value);

            hash_add_import(rt, &import_hash, ie);

            tok = get_token(rt);
            if (tok->type == RJS_TOKEN_comma) {
                tok = get_token(rt);
                switch (tok->type) {
                case RJS_TOKEN_star:
                    if ((r = parse_ns_import(rt, &elist->list, &import_hash)) == RJS_ERR)
                        goto end;
                    break;
                case RJS_TOKEN_lbrace:
                    if ((r = parse_name_imports(rt, &elist->list, &import_hash)) == RJS_ERR)
                        goto end;
                    break;
                default:
                    parse_unexpect_error(rt, &tok->location, _("`*\' or `{\'"));
                    r = RJS_ERR;
                    goto end;
                }
            } else {
                unget_token(rt);
            }
        } else {
            parse_unexpect_error(rt, &tok->location, _("string, identifier, `*\' or `{\'"));
            r = RJS_ERR;
            goto end;
        }
        break;
    }

    if (has_from) {
        RJS_AstModuleRequest *mr;

        if (!(mr = parse_from_clause(rt))) {
            r = RJS_ERR;
            goto end;
        }

        rjs_list_foreach_c(&elist->list, ie, RJS_AstImport, ast.ln) {
            ie->module = mr;
        }
    }

    rjs_list_foreach_c(&elist->list, ie, RJS_AstImport, ast.ln) {
        parser->import_num ++;
    }

    rjs_list_join(&parser->import_list, &elist->list);

    if ((r = auto_semicolon(rt)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    hash_deinit(rt, &import_hash);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a new export entry.*/
static RJS_AstExport*
export_new (RJS_Runtime *rt, RJS_Location *loc, RJS_List *list)
{
    RJS_AstExport *ee;
    size_t         top = rjs_value_stack_save(rt);
    RJS_Value     *tmp = rjs_value_stack_push(rt);

    ee = ast_new(rt, tmp, RJS_AST_Export,loc);

    ee->module      = NULL;
    ee->local_name  = NULL;
    ee->import_name = NULL;
    ee->export_name = NULL;
    ee->has_str     = RJS_FALSE;

    ast_list_append(rt, list, tmp);

    rjs_value_stack_restore(rt, top);

    return ee;
}

/*Parse name exports.*/
static RJS_Result
parse_name_exports (RJS_Runtime *rt, RJS_List *list)
{
    RJS_Token     *tok;
    RJS_AstExport *ee;

    while (1) {
        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;

        ee = export_new(rt, &tok->location, list);

        if ((tok->type == RJS_TOKEN_STRING)
                || (tok->type == RJS_TOKEN_IDENTIFIER)) {
            if (tok->flags & RJS_TOKEN_FL_STRICT_RESERVED) {
                parse_error(rt, &tok->location, _("\"%s\" cannot be used in export entry"),
                        rjs_string_to_enc_chars(rt, tok->value, NULL, NULL));
            }

            if (tok->type == RJS_TOKEN_STRING) {
                ee->has_str = RJS_TRUE;

                if (tok->flags & RJS_TOKEN_FL_UNPAIRED_SURROGATE) {
                    parse_error(rt, &tok->location,
                            _("export name cannot contains unpaired surrogate character"));
                }
            }

            ee->local_name = value_entry_add(rt, &tok->location, tok->value);
        } else {
            parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
            recover_object(rt);
            continue;
        }

        tok = get_token(rt);
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_as)) {
            tok = get_token(rt);

            if ((tok->type == RJS_TOKEN_STRING) || (tok->type == RJS_TOKEN_IDENTIFIER)) {
                if (tok->type == RJS_TOKEN_STRING) {
                    if (tok->flags & RJS_TOKEN_FL_UNPAIRED_SURROGATE) {
                        parse_error(rt, &tok->location,
                                _("export name cannot contains unpaired surrogate character"));
                    }
                }

                ee->export_name = value_entry_add(rt, &tok->location, tok->value);
                loc_update_last_token(rt, &ee->ast.location);
            } else {
                parse_unexpect_token_error(rt, &tok->location, RJS_TOKEN_IDENTIFIER, 0);
                recover_object(rt);
                continue;
            }
        } else {
            unget_token(rt);

            ee->export_name = ee->local_name;
        }

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_rbrace)
            break;
        if (tok->type != RJS_TOKEN_comma) {
            parse_unexpect_error(rt, &tok->location, _("`,\' or `}\'"));
            recover_object(rt);
        }
    }

    return RJS_OK;
}

/*Add a local export entry.*/
static RJS_AstExport*
export_local (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *id, RJS_List *list)
{
    RJS_AstExport     *ee;
    RJS_AstValueEntry *ve;

    ee = export_new(rt, loc, list);

    ve = value_entry_add(rt, loc, id);

    ee->local_name  = ve;
    ee->export_name = ve;

    return ee;
}

/*Export bindings in the elements.*/
static void
export_binding_elements (RJS_Runtime *rt, RJS_AstList *elems, RJS_List *list)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(&elems->list, ast, RJS_Ast, ln) {
        switch (ast->type) {
        case RJS_AST_Elision:
        case RJS_AST_LastElision:
            break;
        case RJS_AST_Rest: {
            RJS_AstRest *rest = (RJS_AstRest*)ast;

            export_binding(rt, &rest->binding, list);
            break;
        }
        case RJS_AST_BindingElem: {
            RJS_AstBindingElem *be = (RJS_AstBindingElem*)ast;

            export_binding(rt, &be->binding, list);
            break;
        }
        default:
            assert(0);
            break;
        }
    }
}

/*Export bindings in the properties.*/
static void
export_binding_properties (RJS_Runtime *rt, RJS_AstList *elems, RJS_List *list)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(&elems->list, ast, RJS_Ast, ln) {
        switch (ast->type) {
        case RJS_AST_Rest: {
            RJS_AstRest *rest = (RJS_AstRest*)ast;

            export_binding(rt, &rest->binding, list);
            break;
        }
        case RJS_AST_BindingProp: {
            RJS_AstBindingProp *bp = (RJS_AstBindingProp*)ast;

            export_binding(rt, &bp->binding, list);
            break;
        }
        default:
            assert(0);
            break;
        }
    }
}

/*Export the binding.*/
static void
export_binding (RJS_Runtime *rt, RJS_Value *b, RJS_List *list)
{
    RJS_Ast *ast = ast_get(rt, b);

    switch (ast->type) {
    case RJS_AST_Id: {
        RJS_AstId *ir = (RJS_AstId*)ast;

        export_local(rt, &ir->ast.location, &ir->identifier->value, list);
        break;
    }
    case RJS_AST_ArrayBinding: {
        RJS_AstList *l = (RJS_AstList*)ast;

        export_binding_elements(rt, l, list);
        break;
    }
    case RJS_AST_ObjectBinding: {
        RJS_AstList *l = (RJS_AstList*)ast;

        export_binding_properties(rt, l, list);
        break;
    }
    default:
        assert(0);
        break;
    }
}

/*Export bindings in the statement.*/
static void
export_bindings_in_stmt (RJS_Runtime *rt, RJS_Value *vstmt, RJS_List *list)
{
    RJS_Ast *ast;

    ast = ast_get(rt, vstmt);

    switch (ast->type) {
    case RJS_AST_VarDecl:
    case RJS_AST_LetDecl:
    case RJS_AST_ConstDecl: {
        RJS_AstList *l = (RJS_AstList*)ast;

        export_binding_elements(rt, l, list);
        break;
    }
    case RJS_AST_FuncDecl: {
        RJS_AstFuncRef *fr = (RJS_AstFuncRef*)ast;

        export_local(rt, &fr->ast.location, &fr->func->name->value, list);
        break;
    }
    case RJS_AST_ClassDecl: {
        RJS_AstClassRef *cr = (RJS_AstClassRef*)ast;

        export_local(rt, &cr->ast.location, &cr->clazz->name->value, list);
        break;
    }
    default:
        break;
    }
}

/*Parse export declaration.*/
static RJS_Result
parse_export_decl (RJS_Runtime *rt, RJS_Value *vs)
{
    RJS_Token            *tok, *ntok;
    RJS_AstExport        *ee, *nee;
    RJS_AstList          *elist;
    RJS_AstModuleRequest *mr;
    RJS_Result            r         = RJS_ERR;
    RJS_Parser           *parser    = rt->parser;
    int                   old_flags = parser->flags;
    size_t                top       = rjs_value_stack_save(rt);
    RJS_Value            *vlist     = rjs_value_stack_push(rt);

    rjs_value_set_undefined(rt, vs);

    if ((r = get_identifier_expect(rt, RJS_IDENTIFIER_export)) == RJS_ERR)
        goto end;

    tok   = curr_token(rt);
    elist = ast_new(rt, vlist, RJS_AST_List, &tok->location);

    tok = get_token(rt);
    switch (tok->type) {
    case RJS_TOKEN_star:
        ee = export_new(rt, &tok->location, &elist->list);

        tok = get_token(rt);
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_as)) {
            tok = get_token(rt);

            if ((tok->type == RJS_TOKEN_STRING) || (tok->type == RJS_TOKEN_IDENTIFIER)) {
                if (tok->type == RJS_TOKEN_STRING) {
                    if (tok->flags & RJS_TOKEN_FL_UNPAIRED_SURROGATE) {
                        parse_error(rt, &tok->location,
                                _("export name cannot contains unpaired surrogate character"));
                    }
                }
                ee->export_name = value_entry_add(rt, &tok->location, tok->value);
            } else {
                parse_unexpect_error(rt, &tok->location, _("string or identifier"));
                r = RJS_ERR;
                goto end;
            }
        } else {
            unget_token(rt);
        }

        if (!(mr = parse_from_clause(rt))) {
            r = RJS_ERR;
            goto end;
        }

        ee->module = mr;

        loc_update_last_token(rt, &ee->ast.location);

        if ((r = auto_semicolon(rt)) == RJS_ERR)
            goto end;
        break;
    case RJS_TOKEN_lbrace:
        if ((r = parse_name_exports(rt, &elist->list)) == RJS_ERR)
            goto end;

        tok = get_token(rt);
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_from)) {
            unget_token(rt);
            if (!(mr = parse_from_clause(rt))) {
                r = RJS_ERR;
                goto end;
            }

            rjs_list_foreach_c(&elist->list, ee, RJS_AstExport, ast.ln) {
                ee->import_name = ee->local_name;
                ee->local_name  = NULL;
                ee->module      = mr;
            }
        } else {
            RJS_AstExport *ee;

            unget_token(rt);

            rjs_list_foreach_c(&elist->list, ee, RJS_AstExport, ast.ln) {
                if (ee->has_str)
                    parse_error(rt, &ee->local_name->ast.location, _("cannot use string literal as local name"));
            }
        }

        if ((r = auto_semicolon(rt)) == RJS_ERR)
            goto end;
        break;
    default:
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_default)) {
            ee = export_new(rt, &tok->location, &elist->list);

            ee->export_name = value_entry_add(rt, &tok->location, rjs_s_default(rt));

            tok = get_token(rt);
            if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_class)) {
                RJS_AstClassRef *cr;

                unget_token(rt);
                parser->flags |= RJS_PARSE_FL_DEFAULT;
                if ((r = parse_class_decl(rt, RJS_FALSE, vs)) == RJS_ERR)
                    goto end;

                cr = ast_get(rt, vs);
                ee->local_name = cr->clazz->binding_name;

                loc_update_last_ast(rt, &ee->ast.location, vs);
            } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_function)) {
                RJS_AstFuncRef *fr;

                unget_token(rt);
                parser->flags |= RJS_PARSE_FL_DEFAULT;
                if ((r = parse_hoistable_decl(rt, RJS_FALSE, vs)) == RJS_ERR)
                    goto end;

                fr = ast_get(rt, vs);
                ee->local_name = fr->func->binding_name;

                loc_update_last_ast(rt, &ee->ast.location, vs);
            } else {
                RJS_Bool matched = RJS_FALSE;

                if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_async)) {
                    ntok = next_token_div(rt);
                    if (token_is_identifier(ntok->type, ntok->flags, RJS_IDENTIFIER_function)
                            && (ntok->location.first_line == tok->location.last_line)) {
                        RJS_AstFuncRef *fr;

                        unget_token(rt);
                        parser->flags |= RJS_PARSE_FL_DEFAULT;
                        if ((r = parse_hoistable_decl(rt, RJS_FALSE, vs)) == RJS_ERR)
                            goto end;

                        fr = ast_get(rt, vs);
                        ee->local_name = fr->func->binding_name;

                        loc_update_last_ast(rt, &ee->ast.location, vs);
                        matched = RJS_TRUE;
                    }
                }

                if (!matched) {
                    RJS_AstExprStmt *stmt;
                    RJS_Ast         *expr;

                    /*Add "*default*" binding.*/
                    decl_item_add(rt, RJS_AST_DECL_CONST, &tok->location, rjs_s_star_default_star(rt), NULL);

                    /*Add default expression statement.*/
                    stmt = ast_new(rt, vs, RJS_AST_DefaultExprStmt, &tok->location);

                    unget_token(rt);
                    if ((r = parse_expr_in_prio(rt, PRIO_ASSI, &stmt->expr)) == RJS_ERR)
                        goto end;

                    check_expr(rt, &stmt->expr);

                    ee->local_name = value_entry_add(rt, &ee->ast.location, rjs_s_star_default_star(rt));

                    loc_update_last_ast(rt, &ee->ast.location, vs);

                    expr = ast_get(rt, &stmt->expr);

                    /*Set the function name as "default".*/
                    func_set_name(rt, expr, &ee->local_name->ast.location, rjs_s_default(rt));

                    if ((r = auto_semicolon(rt)) == RJS_ERR)
                        goto end;
                }
            }
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_var)) {
            unget_token(rt);
            if ((r = parse_var_stmt(rt, vs)) == RJS_ERR)
                goto end;

            export_bindings_in_stmt(rt, vs, &elist->list);
        } else {
            unget_token(rt);
            if ((r = parse_decl(rt, vs)) == RJS_ERR)
                goto end;

            export_bindings_in_stmt(rt, vs, &elist->list);
        }
    }

    /*Add export entries to the list.*/
    rjs_list_foreach_safe_c(&elist->list, ee, nee, RJS_AstExport, ast.ln) {
        /*Check if the export name is already used.*/
        if (ee->export_name) {
            RJS_HashEntry *he, **phe;

            he = hash_lookup(rt, &parser->export_hash, &ee->export_name->value, &phe);
            if (he) {
                RJS_AstExport *old;

                old = RJS_CONTAINER_OF(he, RJS_AstExport, he);

                parse_error(rt, &ee->ast.location, _("export name \"%s\" is already defined"),
                        rjs_string_to_enc_chars(rt, &ee->export_name->value, NULL, NULL));
                parse_prev_define_note(rt, &old->ast.location);
            } else {
                hash_insert(rt, &parser->export_hash, &ee->export_name->value, &ee->he, phe);
            }
        }

        /*Add the entry to the list.*/
        if (!ee->module) {
            rjs_list_append(&parser->local_export_list, &ee->ast.ln);
            parser->local_export_num ++;
        } else if (!ee->export_name) {
            rjs_list_append(&parser->star_export_list, &ee->ast.ln);
            parser->star_export_num ++;
        } else {
            rjs_list_append(&parser->indir_export_list, &ee->ast.ln);
            parser->indir_export_num ++;
        }
    }

    r = RJS_OK;
end:
    parser->flags = old_flags;
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_MODULE*/

/**
 * Scan the referenced GC things in the parser of the rt.
 * \param rt The current runtime.
 * \param parser The parser to be scanned.
 */
void
rjs_gc_scan_parser (RJS_Runtime *rt, RJS_Parser *parser)
{
    RJS_AstNoStrictListStack *ils;
    RJS_AstContainsListStack *cls;

#if ENABLE_PRIV_NAME
    gc_scan_ast_list(rt, &parser->priv_env_list);
    
    if (parser->bot_priv_env)
        rjs_gc_mark(rt, parser->bot_priv_env);

#endif /*ENABLE_PRIV_NAME*/

    for (ils = parser->no_strict_list_stack; ils; ils = ils->bot)
        rjs_gc_mark(rt, ils);

    for (cls = parser->contains_list_stack; cls; cls = cls->bot)
        rjs_gc_mark(rt, cls);

    gc_scan_ast_list(rt, &parser->func_list);
    gc_scan_ast_list(rt, &parser->decl_list);
    gc_scan_ast_list(rt, &parser->class_list);
    gc_scan_ast_list(rt, &parser->value_entry_list);
    gc_scan_ast_list(rt, &parser->binding_table_list);
    gc_scan_ast_list(rt, &parser->func_table_list);
    gc_scan_ast_list(rt, &parser->prop_ref_list);

#if ENABLE_MODULE
    gc_scan_ast_list(rt, &parser->module_request_list);
    gc_scan_ast_list(rt, &parser->import_list);
    gc_scan_ast_list(rt, &parser->local_export_list);
    gc_scan_ast_list(rt, &parser->indir_export_list);
    gc_scan_ast_list(rt, &parser->star_export_list);
#endif /*ENABLE_MODULE*/
}

/**
 * Create a new template entry.
 * \param rt The current runtime.
 * \param loc The location of the template entry.
 * \param str The string value.
 * \param raw The raw string value.
 * \param tok The token to store the template entry.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_template_entry_new (RJS_Runtime *rt, RJS_Location *loc,
        RJS_Value *str, RJS_Value *raw, RJS_Token *tok)
{
    RJS_AstTemplateEntry *te;

    te = ast_new(rt, tok->value, RJS_AST_TemplateEntry, loc);

    rjs_value_copy(rt, &te->str, str);
    rjs_value_copy(rt, &te->raw_str, raw);

    return RJS_OK;
}

#include "rjs_code_gen_inc.c"

#if ENABLE_SCRIPT

/*Parse the script.*/
static RJS_Result
parse_script (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_AstFunc *func;
    RJS_Result   r;
    RJS_Parser  *parser       = rt->parser;
    int          old_flags    = parser->flags;
    size_t       top          = rjs_value_stack_save(rt);
    RJS_Value   *tmp          = rjs_value_stack_push(rt);
    RJS_Bool     check_direct = RJS_TRUE;

    contains_list_push(rt);

    func = func_push(rt, NULL);
    func->flags |= RJS_AST_FUNC_FL_SCRIPT;

    no_strict_list_push(rt);

    func_body(rt);

    while (1) {
        RJS_Token *tok;

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_END)
            break;

        unget_token(rt);
        r = parse_stmt_list_item(rt, tmp);

        if (r == RJS_ERR) {
            recover_stmt(rt, RECOVER_SCRIPT);
        } else {
            ast_list_append(rt, &func->stmt_list, tmp);

            if (check_direct) {
                if (!is_directive_prologue(rt, tmp))
                    check_direct = RJS_FALSE;
            }
        }
    }

    parser->flags = old_flags;

    no_strict_list_pop(rt, RJS_TRUE, RJS_FALSE);
    func_pop(rt);

#if ENABLE_PRIV_NAME
    check_priv_ids(rt);
#endif /*ENABLE_PRIV_NAME*/

    contains_list_check(rt, CONTAINS_FL_NEW_TARGET|CONTAINS_FL_SUPER_CALL|CONTAINS_FL_SUPER_PROP);

    r = parser_has_error(rt) ? RJS_ERR : RJS_OK;
    if (r == RJS_OK)
        r = gen_script(rt, realm, rv);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Parse the script.
 * \param rt The current runtime.
 * \param input The input.
 * \param realm The realm.
 * \param flags The parser flags.
 * \param[out] rv The return script value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_parse_script (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, int flags, RJS_Value *rv)
{
    RJS_Parser parser;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);

    parser_init(rt, &parser, input);

    parser.flags |= flags;

    r = parse_script(rt, realm, rv);

    parser_deinit(rt);

    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_SCRIPT*/

/*Parse a function.*/
static RJS_Result
parse_function (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_Result   r;
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *tmp = rjs_value_stack_push(rt);

    parse_hoistable_decl(rt, RJS_FALSE, tmp);

#if ENABLE_PRIV_NAME
    check_priv_ids(rt);
#endif /*ENABLE_PRIV_NAME*/

    r = parser_has_error(rt) ? RJS_ERR : RJS_OK;
    if (r == RJS_OK)
        r = gen_script(rt, realm, rv);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Parse a function.
 * \param rt The current runtime.
 * \param input The input.
 * \param realm The realm.
 * \param[out] rv Return the script value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_parse_function (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_Parser parser;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);

    parser_init(rt, &parser, input);
    r = parse_function(rt, realm, rv);
    parser_deinit(rt);

    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_EVAL

/*Parse the eval code.*/
static RJS_Result
parse_eval (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_AstFunc *func;
    RJS_Result   r;
    RJS_Parser  *parser       = rt->parser;
    int          old_flags    = parser->flags;
    size_t       top          = rjs_value_stack_save(rt);
    RJS_Value   *tmp          = rjs_value_stack_push(rt);
    RJS_Bool     check_direct = RJS_TRUE;
    int          cflags       = 0;
    
    contains_list_push(rt);

    func = func_push(rt, NULL);
    func->flags |= RJS_AST_FUNC_FL_EVAL;

    no_strict_list_push(rt);

    func_body(rt);

    while (1) {
        RJS_Token *tok;

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_END)
            break;

        unget_token(rt);
        r = parse_stmt_list_item(rt, tmp);

        if (r == RJS_ERR) {
            recover_stmt(rt, RECOVER_SCRIPT);
        } else {
            ast_list_append(rt, &func->stmt_list, tmp);

            if (check_direct) {
                if (!is_directive_prologue(rt, tmp))
                    check_direct = RJS_FALSE;
            }
        }
    }

    parser->flags = old_flags;

    no_strict_list_pop(rt, RJS_TRUE, RJS_FALSE);
    func_pop(rt);

#if ENABLE_PRIV_NAME
    check_priv_ids(rt);
#endif /*ENABLE_PRIV_NAME*/

    cflags = CONTAINS_FL_NEW_TARGET
            |CONTAINS_FL_SUPER_CALL
            |CONTAINS_FL_SUPER_PROP
            |CONTAINS_FL_ARGUMENTS;

    if (parser->flags & RJS_PARSE_FL_NEW_TARGET)
        cflags &= ~CONTAINS_FL_NEW_TARGET;
    if (parser->flags & RJS_PARSE_FL_SUPER_CALL)
        cflags &= ~CONTAINS_FL_SUPER_CALL;
    if (parser->flags & RJS_PARSE_FL_SUPER_PROP)
        cflags &= ~CONTAINS_FL_SUPER_PROP;
    if (parser->flags & RJS_PARSE_FL_ARGS)
        cflags &= ~CONTAINS_FL_ARGUMENTS;

    if (cflags)
        contains_list_check(rt, cflags);

    r = parser_has_error(rt) ? RJS_ERR : RJS_OK;
    if (r == RJS_OK)
        r = gen_script(rt, realm, rv);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Parse the script for "eval".
 * \param rt The current runtime.
 * \param input The source input.
 * \param realm The realm.
 * \param flags The parser flags.
 * \param priv_env The private environment.
 * \param[out] Return the script.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_parse_eval (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, int flags,
        RJS_PrivateEnv *priv_env, RJS_Value *rv)
{
    RJS_Parser parser;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);

    parser_init(rt, &parser, input);

    parser.flags = flags|RJS_PARSE_FL_EVAL;

#if ENABLE_PRIV_NAME
    if (priv_env) {
        parser.bot_priv_env = priv_env;
        parser.flags |= RJS_PARSE_FL_CLASS;
    }
#endif /*ENABLE_PRIV_NAME*/

    r = parse_eval(rt, realm, rv);

    parser_deinit(rt);

    rjs_value_stack_restore(rt, top);
    return r;
}
#endif /*ENABLE_EVAL*/

#if ENABLE_MODULE

/*Parse the module.*/
static RJS_Result
parse_module (RJS_Runtime *rt, const char *id, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_AstFunc   *func;
    RJS_Result     r;
    RJS_AstExport *ee;
    RJS_Parser    *parser    = rt->parser;
    int            old_flags = parser->flags;
    size_t         top       = rjs_value_stack_save(rt);
    RJS_Value     *tmp       = rjs_value_stack_push(rt);

    contains_list_push(rt);

    parser->flags |= RJS_PARSE_FL_STRICT|RJS_PARSE_FL_MODULE|RJS_PARSE_FL_AWAIT;

    func = func_push(rt, NULL);
    func->flags |= RJS_AST_FUNC_FL_MODULE;

    no_strict_list_push(rt);

    func_body(rt);

    while (1) {
        RJS_Token *tok;

        tok = get_token(rt);
        if (tok->type == RJS_TOKEN_END)
            break;

        unget_token(rt);
        if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_import)) {
            RJS_Token *ntok = next_token(rt);

            if ((ntok->type != RJS_TOKEN_dot) && (ntok->type != RJS_TOKEN_lparenthese)) {
                r = parse_import_decl(rt);
            } else {
                r = parse_stmt_list_item(rt, tmp);
                if (r == RJS_OK)
                    ast_list_append(rt, &func->stmt_list, tmp);
            }
        } else if (token_is_identifier(tok->type, tok->flags, RJS_IDENTIFIER_export)) {
            r = parse_export_decl(rt, tmp);
            if (r == RJS_OK)
                ast_list_append(rt, &func->stmt_list, tmp);
        } else {
            r = parse_stmt_list_item(rt, tmp);
            if (r == RJS_OK)
                ast_list_append(rt, &func->stmt_list, tmp);
        }

        if (r == RJS_ERR)
            recover_stmt(rt, RECOVER_MODULE);
    }

    parser->flags = old_flags;

    /*Check if the local export names is defined.*/
    rjs_list_foreach_c(&parser->local_export_list, ee, RJS_AstExport, ast.ln) {
        RJS_HashEntry *he;
        RJS_Value     *name = &ee->local_name->value;

        /*Is default?*/
        if (rjs_string_equal(rt, name, rjs_s_default(rt)))
            continue;

        /*Is defined in variable or lexical declaration.*/
        he = hash_lookup(rt, &func->lex_decl->item_hash, name, NULL);
        if (!he) {
            RJS_AstImport *ie;
            RJS_Bool       found = RJS_FALSE;

            /*Is defined as import local name.*/
            rjs_list_foreach_c(&parser->import_list, ie, RJS_AstImport, ast.ln) {
                if (rjs_string_equal(rt, name, &ie->local_name->value)) {
                    found = RJS_TRUE;
                    break;
                }
            }

            if (!found) {
                parse_error(rt, &ee->ast.location,
                        _("\"%s\" is not defined"),
                        rjs_string_to_enc_chars(rt, name, NULL, NULL));
            }
        }
    }

    no_strict_list_pop(rt, RJS_TRUE, RJS_FALSE);
    func_pop(rt);

#if ENABLE_PRIV_NAME
    check_priv_ids(rt);
#endif /*ENABLE_PRIV_NAME*/

    contains_list_check(rt, CONTAINS_FL_NEW_TARGET|CONTAINS_FL_SUPER_CALL|CONTAINS_FL_SUPER_PROP);

    r = parser_has_error(rt) ? RJS_ERR : RJS_OK;
    if (r == RJS_OK)
        r = gen_module(rt, id, realm, rv);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Parse the module.
 * \param rt The current runtime.
 * \param input The input.
 * \param id The identifier of the module.
 * \param realm The realm.
 * \param[out] rv The return module value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_parse_module (RJS_Runtime *rt, RJS_Input *input, const char *id, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_Parser parser;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);

    parser_init(rt, &parser, input);
    r = parse_module(rt, id, realm, rv);
    parser_deinit(rt);

    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_MODULE*/
