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

/**Reference type.*/
typedef enum {
    RJS_BC_REF_BINDING,  /**< Binding.*/
    RJS_BC_REF_PROPERTY, /**< Property.*/
    RJS_BC_REF_SUPER,    /**< Super property.*/
    RJS_BC_REF_VALUE,    /**< Value.*/
#if ENABLE_PRIV_NAME
    RJS_BC_REF_PRIVATE   /**< Private name.*/
#endif /*ENABLE_PRIV_NAME*/
} RJS_BcRefType;

/**Reference.*/
typedef struct {
    RJS_BcRefType      type;         /**< Reference type.*/
    RJS_AstBindingRef *binding_ref;  /**< The binding reference.*/
    int                env_rid;      /**< The environment register's index.*/
    int                base_rid;     /**< Base value register's index.*/
    RJS_AstPropRef    *ref_name_prop;/**< The property.*/
    int                ref_name_rid; /**< reference name register's index.*/
    int                value_rid;    /**< Value register's index.*/
    RJS_Bool           opt_base;     /**< Is an optional base.*/
    RJS_Bool           opt_expr;     /**< Is an optional expression.*/
    int                old_opt_reg;  /**< Old optional result register.*/
    int                old_opt_label;/**< Old optional end label.*/
} RJS_BcRef;

/*Generate a statement.*/
static RJS_Result
bc_gen_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *stmt);
/*Generate the statement list.*/
static RJS_Result
bc_gen_stmt_list (RJS_Runtime *rt, RJS_BcGen *bg, RJS_List *list);
/*Generate binding assignment reference.*/
static RJS_Result
bc_gen_binding_assi_ref (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *b, RJS_BcRef *ref);
/*Generate binding assignment.*/
static RJS_Result
bc_gen_binding_assi (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *b, int rid, RJS_Bool is_lex, RJS_BcRef *ref);
/*Generate an expression.*/
static RJS_Result
bc_gen_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *expr, int rid);
/*Generate a class.*/
static RJS_Result
bc_gen_class (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstClassRef *cr, int rr);
/*Generate default inintializer.*/
static RJS_Result
bc_gen_default_init (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *init, int rid);
/*Generate assignment reference.*/
static RJS_Result
bc_gen_assi_ref (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *lh, RJS_BcRef *ref);
/*Generate assignment.*/
static RJS_Result
bc_gen_assi (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *lh, int rid, RJS_BcRef *ref);
/*Generate property name.*/
static RJS_Result
bc_gen_prop_name (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *ast, int rr);

/*Output error message.*/
static void
bc_error (RJS_Runtime *rt, const char *fmt, ...)
{
    RJS_Parser *parser = rt->parser;
    va_list     ap;

    va_start(ap, fmt);
    rjs_message_v(rt, parser->lex.input, RJS_MESSAGE_ERROR, NULL,
            fmt, ap);
    va_end(ap);
}

/*Get the AST from the value.*/
static void*
bc_ast_get (RJS_Runtime *rt, RJS_Value *v)
{
    if (rjs_value_is_undefined(rt, v))
        return NULL;

    return (RJS_Ast*)rjs_value_get_gc_thing(rt, v);
}

/*Add a new command with stack depth update.*/
static int
bc_cmd_add_stack_check (RJS_Runtime *rt, RJS_BcGen *bg, RJS_BcType type, int line, RJS_Bool stack_update)
{
    RJS_BcCommand *cmd;
    int            cid = bg->cmd.item_num;

    rjs_vector_resize(&bg->cmd, bg->cmd.item_num + 1, rt);

    cmd = &bg->cmd.items[cid];

    cmd->gen.type = type;
    cmd->gen.line = line;

    if (stack_update) {
        switch (cmd->type) {
        case RJS_BC_push_lex_env:
        case RJS_BC_push_enum:
        case RJS_BC_push_iter:
        case RJS_BC_push_async_iter:
        case RJS_BC_push_class:
        case RJS_BC_push_call:
        case RJS_BC_push_super_call:
        case RJS_BC_push_new:
        case RJS_BC_push_concat:
        case RJS_BC_push_array_assi:
        case RJS_BC_push_object_assi:
        case RJS_BC_push_new_array:
        case RJS_BC_push_new_object:
        case RJS_BC_push_with:
        case RJS_BC_push_try:
        case RJS_BC_restore_lex_env:
            bg->stack_depth ++;
            break;
        case RJS_BC_pop_state:
        case RJS_BC_call:
        case RJS_BC_super_call:
        case RJS_BC_new:
        case RJS_BC_save_lex_env:
#if ENABLE_EVAL
        case RJS_BC_eval:
#endif /*ENABLE_EVAL*/
            bg->stack_depth --;
            break;
        default:
            break;
        }
    }

    return cid;
}

/*Add a new command with stack depth update.*/
static int
bc_cmd_add (RJS_Runtime *rt, RJS_BcGen *bg, RJS_BcType type, int line)
{
    return bc_cmd_add_stack_check(rt, bg, type, line, RJS_TRUE);
}

/*Get the command from its index.*/
static RJS_BcCommand*
bc_cmd_get (RJS_BcGen *bg, int cid)
{
    return &bg->cmd.items[cid];
}

/*Add a new register.*/
static int
bc_reg_add (RJS_Runtime *rt, RJS_BcGen *bg)
{
    RJS_BcRegister *reg;
    int             rid = bg->reg.item_num;

    rjs_vector_resize(&bg->reg, bg->reg.item_num + 1, rt);

    reg = &bg->reg.items[rid];
    reg->id           = -1;
    reg->last_acc_off = -1;

    return rid;
}

/*Add a new label.*/
static int
bc_label_add (RJS_Runtime *rt, RJS_BcGen *bg)
{
    RJS_BcLabel *lab;
    int          lid = bg->label.item_num;

    rjs_vector_resize(&bg->label, bg->label.item_num + 1, rt);

    lab = &bg->label.items[lid];
    lab->cmd_off     = 0;
    lab->stack_depth = bg->stack_depth;

    return lid;
}

/*Generate load expression.*/
static RJS_Result
bc_gen_load_expr (RJS_Runtime *rt, RJS_BcGen *bg, int line, int rid, RJS_BcType type)
{
    int            cid;
    RJS_BcCommand *cmd;

    cid  = bc_cmd_add(rt, bg, type, line);
    cmd  = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

/*Generate get binding expression.*/
static RJS_Result
bc_gen_get_binding_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstId *id, int rid)
{
    int                line;
    int                cid;
    int                er;
    RJS_AstBindingRef *br;
    RJS_BcCommand     *cmd;

    line = id->ast.location.first_line;

    er = bc_reg_add(rt, bg);
    br = rjs_code_gen_binding_ref(rt, &id->ast.location,
            &id->identifier->value);

    cid  = bc_cmd_add(rt, bg, RJS_BC_binding_resolve, line);
    cmd  = bc_cmd_get(bg, cid);
    cmd->binding_resolve.binding = br;
    cmd->binding_resolve.env     = er;

    cid  = bc_cmd_add(rt, bg, RJS_BC_binding_get, line);
    cmd  = bc_cmd_get(bg, cid);
    cmd->binding_get.env     = er;
    cmd->binding_get.binding = br;
    cmd->binding_get.dest    = rid;

    return RJS_OK;
}

/*Generate value expression.*/
static RJS_Result
bc_gen_value_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstValueExpr *expr, int rid)
{
    int            line;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_BcType     type;

    line = expr->ast.location.first_line;

    if (rjs_value_get_gc_thing_type(rt, &expr->ve->value) == RJS_GC_THING_REGEXP)
        type = RJS_BC_load_regexp;
    else
        type = RJS_BC_load_value;

    cid  = bc_cmd_add(rt, bg, type, line);
    cmd  = bc_cmd_get(bg, cid);
    cmd->load_value.value = expr->ve;
    cmd->load_value.dest  = rid;

    return RJS_OK;
}

/*Generate void expression.*/
static RJS_Result
bc_gen_void_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int      t_rid;
    RJS_Ast *ast;

    t_rid = bc_reg_add(rt, bg);
    ast   = bc_ast_get(rt, &expr->operand);

    bc_gen_expr(rt, bg, ast, t_rid);

    return bc_gen_load_expr(rt, bg, ast->location.first_line, rid, RJS_BC_load_undefined);
}

/*Generate unary expression.*/
static RJS_Result
bc_gen_unary_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid, RJS_BcType type)
{
    int            t_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    t_rid = bc_reg_add(rt, bg);
    ast   = bc_ast_get(rt, &expr->operand);

    bc_gen_expr(rt, bg, ast, t_rid);

    cid = bc_cmd_add(rt, bg, type, ast->location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = t_rid;
    cmd->unary.result  = rid;

    return RJS_OK;
}

#if ENABLE_GENERATOR

/*Generate yield expression.*/
static RJS_Result
bc_gen_yield_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int            t_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    t_rid = bc_reg_add(rt, bg);
    ast   = bc_ast_get(rt, &expr->operand);

    if (ast) {
        bc_gen_expr(rt, bg, ast, t_rid);
    } else {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, expr->ast.location.first_line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = t_rid;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_yield, expr->ast.location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = t_rid;

    cid = bc_cmd_add(rt, bg, RJS_BC_yield_resume, expr->ast.location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

/*Generate yield iterator expression.*/
static RJS_Result
bc_gen_yield_star_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int            t_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    t_rid = bc_reg_add(rt, bg);
    ast   = bc_ast_get(rt, &expr->operand);

    bc_gen_expr(rt, bg, ast, t_rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_yield_iter_start, ast->location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = t_rid;

    cid = bc_cmd_add(rt, bg, RJS_BC_yield_iter_next, ast->location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC

/*Generate yield expression.*/
static RJS_Result
bc_gen_await_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int            t_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    t_rid = bc_reg_add(rt, bg);
    ast   = bc_ast_get(rt, &expr->operand);

    bc_gen_expr(rt, bg, ast, t_rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_await, ast->location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = t_rid;

    cid = bc_cmd_add(rt, bg, RJS_BC_await_resume, ast->location.first_line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

#endif /*ENABLE_ASYNC*/

/*Generate binding set command.*/
static RJS_Result
bc_gen_binding_set (RJS_Runtime *rt, RJS_BcGen *bg, int line, int rid, RJS_BcRef *ref)
{
    int            cid;
    RJS_BcCommand *cmd;

    cid = bc_cmd_add(rt, bg, RJS_BC_binding_set, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->binding_set.env     = ref->env_rid;
    cmd->binding_set.binding = ref->binding_ref;
    cmd->binding_set.value   = rid;

    return RJS_OK;
}

/*Generate binding initialize command.*/
static RJS_Result
bc_gen_binding_init (RJS_Runtime *rt, RJS_BcGen *bg, int line, RJS_AstBindingRef *br, int rid)
{
    int            er;
    int            cid;
    RJS_BcCommand *cmd;

    er = bc_reg_add(rt, bg);

    cid = bc_cmd_add(rt, bg, RJS_BC_top_lex_env, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = er;

    cid = bc_cmd_add(rt, bg, RJS_BC_binding_init, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->binding_init.env     = er;
    cmd->binding_init.binding = br;
    cmd->binding_init.value   = rid;

    return RJS_OK;
}

/*Generate reference.*/
static RJS_Result
bc_gen_ref (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *expr, int rid, RJS_BcRef *ref)
{
    int            cid;
    int            line;
    RJS_BcCommand *cmd;

    ref->opt_base = RJS_FALSE;
    ref->opt_expr = RJS_FALSE;

    switch (expr->type) {
    case RJS_AST_Id: {
        RJS_AstId *ir = (RJS_AstId*)expr;

        ref->type = RJS_BC_REF_BINDING;

        ref->env_rid     = bc_reg_add(rt, bg);
        ref->binding_ref = rjs_code_gen_binding_ref(rt, &expr->location, &ir->identifier->value);

        line = expr->location.first_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_binding_resolve, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_resolve.binding = ref->binding_ref;
        cmd->binding_resolve.env     = ref->env_rid;
        break;
    }
    case RJS_AST_MemberExpr: {
        RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)expr;
        RJS_Ast           *ast;

        ref->type          = RJS_BC_REF_PROPERTY;
        ref->base_rid      = bc_reg_add(rt, bg);
        ref->ref_name_prop = NULL;
        ref->ref_name_rid  = -1;

        ast = bc_ast_get(rt, &be->operand1);
        bc_gen_expr(rt, bg, ast, ref->base_rid);

        ast  = bc_ast_get(rt, &be->operand2);
        line = ast->location.first_line;

        if (ast->type == RJS_AST_PropRef) {
            RJS_AstPropRef *pr = (RJS_AstPropRef*)ast;

            ref->ref_name_prop = pr;

            cid = bc_cmd_add(rt, bg, RJS_BC_require_object, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = ref->base_rid;
        } else {
            int tmp_rid = bc_reg_add(rt, bg);

            ref->ref_name_rid = bc_reg_add(rt, bg);

            bc_gen_expr(rt, bg, ast, tmp_rid);

            cid = bc_cmd_add(rt, bg, RJS_BC_require_object, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = ref->base_rid;

            cid = bc_cmd_add(rt, bg, RJS_BC_to_prop, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->unary.operand = tmp_rid;
            cmd->unary.result  = ref->ref_name_rid;
        }
        break;
    }
    case RJS_AST_SuperMemberExpr: {
        RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)expr;
        RJS_Ast           *ast;

        ref->type          = RJS_BC_REF_SUPER;
        ref->base_rid      = bc_reg_add(rt, bg);
        ref->ref_name_prop = NULL;
        ref->ref_name_rid  = -1;

        line = be->ast.location.first_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_load_this, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = ref->base_rid;

        ast = bc_ast_get(rt, &be->operand2);
        if (ast->type == RJS_AST_PropRef) {
            RJS_AstPropRef *pr = (RJS_AstPropRef*)ast;

            ref->ref_name_prop = pr;
        } else {
            int tmp_rid = bc_reg_add(rt, bg);

            ref->ref_name_rid = bc_reg_add(rt, bg);

            bc_gen_expr(rt, bg, ast, tmp_rid);

            line = ast->location.first_line;

            cid = bc_cmd_add(rt, bg, RJS_BC_to_prop, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->unary.operand = tmp_rid;
            cmd->unary.result  = ref->ref_name_rid;
        }
        break;
    }
#if ENABLE_PRIV_NAME
    case RJS_AST_PrivMemberExpr: {
        RJS_AstBinaryExpr *be = (RJS_AstBinaryExpr*)expr;
        RJS_Ast           *ast;
        RJS_AstPrivIdRef  *pir;

        ref->type          = RJS_BC_REF_PRIVATE;
        ref->base_rid      = bc_reg_add(rt, bg);
        ref->ref_name_prop = NULL;
        ref->ref_name_rid  = -1;

        ast = bc_ast_get(rt, &be->operand1);
        bc_gen_expr(rt, bg, ast, ref->base_rid);

        pir = bc_ast_get(rt, &be->operand2);
        ref->ref_name_prop = bc_ast_get(rt, &pir->prop_ref);
        break;
    }
#endif /*ENABLE_PRIV_NAME*/
    case RJS_AST_ParenthesesExpr: {
        RJS_AstUnaryExpr *ue  = (RJS_AstUnaryExpr*)expr;
        RJS_Ast          *ast = bc_ast_get(rt, &ue->operand);

        bc_gen_ref(rt, bg, ast, rid, ref);
        break;
    }
    case RJS_AST_OptionalBase: {
        RJS_AstUnaryExpr *ue  = (RJS_AstUnaryExpr*)expr;
        RJS_Ast          *ast = bc_ast_get(rt, &ue->operand);

        bc_gen_ref(rt, bg, ast, rid, ref);

        ref->opt_base = RJS_TRUE;
        break;
    }
    case RJS_AST_OptionalExpr: {
        RJS_AstUnaryExpr *ue  = (RJS_AstUnaryExpr*)expr;
        RJS_Ast          *ast = bc_ast_get(rt, &ue->operand);

        ref->old_opt_label = bg->opt_end_label;
        ref->old_opt_reg   = bg->opt_res_reg;

        bg->opt_end_label  = bc_label_add(rt, bg);
        bg->opt_res_reg    = rid;

        bc_gen_ref(rt, bg, ast, rid, ref);

        ref->opt_expr = RJS_TRUE;
        break;
    }
    default:
        assert(rid != -1);

        ref->type      = RJS_BC_REF_VALUE;
        ref->value_rid = rid;

        bc_gen_expr(rt, bg, expr, rid);
        break;
    }

    return RJS_OK;
}

/*Get this value from the reference.*/
static RJS_Result
bc_ref_get_this (RJS_Runtime *rt, RJS_BcGen *bg, int line, int *tr, RJS_BcRef *ref)
{
    int            cid;
    int            rid;
    RJS_BcCommand *cmd;

    switch (ref->type) {
    case RJS_BC_REF_BINDING:
        rid = bc_reg_add(rt, bg);
        cid = bc_cmd_add(rt, bg, RJS_BC_load_with_base, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load_with_base.env  = ref->env_rid;
        cmd->load_with_base.dest = rid;
        break;
    case RJS_BC_REF_PROPERTY:
    case RJS_BC_REF_SUPER:
#if ENABLE_PRIV_NAME
    case RJS_BC_REF_PRIVATE:
#endif /*ENABLE_PRIV_NAME*/
        rid = ref->base_rid;
        break;
    case RJS_BC_REF_VALUE:
        rid = bc_reg_add(rt, bg);
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;
        break;
    default:
        assert(0);
        rid = -1;
    }

    *tr = rid;
    return RJS_OK;
}

/*Get the value from the reference.*/
static RJS_Result
bc_ref_get_value (RJS_Runtime *rt, RJS_BcGen *bg, int line, int rid, RJS_BcRef *ref)
{
    int            cid;
    RJS_BcCommand *cmd;

    switch (ref->type) {
    case RJS_BC_REF_BINDING:
        cid = bc_cmd_add(rt, bg, RJS_BC_binding_get, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_get.env     = ref->env_rid;
        cmd->binding_get.binding = ref->binding_ref;
        cmd->binding_get.dest    = rid;
        break;
    case RJS_BC_REF_PROPERTY:
        if (ref->ref_name_prop) {
            cid = bc_cmd_add(rt, bg, RJS_BC_prop_get, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->prop_get.base = ref->base_rid;
            cmd->prop_get.prop = ref->ref_name_prop;
            cmd->prop_get.dest = rid;
        } else {
            cid = bc_cmd_add(rt, bg, RJS_BC_prop_get_expr, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binary.operand1 = ref->base_rid;
            cmd->binary.operand2 = ref->ref_name_rid;
            cmd->binary.result   = rid;
        }
        break;
#if ENABLE_PRIV_NAME
    case RJS_BC_REF_PRIVATE:
        cid = bc_cmd_add(rt, bg, RJS_BC_priv_get, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->priv_get.base = ref->base_rid;
        cmd->priv_get.priv = ref->ref_name_prop;
        cmd->priv_get.dest = rid;
        break;
#endif /*ENABLE_PRIV_NAME*/
    case RJS_BC_REF_SUPER:
        if (ref->ref_name_prop) {
            cid = bc_cmd_add(rt, bg, RJS_BC_super_prop_get, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->super_prop_get.thiz = ref->base_rid;
            cmd->super_prop_get.prop = ref->ref_name_prop;
            cmd->super_prop_get.dest = rid;
        } else {
            cid = bc_cmd_add(rt, bg, RJS_BC_super_prop_get_expr, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binary.operand1 = ref->base_rid;
            cmd->binary.operand2 = ref->ref_name_rid;
            cmd->binary.result   = rid;
        }
        break;
    case RJS_BC_REF_VALUE:
        if (ref->value_rid != rid) {
            cid = bc_cmd_add(rt, bg, RJS_BC_dup, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->unary.operand = ref->value_rid;
            cmd->unary.result  = rid;
        }
        break;
    }

    if (ref->opt_base) {
        int cr  = bc_reg_add(rt, bg);
        int lop = bc_label_add(rt, bg);

        cid = bc_cmd_add(rt, bg, RJS_BC_is_undefined_null, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = rid;
        cmd->unary.result  = cr;

        cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.label = lop;
        cmd->jump_cond.value = cr;

        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->opt_res_reg;

        cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump.label = bg->opt_end_label;

        cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->stub.label = lop;
    }

    if (ref->opt_expr) {
        cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->stub.label = bg->opt_end_label;

        bg->opt_end_label = ref->old_opt_label;
        bg->opt_res_reg   = ref->old_opt_reg;
    }

    return RJS_OK;
}

/*Set the value to the reference.*/
static RJS_Result
bc_ref_set_value (RJS_Runtime *rt, RJS_BcGen *bg, int line, int rid, RJS_BcRef *ref)
{
    int            cid;
    RJS_BcCommand *cmd;

    switch (ref->type) {
    case RJS_BC_REF_BINDING:
        bc_gen_binding_set(rt, bg, line, rid, ref);
        break;
    case RJS_BC_REF_PROPERTY:
        if (ref->ref_name_prop) {
            cid = bc_cmd_add(rt, bg, RJS_BC_prop_set, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->prop_set.base  = ref->base_rid;
            cmd->prop_set.prop  = ref->ref_name_prop;
            cmd->prop_set.value = rid;
        } else {
            cid = bc_cmd_add(rt, bg, RJS_BC_prop_set_expr, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->prop_set_expr.base  = ref->base_rid;
            cmd->prop_set_expr.prop  = ref->ref_name_rid;
            cmd->prop_set_expr.value = rid;
        }
        break;
#if ENABLE_PRIV_NAME
    case RJS_BC_REF_PRIVATE:
        cid = bc_cmd_add(rt, bg, RJS_BC_priv_set, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->priv_set.base  = ref->base_rid;
        cmd->priv_set.priv  = ref->ref_name_prop;
        cmd->priv_set.value = rid;
        break;
#endif /*ENABLE_PRIV_NAME*/
    case RJS_BC_REF_SUPER:
        if (ref->ref_name_prop) {
            cid = bc_cmd_add(rt, bg, RJS_BC_super_prop_set, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->super_prop_set.thiz  = ref->base_rid;
            cmd->super_prop_set.prop  = ref->ref_name_prop;
            cmd->super_prop_set.value = rid;
        } else {
            cid = bc_cmd_add(rt, bg, RJS_BC_super_prop_set_expr, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->super_prop_set_expr.thiz  = ref->base_rid;
            cmd->super_prop_set_expr.prop  = ref->ref_name_rid;
            cmd->super_prop_set_expr.value = rid;
        }
        break;
    case RJS_BC_REF_VALUE:
        assert(0);
        break;
    }

    return RJS_OK;
}

/*Generate typeof expression.*/
static RJS_Result
bc_gen_typeof_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    RJS_BcRef      ref;
    int            line;
    int            t_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    t_rid = bc_reg_add(rt, bg);
    ast   = bc_ast_get(rt, &expr->operand);

    bc_gen_ref(rt, bg, ast, t_rid, &ref);

    line = ast->location.first_line;

    if (ref.type == RJS_BC_REF_BINDING) {
        cid = bc_cmd_add(rt, bg, RJS_BC_typeof_binding, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->typeof_binding.binding = ref.binding_ref;
        cmd->typeof_binding.dest    = rid;
    } else {
        bc_ref_get_value(rt, bg, line, t_rid, &ref);

        cid = bc_cmd_add(rt, bg, RJS_BC_typeof, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = t_rid;
        cmd->unary.result  = rid;
    }

    return RJS_OK;
}

/*Generate array pattern assignment.*/
static RJS_Result
bc_gen_array_assi (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstList *l, int rid)
{
    RJS_Ast       *i_ast, *ast;
    int            line;
    int            cid;
    RJS_BcCommand *cmd;

    line = l->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_array_assi, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = rid;

    rjs_list_foreach_c(&l->list, i_ast, RJS_Ast, ln) {
        line = i_ast->location.first_line;

        switch (i_ast->type) {
        case RJS_AST_Elision:
            cid = bc_cmd_add(rt, bg, RJS_BC_next_array_item, line);
            break;
        case RJS_AST_BindingElem: {
            RJS_AstBindingElem *be  = (RJS_AstBindingElem*)i_ast;
            int                 tr  = bc_reg_add(rt, bg);
            RJS_BcRef           ref;

            ast = bc_ast_get(rt, &be->binding);
            bc_gen_assi_ref(rt, bg, ast, &ref);

            cid = bc_cmd_add(rt, bg, RJS_BC_get_array_item, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load.dest = tr;

            ast = bc_ast_get(rt, &be->init);
            bc_gen_default_init(rt, bg, ast, tr);

            ast = bc_ast_get(rt, &be->binding);
            bc_gen_assi(rt, bg, ast, tr, &ref);
            break;
        }
        case RJS_AST_Rest: {
            RJS_AstRest *rest = (RJS_AstRest*)i_ast;
            int          tr   = bc_reg_add(rt, bg);
            RJS_BcRef    ref;

            ast = bc_ast_get(rt, &rest->binding);
            bc_gen_assi_ref(rt, bg, ast, &ref);

            cid = bc_cmd_add(rt, bg, RJS_BC_rest_array_items, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load.dest = tr;

            ast = bc_ast_get(rt, &rest->binding);
            bc_gen_assi(rt, bg, ast, tr, &ref);
            break;
        }
        default:
            assert(0);
        }
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Generate object pattern assignment.*/
static RJS_Result
bc_gen_object_assi (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstList *l, int rid)
{
    RJS_Ast       *p_ast, *ast;
    int            line;
    int            cid;
    RJS_BcCommand *cmd;

    line = l->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_object_assi, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = rid;

    rjs_list_foreach_c(&l->list, p_ast, RJS_Ast, ln) {
        line = p_ast->location.first_line;

        switch (p_ast->type) {
        case RJS_AST_BindingProp: {
            RJS_AstBindingProp *bp       = (RJS_AstBindingProp*)p_ast;
            int                 tr       = bc_reg_add(rt, bg);
            int                 kr       = -1;
            RJS_Bool            str_prop = RJS_FALSE;
            RJS_BcRef           ref;

            ast  = bc_ast_get(rt, &bp->name);
            line = ast->location.first_line;
            if (ast->type == RJS_AST_ValueExpr) {
                RJS_AstValueExpr *ve = (RJS_AstValueExpr*)ast;

                if (rjs_value_is_string(rt, &ve->ve->value))
                    str_prop = RJS_TRUE;
            }
            
            if (!str_prop) {
                int nr = bc_reg_add(rt, bg);

                kr = bc_reg_add(rt, bg);

                bc_gen_expr(rt, bg, ast, nr);

                cid = bc_cmd_add(rt, bg, RJS_BC_to_prop, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->unary.operand = nr;
                cmd->unary.result  = kr;
            }

            ast = bc_ast_get(rt, &bp->binding);
            bc_gen_assi_ref(rt, bg, ast, &ref);

            ast  = bc_ast_get(rt, &bp->name);
            line = ast->location.first_line;
            if (str_prop) {
                RJS_AstValueExpr *ve;

                ve = (RJS_AstValueExpr*)ast;

                cid = bc_cmd_add(rt, bg, RJS_BC_get_object_prop, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->get_object_prop.prop = rjs_code_gen_prop_ref(rt, &bp->prop_ref,
                        &ast->location, bg->func_ast, &ve->ve->value);
                cmd->get_object_prop.dest = tr;
            } else {
                cid = bc_cmd_add(rt, bg, RJS_BC_get_object_prop_expr, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->unary.operand = kr;
                cmd->unary.result  = tr;
            }

            ast = bc_ast_get(rt, &bp->init);
            bc_gen_default_init(rt, bg, ast, tr);

            ast = bc_ast_get(rt, &bp->binding);
            bc_gen_assi(rt, bg, ast, tr, &ref);
            break;
        }
        case RJS_AST_Rest: {
            RJS_AstRest *rest = (RJS_AstRest*)p_ast;
            int          tr   = bc_reg_add(rt, bg);
            RJS_BcRef    ref;

            ast = bc_ast_get(rt, &rest->binding);
            bc_gen_assi_ref(rt, bg, ast, &ref);

            cid = bc_cmd_add(rt, bg, RJS_BC_rest_object_props, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load.dest = tr;

            ast = bc_ast_get(rt, &rest->binding);
            bc_gen_assi(rt, bg, ast, tr, &ref);
            break;
        }
        default:
            assert(0);
        }
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Generate assignment reference.*/
static RJS_Result
bc_gen_assi_ref (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *lh, RJS_BcRef *ref)
{
    if ((lh->type != RJS_AST_ArrayBinding) && (lh->type != RJS_AST_ObjectBinding)) {
        bc_gen_ref(rt, bg, lh, -1, ref);
    }

    return RJS_OK;
}

/*Generate assignment.*/
static RJS_Result
bc_gen_assi (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *lh, int rid, RJS_BcRef *ref)
{
    if (lh->type == RJS_AST_ArrayBinding) {
        bc_gen_array_assi(rt, bg, (RJS_AstList*)lh, rid);
    } else if (lh->type == RJS_AST_ObjectBinding) {
        bc_gen_object_assi(rt, bg, (RJS_AstList*)lh, rid);
    } else {
        int line;

        line = lh->location.first_line;

        bc_ref_set_value(rt, bg, line, rid, ref);
    }

    return RJS_OK;
}


/*Generate assignment expression.*/
static RJS_Result
bc_gen_assi_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid)
{
    int        line;
    RJS_Ast   *ast;
    RJS_BcRef  ref;

    ast = bc_ast_get(rt, &expr->operand1);
    
    if ((ast->type == RJS_AST_ArrayBinding) || (ast->type == RJS_AST_ObjectBinding)) {
        RJS_Ast *r_ast;

        r_ast = bc_ast_get(rt, &expr->operand2);
        bc_gen_expr(rt, bg, r_ast, rid);

        if (ast->type == RJS_AST_ArrayBinding)
            bc_gen_array_assi(rt, bg, (RJS_AstList*)ast, rid);
        else
            bc_gen_object_assi(rt, bg, (RJS_AstList*)ast, rid);
    } else {
        bc_gen_ref(rt, bg, ast, -1, &ref);

        ast = bc_ast_get(rt, &expr->operand2);
        bc_gen_expr(rt, bg, ast, rid);

        line = expr->ast.location.first_line;
        bc_ref_set_value(rt, bg, line, rid, &ref);
    }

    return RJS_OK;
}

/*Generate delete expression.*/
static RJS_Result
bc_gen_delete_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int            line;
    int            t_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;
    RJS_BcRef      ref;

    t_rid = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &expr->operand);
    bc_gen_ref(rt, bg, ast, t_rid, &ref);

    line = expr->ast.location.first_line;

    switch (ref.type) {
    case RJS_BC_REF_VALUE:
        cid = bc_cmd_add(rt, bg, RJS_BC_load_true, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;
        break;
    case RJS_BC_REF_BINDING:
        cid = bc_cmd_add(rt, bg, RJS_BC_del_binding, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->del_binding.env     = ref.env_rid;
        cmd->del_binding.binding = ref.binding_ref;
        cmd->del_binding.result  = rid;
        break;
    case RJS_BC_REF_PROPERTY:
        if (ref.ref_name_prop) {
            cid = bc_cmd_add(rt, bg, RJS_BC_del_prop, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->del_prop.base   = ref.base_rid;
            cmd->del_prop.prop   = ref.ref_name_prop;
            cmd->del_prop.result = rid;
        } else {
            cid = bc_cmd_add(rt, bg, RJS_BC_del_prop_expr, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binary.operand1 = ref.base_rid;
            cmd->binary.operand2 = ref.ref_name_rid;
            cmd->binary.result   = rid;
        }
        break;
    case RJS_BC_REF_SUPER:
        cid = bc_cmd_add(rt, bg, RJS_BC_throw_ref_error, line);
        break;
    default:
        assert(0);
    }

    return RJS_OK;
}

/*Generate pre increase/decrease expression.*/
static RJS_Result
bc_gen_pre_inc_dec_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int            line;
    int            t_rid, n_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;
    RJS_BcRef      ref;

    ast = bc_ast_get(rt, &expr->operand);
    bc_gen_ref(rt, bg, ast, -1, &ref);

    line = expr->ast.location.first_line;

    t_rid = bc_reg_add(rt, bg);
    n_rid = bc_reg_add(rt, bg);
    bc_ref_get_value(rt, bg, line, t_rid, &ref);

    cid = bc_cmd_add(rt, bg, RJS_BC_to_numeric, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = t_rid;
    cmd->unary.result  = n_rid;

    cid = bc_cmd_add(rt, bg,
            (expr->ast.type == RJS_AST_PreIncExpr) ? RJS_BC_inc : RJS_BC_dec,
            line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = n_rid;
    cmd->unary.result  = rid;

    bc_ref_set_value(rt, bg, line, rid, &ref);

    return RJS_OK;
}

/*Generate post increase/decrease expression.*/
static RJS_Result
bc_gen_post_inc_dec_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *expr, int rid)
{
    int            line;
    int            t_rid, n_rid;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;
    RJS_BcRef      ref;

    ast = bc_ast_get(rt, &expr->operand);
    bc_gen_ref(rt, bg, ast, -1, &ref);

    line = expr->ast.location.first_line;

    t_rid = bc_reg_add(rt, bg);
    n_rid = bc_reg_add(rt, bg);
    bc_ref_get_value(rt, bg, line, t_rid, &ref);

    cid = bc_cmd_add(rt, bg, RJS_BC_to_numeric, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = t_rid;
    cmd->unary.result  = rid;

    cid = bc_cmd_add(rt, bg,
            (expr->ast.type == RJS_AST_PostIncExpr) ? RJS_BC_inc : RJS_BC_dec,
            line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = rid;
    cmd->unary.result  = n_rid;

    bc_ref_set_value(rt, bg, line, n_rid, &ref);

    return RJS_OK;
}

/*Generate member expression.*/
static RJS_Result
bc_gen_member_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid)
{
    RJS_BcRef  ref;
    int        line;

    bc_gen_ref(rt, bg, &expr->ast, rid, &ref);

    line = expr->ast.location.first_line;

    return bc_ref_get_value(rt, bg, line, rid, &ref);
}

/*Generate binary expression.*/
static RJS_Result
bc_gen_binary_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid, RJS_BcType type)
{
    RJS_Ast       *ast;
    int            line;
    int            r1, r2;
    int            cid;
    RJS_BcCommand *cmd;

    r1 = bc_reg_add(rt, bg);
    r2 = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &expr->operand1);
    bc_gen_expr(rt, bg, ast, r1);

    ast = bc_ast_get(rt, &expr->operand2);
    bc_gen_expr(rt, bg, ast, r2);

    line = expr->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, type, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->binary.operand1 = r1;
    cmd->binary.operand2 = r2;
    cmd->binary.result   = rid;

    return RJS_OK;
}

/*Generate in expression.*/
static RJS_Result
bc_gen_in_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid)
{
    RJS_Ast       *ast;
    int            line;
    int            r1, r2;
    int            cid;
    RJS_BcType     bc_type;
    RJS_BcCommand *cmd;

    r1 = bc_reg_add(rt, bg);
    r2 = bc_reg_add(rt, bg);

    line = expr->ast.location.first_line;

    ast = bc_ast_get(rt, &expr->operand1);

#if ENABLE_PRIV_NAME
    if (ast->type == RJS_AST_PrivIdRef) {
        RJS_AstPrivIdRef *pir = (RJS_AstPrivIdRef*)ast;
        RJS_AstPropRef   *pr  = bc_ast_get(rt, &pir->prop_ref);

        cid = bc_cmd_add(rt, bg, RJS_BC_load_value, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load_value.value = pr->prop;
        cmd->load_value.dest  = r1;

        bc_type = RJS_BC_has_priv;
    } else
#endif /*ENABLE_PRIV_NAME*/
    {
        int tr = bc_reg_add(rt, bg);

        bc_gen_expr(rt, bg, ast, tr);

        cid = bc_cmd_add(rt, bg, RJS_BC_to_prop, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = tr;
        cmd->unary.result  = r1;

        bc_type = RJS_BC_has_prop;
    }

    ast = bc_ast_get(rt, &expr->operand2);
    bc_gen_expr(rt, bg, ast, r2);

    cid = bc_cmd_add(rt, bg, bc_type, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->binary.operand1 = r2;
    cmd->binary.operand2 = r1;
    cmd->binary.result   = rid;

    return RJS_OK;
}

/*Generate logic expression.*/
static RJS_Result
bc_gen_logic_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid)
{
    RJS_Ast       *ast;
    int            line;
    int            lid;
    int            cid;
    int            t_rid;
    RJS_BcCommand *cmd;

    lid = bc_label_add(rt, bg);

    ast = bc_ast_get(rt, &expr->operand1);
    bc_gen_expr(rt, bg, ast, rid);

    line = expr->ast.location.first_line;

    switch (expr->ast.type) {
    case RJS_AST_AndExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = rid;
        cmd->jump_cond.label = lid;
        break;
    case RJS_AST_OrExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_jump_true, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = rid;
        cmd->jump_cond.label = lid;
        break;
    case RJS_AST_QuesExpr:
        t_rid = bc_reg_add(rt, bg);

        cid = bc_cmd_add(rt, bg, RJS_BC_is_undefined_null, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = rid;
        cmd->unary.result  = t_rid;

        cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = t_rid;
        cmd->jump_cond.label = lid;
        break;
    }

    ast = bc_ast_get(rt, &expr->operand2);
    bc_gen_expr(rt, bg, ast, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lid;

    return RJS_OK;
}

/*Generate condition expression.*/
static RJS_Result
bc_gen_cond_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstCondExpr *expr, int rid)
{
    RJS_Ast       *ast;
    int            line;
    int            l1, l2;
    int            cid;
    int            cr;
    RJS_BcCommand *cmd;

    cr = bc_reg_add(rt, bg);
    l1 = bc_label_add(rt, bg);
    l2 = bc_label_add(rt, bg);

    ast = bc_ast_get(rt, &expr->cond);
    bc_gen_expr(rt, bg, ast, cr);

    line = expr->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.value = cr;
    cmd->jump_cond.label = l1;

    ast = bc_ast_get(rt, &expr->true_value);
    bc_gen_expr(rt, bg, ast, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = l2;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = l1;

    ast = bc_ast_get(rt, &expr->false_value);
    bc_gen_expr(rt, bg, ast, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = l2;

    return RJS_OK;
}

/*Generate arguments.*/
static RJS_Result
bc_gen_args (RJS_Runtime *rt, RJS_BcGen *bg, int line, RJS_List *args)
{
    RJS_Ast *a_ast;

    rjs_list_foreach_c(args, a_ast, RJS_Ast, ln) {
        int            line;
        int            rid;
        int            cid;
        RJS_BcCommand *cmd;

        if (a_ast->type == RJS_AST_LastElision)
            continue;

        line = a_ast->location.first_line;

        rid = bc_reg_add(rt, bg);

        if (a_ast->type == RJS_AST_SpreadExpr) {
            RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)a_ast;
            RJS_Ast          *e;

            e = bc_ast_get(rt, &ue->operand);
            bc_gen_expr(rt, bg, e, rid);

            cid = bc_cmd_add(rt, bg, RJS_BC_spread_args_add, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = rid;
        } else {
            bc_gen_expr(rt, bg, a_ast, rid);

            cid = bc_cmd_add(rt, bg, RJS_BC_arg_add, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = rid;
        }
    }

    return RJS_OK;
}

/*Generate call expression.*/
static RJS_Result
bc_gen_call_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstCall *expr, int rid, RJS_Bool tail)
{
    RJS_Ast       *ast;
    RJS_BcRef      ref;
    int            line;
    int            fr;
    int            tr   = -1;
    int            cid;
    RJS_BcCommand *cmd;
    RJS_Bool       is_opt;

    fr = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &expr->func);
    bc_gen_ref(rt, bg, ast, fr, &ref);

    line = expr->ast.location.first_line;

    bc_ref_get_value(rt, bg, line, fr, &ref);
    bc_ref_get_this(rt, bg, line, &tr, &ref);

    cid = bc_cmd_add(rt, bg, RJS_BC_push_call, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->push_call.func = fr;
    cmd->push_call.thiz = tr;

    bc_gen_args(rt, bg, line, &expr->arg_list);

    is_opt = RJS_FALSE;

    if ((rid != -1) && (bg->opt_res_reg == rid))
        is_opt = RJS_TRUE;

#if ENABLE_EVAL
    /*Is direct "eval"?*/
    if ((ref.type == RJS_BC_REF_BINDING)
            && !is_opt
            && rjs_string_equal(rt, &ref.binding_ref->name->value, rjs_s_eval(rt))) {
        cid = bc_cmd_add(rt, bg, tail ? RJS_BC_tail_eval : RJS_BC_eval, line);
    } else
#endif /*ENABLE_EVAL*/
    {
        cid = bc_cmd_add(rt, bg, tail ? RJS_BC_tail_call : RJS_BC_call, line);
    }

    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

/*Generate super call expression.*/
static RJS_Result
bc_gen_super_call_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstCall *expr, int rid)
{
    int            line;
    int            cid;
    RJS_BcCommand *cmd;

    line = expr->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_super_call, line);

    bc_gen_args(rt, bg, line, &expr->arg_list);

    cid = bc_cmd_add(rt, bg, RJS_BC_super_call, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

/*Generate new expression.*/
static RJS_Result
bc_gen_new_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstCall *expr, int rid)
{
    RJS_Ast       *ast;
    int            line;
    int            fr;
    int            cid;
    RJS_BcCommand *cmd;

    fr = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &expr->func);
    bc_gen_expr(rt, bg, ast, fr);

    line = expr->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_new, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = fr;

    bc_gen_args(rt, bg, line, &expr->arg_list);

    cid = bc_cmd_add(rt, bg, RJS_BC_new, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

/*Generate template expression.*/
static RJS_Result
bc_gen_templ_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstTemplate *expr, int rid, RJS_Bool tco)
{
    RJS_Ast       *ast;
    int            line;
    int            tr   = -1;
    int            fr   = -1;
    int            cid;
    RJS_BcCommand *cmd;

    line = expr->ast.location.first_line;

    ast = bc_ast_get(rt, &expr->func);

    if (ast) {
        RJS_BcRef ref;

        fr = bc_reg_add(rt, bg);

        ast = bc_ast_get(rt, &expr->func);

        bc_gen_ref(rt, bg, ast, fr, &ref);

        bc_ref_get_value(rt, bg, line, fr, &ref);
        bc_ref_get_this(rt, bg, line, &tr, &ref);
    }

    if (ast) {
        cid = bc_cmd_add(rt, bg, RJS_BC_push_call, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->push_call.func = fr;
        cmd->push_call.thiz = tr;
    } else {
        cid = bc_cmd_add(rt, bg, RJS_BC_push_concat, line);
    }

    tr = bc_reg_add(rt, bg);

    cid = bc_cmd_add(rt, bg, RJS_BC_load_value, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load_value.value = expr->ve;
    cmd->load_value.dest  = tr;

    cid = bc_cmd_add(rt, bg, RJS_BC_arg_add, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = tr;

    rjs_list_foreach_c(&expr->expr_list, ast, RJS_Ast, ln) {
        tr = bc_reg_add(rt, bg);
        bc_gen_expr(rt, bg, ast, tr);

        line = ast->location.first_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_arg_add, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = tr;
    }

    cid = bc_cmd_add(rt, bg, tco ? RJS_BC_tail_call : RJS_BC_call, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    return RJS_OK;
}

/*Generate comma expression.*/
static RJS_Result
bc_gen_comma_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstList *expr, int rid)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(&expr->list, ast, RJS_Ast, ln) {
        int t_rid;

        if (ast->ln.next == &expr->list)
            t_rid = rid;
        else
            t_rid = bc_reg_add(rt, bg);

        bc_gen_expr(rt, bg, ast, t_rid);
    }

    return RJS_OK;
}

/*Generate function expression.*/
static RJS_Result
bc_gen_func_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstFuncRef *expr, int rid)
{
    int            line;
    int            cid;
    RJS_BcCommand *cmd;

    line = expr->ast.location.first_line;

    if (expr->binding_ref) {
        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = expr->decl;
        
        rjs_code_gen_binding_init_table(rt, &expr->lex_table, expr->decl);

        cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_table.table = bc_ast_get(rt, &expr->lex_table);
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_func_create, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->func_create.func = expr->func;
    cmd->func_create.dest = rid;

    if (expr->binding_ref) {
        bc_gen_binding_init(rt, bg, line, expr->binding_ref, rid);

        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
    }

    return RJS_OK;
}

/*Generate self-op assignment expression.*/
static RJS_Result
bc_gen_op_assi_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid)
{
    RJS_BcRef      ref;
    int            line;
    int            lr, rr;
    int            cid = -1;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    line = expr->ast.location.first_line;

    lr = bc_reg_add(rt, bg);
    rr = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &expr->operand1);

    bc_gen_ref(rt, bg, ast, lr, &ref);

    bc_ref_get_value(rt, bg, line, lr, &ref);

    ast = bc_ast_get(rt, &expr->operand2);
    bc_gen_expr(rt, bg, ast, rr);

    switch (expr->ast.type) {
    case RJS_AST_AddAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_add, line);
        break;
    case RJS_AST_SubAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_sub, line);
        break;
    case RJS_AST_MulAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_mul, line);
        break;
    case RJS_AST_DivAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_div, line);
        break;
    case RJS_AST_ModAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_mod, line);
        break;
    case RJS_AST_ExpAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_exp, line);
        break;
    case RJS_AST_ShlAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_shl, line);
        break;
    case RJS_AST_ShrAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_shr, line);
        break;
    case RJS_AST_UShrAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_ushr, line);
        break;
    case RJS_AST_BitAndAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_and, line);
        break;
    case RJS_AST_BitXorAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_xor, line);
        break;
    case RJS_AST_BitOrAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_or, line);
        break;
    default:
        assert(0);
    }

    cmd = bc_cmd_get(bg, cid);
    cmd->binary.operand1 = lr;
    cmd->binary.operand2 = rr;
    cmd->binary.result   = rid;

    bc_ref_set_value(rt, bg, line, rid, &ref);

    return RJS_OK;
}

/*Generate optional assignment expression.*/
static RJS_Result
bc_gen_opt_assi_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBinaryExpr *expr, int rid)
{
    RJS_BcRef      ref;
    int            line;
    int            cr;
    int            cid;
    int            lid;
    RJS_Ast       *ast;
    RJS_BcCommand *cmd;

    line = expr->ast.location.first_line;
    lid  = bc_label_add(rt, bg);

    ast = bc_ast_get(rt, &expr->operand1);
    bc_gen_ref(rt, bg, ast, rid, &ref);

    bc_ref_get_value(rt, bg, line, rid, &ref);

    switch (expr->ast.type) {
    case RJS_AST_AndAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = rid;
        cmd->jump_cond.label = lid;
        break;
    case RJS_AST_OrAssiExpr:
        cid = bc_cmd_add(rt, bg, RJS_BC_jump_true, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = rid;
        cmd->jump_cond.label = lid;
        break;
    case RJS_AST_QuesAssiExpr:
        cr  = bc_reg_add(rt, bg);

        cid = bc_cmd_add(rt, bg, RJS_BC_is_undefined_null, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = rid;
        cmd->unary.result  = cr;

        cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = cr;
        cmd->jump_cond.label = lid;
        break;
    default:
        break;
    }

    ast = bc_ast_get(rt, &expr->operand2);
    bc_gen_expr(rt, bg, ast, rid);

    bc_ref_set_value(rt, bg, line, rid, &ref);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lid;

    return RJS_OK;
}

/*Generate the array literal.*/
static RJS_Result
bc_gen_array (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstList *a, int rid)
{
    RJS_Ast       *i_ast, *ast;
    int            line;
    int            tr;
    int            cid;
    RJS_BcCommand *cmd;

    line = a->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_new_array, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    rjs_list_foreach_c(&a->list, i_ast, RJS_Ast, ln) {
        line = i_ast->location.first_line;

        switch (i_ast->type) {
        case RJS_AST_LastElision:
            break;
        case RJS_AST_Elision:
            cid = bc_cmd_add(rt, bg, RJS_BC_array_elision_item, line);
            break;
        case RJS_AST_SpreadExpr: {
            RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)i_ast;

            tr  = bc_reg_add(rt, bg);
            ast = bc_ast_get(rt, &ue->operand);
            bc_gen_expr(rt, bg, ast, tr);

            cid = bc_cmd_add(rt, bg, RJS_BC_array_spread_items, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = tr;
            break;
        }
        default:
            tr = bc_reg_add(rt, bg);
            bc_gen_expr(rt, bg, i_ast, tr);

            cid = bc_cmd_add(rt, bg, RJS_BC_array_add_item, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = tr;
            break;
        }
    }

    line = a->ast.location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Check if the node is an anonymous function.*/
static RJS_Bool
is_anonymous_function (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Ast *ast = bc_ast_get(rt, v);

    if (ast->type == RJS_AST_FuncExpr) {
        RJS_AstFuncRef *fe = (RJS_AstFuncRef*)ast;

        if (!fe->func->name)
            return RJS_TRUE;
    } else if (ast->type == RJS_AST_ClassExpr) {
        RJS_AstClassRef *ce = (RJS_AstClassRef*)ast;

        if (!ce->clazz->name)
            return RJS_TRUE;
    } else if (ast->type == RJS_AST_ParenthesesExpr) {
        RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)ast;

        return is_anonymous_function(rt, &ue->operand);
    }

    return RJS_FALSE;
}

/*Generate the object literal.*/
static RJS_Result
bc_gen_object (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstList *o, int rid)
{
    RJS_Ast       *p_ast, *ast;
    int            line;
    int            tr, nr;
    int            cid;
    RJS_BcCommand *cmd;

    line = o->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_new_object, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = rid;

    rjs_list_foreach_c(&o->list, p_ast, RJS_Ast, ln) {
        line = p_ast->location.first_line;

        switch (p_ast->type) {
        case RJS_AST_Prop:
        case RJS_AST_SetProto: {
            RJS_AstProp *prop  = (RJS_AstProp*)p_ast;
            RJS_Bool     is_af = RJS_FALSE;

            nr = bc_reg_add(rt, bg);
            tr = bc_reg_add(rt, bg);

            ast = bc_ast_get(rt, &prop->name);
            bc_gen_prop_name(rt, bg, ast, nr);

            ast = bc_ast_get(rt, &prop->value);
            if (ast) {
                is_af = is_anonymous_function(rt, &prop->value);

                bc_gen_expr(rt, bg, ast, tr);
            } else {
                RJS_AstValueExpr  *ve = bc_ast_get(rt, &prop->name);
                int                er = bc_reg_add(rt, bg);
                RJS_AstBindingRef *br;

                br = rjs_code_gen_binding_ref(rt, &ve->ast.location, &ve->ve->value);

                cid = bc_cmd_add(rt, bg, RJS_BC_binding_resolve, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->binding_resolve.binding = br;
                cmd->binding_resolve.env     = er;

                cid = bc_cmd_add(rt, bg, RJS_BC_binding_get, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->binding_get.env     = er;
                cmd->binding_get.binding = br;
                cmd->binding_get.dest    = tr;
            }

            if (p_ast->type == RJS_AST_SetProto) {
                cid = bc_cmd_add(rt, bg, RJS_BC_set_proto, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->set_proto.obj   = rid;
                cmd->set_proto.proto = tr;
            } else {
                cid = bc_cmd_add(rt, bg,
                        is_af ? RJS_BC_object_add_func : RJS_BC_object_add_prop,
                        line);
                cmd = bc_cmd_get(bg, cid);
                cmd->object_add_prop.prop  = nr;
                cmd->object_add_prop.value = tr;
            }
            break;
        }
        case RJS_AST_SpreadExpr: {
            RJS_AstUnaryExpr *ue = (RJS_AstUnaryExpr*)p_ast;

            tr  = bc_reg_add(rt, bg);
            ast = bc_ast_get(rt, &ue->operand);
            bc_gen_expr(rt, bg, ast, tr);

            cid = bc_cmd_add(rt, bg, RJS_BC_object_spread_props, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->store.value = tr;
            break;
        }
        case RJS_AST_ClassElem: {
            RJS_AstClassElem *ce = (RJS_AstClassElem*)p_ast;

            nr  = bc_reg_add(rt, bg);
            ast = bc_ast_get(rt, &ce->name);
            bc_gen_prop_name(rt, bg, ast, nr);

            switch (ce->type) {
            case RJS_AST_CLASS_ELEM_METHOD:
                cid = bc_cmd_add(rt, bg, RJS_BC_object_method_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = nr;
                cmd->class_elem.func = ce->func;
                break;
            case RJS_AST_CLASS_ELEM_GET:
                cid = bc_cmd_add(rt, bg, RJS_BC_object_getter_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = nr;
                cmd->class_elem.func = ce->func;
                break;
            case RJS_AST_CLASS_ELEM_SET:
                cid = bc_cmd_add(rt, bg, RJS_BC_object_setter_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = nr;
                cmd->class_elem.func = ce->func;
                break;
            default:
                assert(0);
            }
            break;
        }
        default:
            assert(0);
        }
    }

    line = o->ast.location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Parse optional base expression.*/
static RJS_Result
bc_gen_optional_base (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *ue, int rid)
{
    int            line = ue->ast.location.first_line;
    RJS_Ast       *ast  = bc_ast_get(rt, &ue->operand);
    int            lop;
    int            cid;
    int            cr;
    RJS_BcCommand *cmd;

    assert((bg->opt_end_label != -1) && (bg->opt_res_reg != -1));

    lop = bc_label_add(rt, bg);
    cr  = bc_reg_add(rt, bg);

    bc_gen_expr(rt, bg, ast, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_is_undefined_null, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = rid;
    cmd->unary.result  = cr;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.label = lop;
    cmd->jump_cond.value = cr;

    cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->load.dest = bg->opt_res_reg;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = bg->opt_end_label;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lop;

    return RJS_OK;
}

/*Parse optional expression.*/
static RJS_Result
bc_gen_optional_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstUnaryExpr *ue, int rid)
{
    int            old_label = bg->opt_end_label;
    int            old_reg   = bg->opt_res_reg;
    RJS_Ast       *ast       = bc_ast_get(rt, &ue->operand);
    int            line      = ue->ast.location.first_line;
    int            cid;
    RJS_BcCommand *cmd;

    bg->opt_end_label = bc_label_add(rt, bg);
    bg->opt_res_reg   = rid;

    bc_gen_expr(rt, bg, ast, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = bg->opt_end_label;

    bg->opt_end_label = old_label;
    bg->opt_res_reg   = old_reg;

    return RJS_OK;
}

/*Generate an expression.*/
static RJS_Result
bc_gen_expr (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *expr, int rid)
{
    switch (expr->type) {
    case RJS_AST_OptionalBase:
        bc_gen_optional_base(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_OptionalExpr:
        bc_gen_optional_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_Array:
        bc_gen_array(rt, bg, (RJS_AstList*)expr, rid);
        break;
    case RJS_AST_Object:
        bc_gen_object(rt, bg, (RJS_AstList*)expr, rid);
        break;
#if ENABLE_MODULE
    case RJS_AST_ImportMetaExpr:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_import_meta);
        break;
#endif /*ENABLE_MODULE*/
    case RJS_AST_NewTargetExpr:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_new_target);
        break;
    case RJS_AST_This:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_this);
        break;
    case RJS_AST_True:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_true);
        break;
    case RJS_AST_False:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_false);
        break;
    case RJS_AST_Null:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_null);
        break;
    case RJS_AST_Zero:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_0);
        break;
    case RJS_AST_One:
        bc_gen_load_expr(rt, bg, expr->location.first_line, rid, RJS_BC_load_1);
        break;
    case RJS_AST_Id:
        bc_gen_get_binding_expr(rt, bg, (RJS_AstId*)expr, rid);
        break;
    case RJS_AST_ValueExpr:
        bc_gen_value_expr(rt, bg, (RJS_AstValueExpr*)expr, rid);
        break;
    case RJS_AST_VoidExpr:
        bc_gen_void_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_ToNumExpr:
        bc_gen_unary_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid, RJS_BC_to_number);
        break;
    case RJS_AST_TypeOfExpr:
        bc_gen_typeof_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_NotExpr:
        bc_gen_unary_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid, RJS_BC_not);
        break;
    case RJS_AST_RevExpr:
        bc_gen_unary_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid, RJS_BC_reverse);
        break;
    case RJS_AST_NegExpr:
        bc_gen_unary_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid, RJS_BC_negative);
        break;
#if ENABLE_GENERATOR
    case RJS_AST_YieldExpr:
        bc_gen_yield_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_YieldStarExpr:
        bc_gen_yield_star_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
    case RJS_AST_AwaitExpr:
        bc_gen_await_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
#endif /*ENABLE_ASYNC*/
#if ENABLE_MODULE
    case RJS_AST_ImportExpr:
        bc_gen_unary_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid, RJS_BC_import);
        break;
#endif /*ENABLE_MODULE*/
    case RJS_AST_DelExpr:
        bc_gen_delete_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_PreIncExpr:
    case RJS_AST_PreDecExpr:
        bc_gen_pre_inc_dec_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_PostIncExpr:
    case RJS_AST_PostDecExpr:
        bc_gen_post_inc_dec_expr(rt, bg, (RJS_AstUnaryExpr*)expr, rid);
        break;
    case RJS_AST_ParenthesesExpr: {
        RJS_AstUnaryExpr *ue  = (RJS_AstUnaryExpr*)expr;
        RJS_Ast          *ast = bc_ast_get(rt, &ue->operand);

        bc_gen_expr(rt, bg, ast, rid);
        break;
    }
    case RJS_AST_MemberExpr:
    case RJS_AST_PrivMemberExpr:
    case RJS_AST_SuperMemberExpr:
        bc_gen_member_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid);
        break;
    case RJS_AST_AddExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_add);
        break;
    case RJS_AST_SubExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_sub);
        break;
    case RJS_AST_MulExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_mul);
        break;
    case RJS_AST_DivExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_div);
        break;
    case RJS_AST_ModExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_mod);
        break;
    case RJS_AST_ExpExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_exp);
        break;
    case RJS_AST_ShlExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_shl);
        break;
    case RJS_AST_ShrExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_shr);
        break;
    case RJS_AST_UShrExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_ushr);
        break;
    case RJS_AST_LtExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_lt);
        break;
    case RJS_AST_GtExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_gt);
        break;
    case RJS_AST_LeExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_le);
        break;
    case RJS_AST_GeExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_ge);
        break;
    case RJS_AST_EqExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_eq);
        break;
    case RJS_AST_NeExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_ne);
        break;
    case RJS_AST_InExpr:
        bc_gen_in_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid);
        break;
    case RJS_AST_InstanceOfExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_instanceof);
        break;
    case RJS_AST_StrictEqExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_strict_eq);
        break;
    case RJS_AST_StrictNeExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_strict_ne);
        break;
    case RJS_AST_BitAndExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_and);
        break;
    case RJS_AST_BitXorExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_xor);
        break;
    case RJS_AST_BitOrExpr:
        bc_gen_binary_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid, RJS_BC_or);
        break;
    case RJS_AST_AndExpr:
    case RJS_AST_OrExpr:
    case RJS_AST_QuesExpr:
        bc_gen_logic_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid);
        break;
    case RJS_AST_CondExpr:
        bc_gen_cond_expr(rt, bg, (RJS_AstCondExpr*)expr, rid);
        break;
    case RJS_AST_CallExpr:
        bc_gen_call_expr(rt, bg, (RJS_AstCall*)expr, rid, RJS_FALSE);
        break;
    case RJS_AST_SuperCallExpr:
        bc_gen_super_call_expr(rt, bg, (RJS_AstCall*)expr, rid);
        break;
    case RJS_AST_NewExpr:
        bc_gen_new_expr(rt, bg, (RJS_AstCall*)expr, rid);
        break;
    case RJS_AST_Template:
        bc_gen_templ_expr(rt, bg, (RJS_AstTemplate*)expr, rid, RJS_FALSE);
        break;
    case RJS_AST_CommaExpr:
        bc_gen_comma_expr(rt, bg, (RJS_AstList*)expr, rid);
        break;
    case RJS_AST_FuncExpr:
        bc_gen_func_expr(rt, bg, (RJS_AstFuncRef*)expr, rid);
        break;
    case RJS_AST_ClassExpr:
        bc_gen_class(rt, bg, (RJS_AstClassRef*)expr, rid);
        break;
    case RJS_AST_AssiExpr:
        bc_gen_assi_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid);
        break;
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
        bc_gen_op_assi_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid);
        break;
    case RJS_AST_AndAssiExpr:
    case RJS_AST_OrAssiExpr:
    case RJS_AST_QuesAssiExpr:
        bc_gen_opt_assi_expr(rt, bg, (RJS_AstBinaryExpr*)expr, rid);
        break;
    case RJS_AST_LastElision:
        break;
    default:
        assert(0);
    }

    return RJS_OK;
}

/*Generate an expression statement.*/
static RJS_Result
bc_gen_expr_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstExprStmt *stmt)
{
    RJS_Ast    *expr;
    int         rid;

    if (bg->rv_reg != -1)
        rid = bg->rv_reg;
    else
        rid = bc_reg_add(rt, bg);

    expr = bc_ast_get(rt, &stmt->expr);

    return bc_gen_expr(rt, bg, expr, rid);
}

/*Generate a block.*/
static RJS_Result
bc_gen_block (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstBlock *block)
{
    int            le;
    int            cid;
    int            line;
    RJS_BcCommand *cmd;

    if (rjs_list_is_empty(&block->stmt_list))
        return RJS_OK;

    line = block->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->decl.decl = block->decl;

    if (block->decl)
        rjs_code_gen_binding_init_table(rt, &block->lex_table, block->decl);

    if (bc_ast_get(rt, &block->lex_table)) {
        cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_table.table = bc_ast_get(rt, &block->lex_table);
    }

    if (bc_ast_get(rt, &block->func_table)) {
        cid = bc_cmd_add(rt, bg, RJS_BC_func_table_init, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->func_table.table = bc_ast_get(rt, &block->func_table);
    }

    le = bc_label_add(rt, bg);
    block->break_js.label  = le;
    block->break_js.rv_reg = bg->rv_reg;

    rjs_code_gen_push_decl(rt, block->decl);
    bc_gen_stmt_list(rt, bg, &block->stmt_list);
    rjs_code_gen_pop_decl(rt);

    line = block->ast.location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = le;

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Generate a if statement.*/
static RJS_Result
bc_gen_if_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstIfStmt *ifs)
{
    RJS_Ast       *ast;
    int            cid;
    int            rid;
    int            l1, l2;
    int            line;
    RJS_BcCommand *cmd;

    line = ifs->ast.location.first_line;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    rid = bc_reg_add(rt, bg);
    l1  = bc_label_add(rt, bg);
    l2  = bc_label_add(rt, bg);
    ast = bc_ast_get(rt, &ifs->cond);

    ifs->break_js.label  = l2;
    ifs->break_js.rv_reg = bg->rv_reg;

    bc_gen_expr(rt, bg, ast, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.value = rid;
    cmd->jump_cond.label = l1;

    line = ifs->ast.location.last_line;

    ast = bc_ast_get(rt, &ifs->if_stmt);
    if (ast) {
        bc_gen_stmt(rt, bg, ast);

        line = ast->location.last_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump.label = l2;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = l1;

    ast = bc_ast_get(rt, &ifs->else_stmt);
    if (ast) {
        bc_gen_stmt(rt, bg, ast);

        line = ast->location.last_line;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = l2;

    return RJS_OK;
}

/*Generate a do while statement.*/
static RJS_Result
bc_gen_do_while_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstLoopStmt *ls)
{
    RJS_Ast       *ast;
    int            cid;
    int            rid;
    int            lb, le, lc;
    int            line;
    RJS_BcCommand *cmd;

    rid = bc_reg_add(rt, bg);
    lb  = bc_label_add(rt, bg);
    le  = bc_label_add(rt, bg);
    lc  = bc_label_add(rt, bg);

    ls->continue_js.label  = lc;
    ls->continue_js.rv_reg = bg->rv_reg;
    ls->break_js.label     = le;
    ls->break_js.rv_reg    = bg->rv_reg;

    line = ls->ast.location.first_line;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lb;

    ast = bc_ast_get(rt, &ls->loop_stmt);
    if (ast)
        bc_gen_stmt(rt, bg, ast);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lc;

    ast = bc_ast_get(rt, &ls->cond);
    bc_gen_expr(rt, bg, ast, rid);

    line = ast->location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_true, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.value = rid;
    cmd->jump_cond.label = lb;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = le;

    return RJS_OK;
}

/*Generate a while statement.*/
static RJS_Result
bc_gen_while_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstLoopStmt *ls)
{
    RJS_Ast       *ast;
    int            cid;
    int            rid;
    int            l1, l2;
    int            line;
    RJS_BcCommand *cmd;

    rid = bc_reg_add(rt, bg);
    l1  = bc_label_add(rt, bg);
    l2  = bc_label_add(rt, bg);

    line = ls->ast.location.first_line;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = l1;

    ast = bc_ast_get(rt, &ls->cond);
    bc_gen_expr(rt, bg, ast, rid);

    line = ast->location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.value = rid;
    cmd->jump_cond.label = l2;

    ls->continue_js.label  = l1;
    ls->continue_js.rv_reg = bg->rv_reg;
    ls->break_js.label     = l2;
    ls->break_js.rv_reg    = bg->rv_reg;

    ast = bc_ast_get(rt, &ls->loop_stmt);
    if (ast)
        bc_gen_stmt(rt, bg, ast);

    line = ls->ast.location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = l1;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = l2;

    return RJS_OK;
}

/*Generate a debugger statement.*/
static RJS_Result
bc_gen_debugger_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *stmt)
{
    int line;

    line = stmt->location.first_line;

    bc_cmd_add(rt, bg, RJS_BC_debugger, line);

    return RJS_OK;
}

/*Generate a throw statement.*/
static RJS_Result
bc_gen_throw_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstExprStmt *ts)
{
    int            line;
    int            cid;
    int            rid;
    RJS_Ast       *ast;
    RJS_BcCommand *cmd;

    rid = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &ts->expr);
    bc_gen_expr(rt, bg, ast, rid);

    line = ts->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_throw_error, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = rid;

    return RJS_OK;
}

/*Generate a return statement.*/
static RJS_Result
bc_gen_return_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstExprStmt *ts)
{
    int            line;
    int            cid;
    int            rid;
    RJS_Ast       *ast;
    RJS_BcCommand *cmd;

    rid = bc_reg_add(rt, bg);

    line = ts->ast.location.first_line;

    ast = bc_ast_get(rt, &ts->expr);
    if (ast) {
        if (ast->type == RJS_AST_CommaExpr) {
            RJS_AstList *list = (RJS_AstList*)ast;
            RJS_Ast     *se;

            rjs_list_foreach_c(&list->list, se, RJS_Ast, ln) {
                int trid;

                if (se->ln.next == &list->list)
                    break;

                trid = bc_reg_add(rt, bg);
                bc_gen_expr(rt, bg, se, trid);
            }

            ast = RJS_CONTAINER_OF(list->list.prev, RJS_Ast, ln);
        }

        if ((ast->type == RJS_AST_CallExpr) && bg->tco) {
            bc_gen_call_expr(rt, bg, (RJS_AstCall*)ast, rid, RJS_TRUE);
        } else if ((ast->type == RJS_AST_Template) && bg->tco) {
            bc_gen_templ_expr(rt, bg, (RJS_AstTemplate*)ast, rid, RJS_TRUE);
        } else {
#if ENABLE_ASYNC && ENABLE_GENERATOR
            if ((bg->func_ast->flags & RJS_AST_FUNC_FL_ASYNC)
                    && (bg->func_ast->flags & RJS_AST_FUNC_FL_GENERATOR)) {
                int trid = bc_reg_add(rt, bg);

                bc_gen_expr(rt, bg, ast, trid);

                cid = bc_cmd_add(rt, bg, RJS_BC_await, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->store.value = trid;

                cid = bc_cmd_add(rt, bg, RJS_BC_await_resume, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->load.dest = rid;
            } else
#endif /*ENABLE_ASYNC && ENABLE_GENERATOR*/
            {
                bc_gen_expr(rt, bg, ast, rid);
            }
        }
    } else {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_return_value, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->store.value = rid;

    return RJS_OK;
}

/*Generate a break or continue statement.*/
static RJS_Result
bc_gen_jump_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstJumpStmt *js)
{
    int            line;
    int            lid;
    int            cid;
    int            i, depth;
    RJS_BcLabel   *label;
    RJS_BcCommand *cmd;

    if (!js->dest)
        return RJS_OK;

    line = js->ast.location.first_line;

    lid   = js->dest->label;
    label = &bg->label.items[lid];

    depth = bg->stack_depth;
    assert(depth >= label->stack_depth);

    for (i = 0; i < depth - label->stack_depth; i ++) {
        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
    }

    if (bg->rv_reg != -1) {
        if (bg->rv_reg != js->dest->rv_reg) {
            cid = bc_cmd_add(rt, bg, RJS_BC_dup, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->unary.operand = bg->rv_reg;
            cmd->unary.result  = js->dest->rv_reg;
        }
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = lid;

    /*Restore the stack depth. These pop state commands do not modify the state stack pointer.*/
    bg->stack_depth = depth;

    return RJS_OK;
}

/*Generate default inintializer.*/
static RJS_Result
bc_gen_default_init (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *init, int rid)
{
    int            cid;
    int            line;
    RJS_BcCommand *cmd;
    int            lid = bc_label_add(rt, bg);
    int            cr  = bc_reg_add(rt, bg);

    if (!init)
        return RJS_OK;

    line = init->location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_is_undefined, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = rid;
    cmd->unary.result  = cr;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.label = lid;
    cmd->jump_cond.value = cr;

    bc_gen_expr(rt, bg, init, rid);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lid;

    return RJS_OK;
}

/*Generate binding element initializer.*/
static RJS_Result
bc_gen_binding_elem_init (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *e, RJS_Bool is_lex)
{
    int            cid;
    int            line;
    RJS_BcCommand *cmd;

    line = e->location.first_line;

    switch (e->type) {
    case RJS_AST_Elision:
        cid = bc_cmd_add(rt, bg, RJS_BC_next_array_item, line);
        break;
    case RJS_AST_BindingElem: {
        RJS_AstBindingElem *be  = (RJS_AstBindingElem*)e;
        int                 rid = bc_reg_add(rt, bg);
        RJS_BcRef           ref;
        RJS_Ast            *b, *init;

        b = bc_ast_get(rt, &be->binding);
        bc_gen_binding_assi_ref(rt, bg, b, &ref);

        cid = bc_cmd_add(rt, bg, RJS_BC_get_array_item, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;

        init = bc_ast_get(rt, &be->init);
        bc_gen_default_init(rt, bg, init, rid);

        b = bc_ast_get(rt, &be->binding);
        bc_gen_binding_assi(rt, bg, b, rid, is_lex, &ref);
        break;
    }
    case RJS_AST_Rest: {
        RJS_AstRest  *rest = (RJS_AstRest*)e;
        int           rid  = bc_reg_add(rt, bg);
        RJS_BcRef     ref;
        RJS_Ast      *b;

        b = bc_ast_get(rt, &rest->binding);
        bc_gen_binding_assi_ref(rt, bg, b, &ref);

        cid = bc_cmd_add(rt, bg, RJS_BC_rest_array_items, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;

        b = bc_ast_get(rt, &rest->binding);
        bc_gen_binding_assi(rt, bg, b, rid, is_lex, &ref);
        break;
    }
    default:
        assert(0);
        break;
    }

    return RJS_OK;
}

/*Generate binding property initializer.*/
static RJS_Result
bc_gen_binding_prop_init (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *e, RJS_Bool is_lex)
{
    int            cid;
    int            line;
    RJS_BcCommand *cmd;

    line = e->location.first_line;

    switch (e->type) {
    case RJS_AST_BindingProp: {
        RJS_AstBindingProp *bp  = (RJS_AstBindingProp*)e;
        int                 tid = bc_reg_add(rt, bg);
        int                 pid = bc_reg_add(rt, bg);
        int                 rid = bc_reg_add(rt, bg);
        RJS_BcRef           ref;
        RJS_Ast            *nast;
        RJS_Ast            *b, *init;

        nast = bc_ast_get(rt, &bp->name);
        bc_gen_expr(rt, bg, nast, tid);

        b = bc_ast_get(rt, &bp->binding);
        bc_gen_binding_assi_ref(rt, bg, b, &ref);

        cid = bc_cmd_add(rt, bg, RJS_BC_to_prop, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = tid;
        cmd->unary.result  = pid;

        cid = bc_cmd_add(rt, bg, RJS_BC_get_object_prop_expr, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = pid;
        cmd->unary.result  = rid;

        init = bc_ast_get(rt, &bp->init);
        bc_gen_default_init(rt, bg, init, rid);

        b = bc_ast_get(rt, &bp->binding);
        bc_gen_binding_assi(rt, bg, b, rid, is_lex, &ref);
        break;
    }
    case RJS_AST_Rest: {
        RJS_AstRest  *rest = (RJS_AstRest*)e;
        int           rid  = bc_reg_add(rt, bg);
        RJS_BcRef     ref;
        RJS_Ast      *b;

        b = bc_ast_get(rt, &rest->binding);
        bc_gen_binding_assi_ref(rt, bg, b, &ref);

        cid = bc_cmd_add(rt, bg, RJS_BC_rest_object_props, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;

        bc_gen_binding_assi(rt, bg, b, rid, is_lex, &ref);
        break;
    }
    default:
        assert(0);
        break;
    }

    return RJS_OK;
}

/*Generate binding assignment reference.*/
static RJS_Result
bc_gen_binding_assi_ref (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *b, RJS_BcRef *ref)
{
    if (b->type == RJS_AST_Id) {
        RJS_AstId     *ir = (RJS_AstId*)b;
        int            cid;
        int            line;
        RJS_BcCommand *cmd;

        line = b->location.first_line;

        ref->env_rid     = bc_reg_add(rt, bg);
        ref->binding_ref = rjs_code_gen_binding_ref(rt,
                &ir->ast.location, &ir->identifier->value);

        cid = bc_cmd_add(rt, bg, RJS_BC_binding_resolve, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_resolve.binding = ref->binding_ref;
        cmd->binding_resolve.env     = ref->env_rid;
    }

    return RJS_OK;
}

/*Generate binding assignment.*/
static RJS_Result
bc_gen_binding_assi (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *b, int rid, RJS_Bool is_lex, RJS_BcRef *ref)
{
    int            cid;
    int            line;
    RJS_BcCommand *cmd;

    line = b->location.first_line;

    switch (b->type) {
    case RJS_AST_Id: {
        if (is_lex) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_init.env     = ref->env_rid;
            cmd->binding_init.binding = ref->binding_ref;
            cmd->binding_init.value   = rid;
        } else {
            bc_gen_binding_set(rt, bg, line, rid, ref);
        }
        break;
    }
    case RJS_AST_ArrayBinding: {
        RJS_AstList *l = (RJS_AstList*)b;
        RJS_Ast     *e;

        cid = bc_cmd_add(rt, bg, RJS_BC_push_array_assi, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = rid;

        rjs_list_foreach_c(&l->list, e, RJS_Ast, ln) {
            bc_gen_binding_elem_init(rt, bg, e, is_lex);
        }

        line = b->location.last_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
        break;
    }
    case RJS_AST_ObjectBinding: {
        RJS_AstList *l = (RJS_AstList*)b;
        RJS_Ast     *e;

        cid = bc_cmd_add(rt, bg, RJS_BC_push_object_assi, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = rid;

        rjs_list_foreach_c(&l->list, e, RJS_Ast, ln) {
            bc_gen_binding_prop_init(rt, bg, e, is_lex);
        }

        line = b->location.last_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
        break;
    }
    default:
        assert(0);
    }

    return RJS_OK;
}

/*Generate parameters initialize code.*/
static RJS_Result
bc_gen_params_init (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstFunc *func)
{
    RJS_Ast  *p_ast;
    int       id     = 0;
    RJS_Bool  is_lex = !(func->flags & RJS_AST_FUNC_FL_DUP_PARAM);

    rjs_list_foreach_c(&func->param_list, p_ast, RJS_Ast, ln) {
        int            cid;
        int            rid;
        int            line;
        RJS_Ast       *b;
        RJS_BcCommand *cmd;
        RJS_BcRef      ref;

        line = p_ast->location.first_line;

        if (p_ast->type == RJS_AST_Rest) {
            RJS_AstRest *rest = (RJS_AstRest*)p_ast;
            b = bc_ast_get(rt, &rest->binding);
        } else {
            RJS_AstBindingElem *be = (RJS_AstBindingElem*)p_ast;
            b = bc_ast_get(rt, &be->binding);
        }

        bc_gen_binding_assi_ref(rt, bg, b, &ref);

        rid = bc_reg_add(rt, bg);

        if (p_ast->type == RJS_AST_Rest) {
            RJS_AstRest *rest = (RJS_AstRest*)p_ast;

            cid = bc_cmd_add(rt, bg, RJS_BC_load_rest_args, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load_rest_args.id   = id;
            cmd->load_rest_args.dest = rid;

            b = bc_ast_get(rt, &rest->binding);
        } else {
            RJS_AstBindingElem *be = (RJS_AstBindingElem*)p_ast;
            RJS_Ast            *init;

            cid = bc_cmd_add(rt, bg, RJS_BC_load_arg, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load_arg.id   = id;
            cmd->load_arg.dest = rid;

            init = bc_ast_get(rt, &be->init);
            bc_gen_default_init(rt, bg, init, rid);

            b = bc_ast_get(rt, &be->binding);
        }

        bc_gen_binding_assi(rt, bg, b, rid, is_lex, &ref);

        id ++;
    }

    return RJS_OK;
}

/*Generate a declaration statement.*/
static RJS_Result
bc_gen_decl_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstList *list)
{
    RJS_AstBindingElem *be;
    RJS_Ast            *ast;
    int                 rid;
    RJS_Bool            is_lex = (list->ast.type != RJS_AST_VarDecl);

    rjs_list_foreach_c(&list->list, be, RJS_AstBindingElem, ast.ln) {
        RJS_Ast  *init;
        RJS_BcRef ref;

        init = bc_ast_get(rt, &be->init);
        if (init) {
            ast = bc_ast_get(rt, &be->binding);
            bc_gen_binding_assi_ref(rt, bg, ast, &ref);

            rid = bc_reg_add(rt, bg);

            bc_gen_expr(rt, bg, init, rid);

            bc_gen_binding_assi(rt, bg, ast, rid, is_lex, &ref);
        } else if (is_lex) {
            int            line;
            int            cid;
            RJS_BcCommand *cmd;

            line = be->ast.location.first_line;

            ast = bc_ast_get(rt, &be->binding);
            bc_gen_binding_assi_ref(rt, bg, ast, &ref);

            rid = bc_reg_add(rt, bg);

            cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load.dest = rid;

            ast = bc_ast_get(rt, &be->binding);
            bc_gen_binding_assi(rt, bg, ast, rid, is_lex, &ref);
        }
    }

    return RJS_OK;
}

/*Generate a for statement.*/
static RJS_Result
bc_gen_for_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstForStmt *fs)
{
    int            line;
    int            cid;
    int            rid;
    int            ls, lc, lb;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    /*Initialize.*/
    line = fs->ast.location.first_line;

    if (fs->decl) {
        rjs_code_gen_binding_init_table(rt, &fs->lex_table, fs->decl);

        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = fs->decl;

        if (bc_ast_get(rt, &fs->lex_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &fs->lex_table);
        }

        rjs_code_gen_push_decl(rt, fs->decl);
    }

    ast = bc_ast_get(rt, &fs->init);
    if (ast) {
        /*Do not update eval return register in initialize statement.*/
        int rv_reg = bg->rv_reg;

        bg->rv_reg = -1;
        bc_gen_stmt(rt, bg, ast);
        bg->rv_reg = rv_reg;
    }

    ls = bc_label_add(rt, bg);
    lc = bc_label_add(rt, bg);
    lb = bc_label_add(rt, bg);

    fs->break_js.label     = lb;
    fs->break_js.rv_reg    = bg->rv_reg;
    fs->continue_js.label  = lc;
    fs->continue_js.rv_reg = bg->rv_reg;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    if (fs->decl) {
        cid = bc_cmd_add(rt, bg, RJS_BC_next_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_table.table = bc_ast_get(rt, &fs->lex_table);
    }

    /*Check loop condition.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = ls;

    ast = bc_ast_get(rt, &fs->cond);
    if (ast) {
        rid = bc_reg_add(rt, bg);

        bc_gen_expr(rt, bg, ast, rid);

        cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->jump_cond.value = rid;
        cmd->jump_cond.label = lb;
    }

    /*Run loop statement.*/
    ast = bc_ast_get(rt, &fs->loop_stmt);
    bc_gen_stmt(rt, bg, ast);

    line = fs->ast.location.last_line;

    /*Step.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lc;

    if (fs->decl) {
        cid = bc_cmd_add(rt, bg, RJS_BC_next_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->binding_table.table = bc_ast_get(rt, &fs->lex_table);
    }

    ast = bc_ast_get(rt, &fs->step);
    if (ast) {
        rid  = bc_reg_add(rt, bg);

        bc_gen_expr(rt, bg, ast, rid);
    }

    /*Loop.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = ls;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lb;

    if (fs->decl)
        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    if (fs->decl)
        rjs_code_gen_pop_decl(rt);

    return RJS_OK;
}

/*Generate a for in/of statement.*/
static RJS_Result
bc_gen_for_in_of_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstForStmt *fs)
{
    int            line;
    int            cid;
    int            rid, dr, vr;
    int            lc, lb;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;
    RJS_Bool       old_tco = bg->tco;

    /*Initialize.*/
    line = fs->ast.location.first_line;

    if (fs->decl) {
        rjs_code_gen_binding_init_table(rt, &fs->lex_table, fs->decl);

        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = fs->decl;

        if (bc_ast_get(rt, &fs->lex_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &fs->lex_table);
        }

        rjs_code_gen_push_decl(rt, fs->decl);
    }

    ast = bc_ast_get(rt, &fs->cond);
    rid = bc_reg_add(rt, bg);
    bc_gen_expr(rt, bg, ast, rid);

    if (fs->decl) {
        rjs_code_gen_pop_decl(rt);
        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
    }

    switch (fs->ast.type) {
    case RJS_AST_ForInStmt:
        cid = bc_cmd_add(rt, bg, RJS_BC_push_enum, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = rid;
        break;
    case RJS_AST_ForOfStmt:
        cid = bc_cmd_add(rt, bg, RJS_BC_push_iter, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = rid;
        break;
#if ENABLE_ASYNC
    case RJS_AST_AwaitForOfStmt:
        cid = bc_cmd_add(rt, bg, RJS_BC_push_async_iter, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = rid;
        break;
#endif /*ENABLE_ASYNC*/
    default:
        assert(0);
    }

    lc = bc_label_add(rt, bg);
    lb = bc_label_add(rt, bg);

    fs->break_js.label     = lb;
    fs->break_js.rv_reg    = bg->rv_reg;
    fs->continue_js.label  = lc;
    fs->continue_js.rv_reg = bg->rv_reg;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    /*Check loop condition.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lc;

    dr = bc_reg_add(rt, bg);
    vr = bc_reg_add(rt, bg);

#if ENABLE_ASYNC
    if (fs->ast.type == RJS_AST_AwaitForOfStmt) {
        cid = bc_cmd_add(rt, bg, RJS_BC_async_for_step, line);
        cmd = bc_cmd_get(bg, cid);

        cid = bc_cmd_add(rt, bg, RJS_BC_async_for_step_resume, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->for_step.done  = dr;
        cmd->for_step.value = vr;
    } else
#endif /*ENABLE_ASYNC*/
    {
        cid = bc_cmd_add(rt, bg, RJS_BC_for_step, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->for_step.done  = dr;
        cmd->for_step.value = vr;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_jump_false, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump_cond.value = dr;
    cmd->jump_cond.label = lb;

    if (fs->decl) {
        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = fs->decl;

        if (bc_ast_get(rt, &fs->lex_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &fs->lex_table);
        }

        rjs_code_gen_push_decl(rt, fs->decl);
    }

    ast = bc_ast_get(rt, &fs->init);
    switch (ast->type) {
    case RJS_AST_VarDecl:
    case RJS_AST_LetDecl:
    case RJS_AST_ConstDecl: {
        RJS_AstList        *l;
        RJS_Bool            is_lex;
        RJS_AstBindingElem *be;
        RJS_Ast            *b;
        RJS_BcRef           ref;

        l      = (RJS_AstList*)ast;
        be     = RJS_CONTAINER_OF(l->list.next, RJS_AstBindingElem, ast.ln);
        b      = bc_ast_get(rt, &be->binding);
        is_lex = (ast->type != RJS_AST_VarDecl);

        bc_gen_binding_assi_ref(rt, bg, b, &ref);
        bc_gen_binding_assi(rt, bg, b, vr, is_lex, &ref);
        break;
    }
    case RJS_AST_ExprStmt: {
        RJS_AstExprStmt *es = (RJS_AstExprStmt*)ast;
        RJS_Ast         *lh = bc_ast_get(rt, &es->expr);
        RJS_BcRef        ref;

        bc_gen_assi_ref(rt, bg, lh, &ref);
        bc_gen_assi(rt, bg, lh, vr, &ref);
        break;
    }
    default:
        assert(0);
    }

    if (fs->ast.type == RJS_AST_AwaitForOfStmt)
        bg->tco = RJS_FALSE;

    /*Run loop statement.*/
    ast = bc_ast_get(rt, &fs->loop_stmt);
    bc_gen_stmt(rt, bg, ast);

    /*Next.*/
    line = fs->ast.location.last_line;

    if (fs->decl) {
        rjs_code_gen_pop_decl(rt);
        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = lc;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lb;

    bg->tco = old_tco;

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Generate a switch statement.*/
static RJS_Result
bc_gen_switch_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstSwitchStmt *ss)
{
    int            line;
    int            cid;
    int            rid;
    int            le;
    int            default_lid = -1;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;
    RJS_AstCase   *cc;

    rid = bc_reg_add(rt, bg);
    ast = bc_ast_get(rt, &ss->cond);
    bc_gen_expr(rt, bg, ast, rid);

    line = ss->ast.location.first_line;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    if (ss->decl) {
        rjs_code_gen_binding_init_table(rt, &ss->lex_table, ss->decl);

        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = ss->decl;

        if (bc_ast_get(rt, &ss->lex_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &ss->lex_table);
        }

        if (bc_ast_get(rt, &ss->func_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_func_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->func_table.table = bc_ast_get(rt, &ss->func_table);
        }

        rjs_code_gen_push_decl(rt, ss->decl);
    }

    le = bc_label_add(rt, bg);

    ss->break_js.label  = le;
    ss->break_js.rv_reg = bg->rv_reg;

    /*Solve case conditions.*/
    rjs_list_foreach_c(&ss->case_list, cc, RJS_AstCase, ast.ln) {
        ast = bc_ast_get(rt, &cc->cond);

        if (ast) {
            int cr  = bc_reg_add(rt, bg);
            int tr  = bc_reg_add(rt, bg);
            int lid = bc_label_add(rt, bg);

            bc_gen_expr(rt, bg, ast, cr);

            line = ast->location.first_line;
            cid  = bc_cmd_add(rt, bg, RJS_BC_strict_eq, line);
            cmd  = bc_cmd_get(bg, cid);
            cmd->binary.operand1 = rid;
            cmd->binary.operand2 = cr;
            cmd->binary.result   = tr;

            cid = bc_cmd_add(rt, bg, RJS_BC_jump_true, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->jump_cond.value = tr;
            cmd->jump_cond.label = lid;

            cc->label = lid;
        } else {
            assert(default_lid == -1);

            default_lid = bc_label_add(rt, bg);

            cc->label = default_lid;
        }
    }

    line = ss->ast.location.first_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);

    if (default_lid != -1) {
        cmd->jump.label = default_lid;
    } else {
        cmd->jump.label = le;
    }

    /*Solve case statements.*/
    rjs_list_foreach_c(&ss->case_list, cc, RJS_AstCase, ast.ln) {
        int lid = cc->label;

        line = cc->ast.location.first_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->stub.label = lid;

        bc_gen_stmt_list(rt, bg, &cc->stmt_list);
    }

    line = ss->ast.location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = le;

    if (ss->decl) {
        rjs_code_gen_pop_decl(rt);

        cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
    }

    return RJS_OK;
}

/*Generate a try statement.*/
static RJS_Result
bc_gen_try_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstTryStmt *ts)
{
    int            line;
    int            cid;
    int            lc, lf, le, lb;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;
    RJS_Bool       old_tco = bg->tco;

    lc = bc_label_add(rt, bg);
    lf = bc_label_add(rt, bg);
    le = bc_label_add(rt, bg);

    /*Try block.*/
    line = ts->ast.location.first_line;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_push_try, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->push_try.catch_label = lc;
    cmd->push_try.final_label = lf;

    lb = bc_label_add(rt, bg);

    ts->break_js.label  = lb;
    ts->break_js.rv_reg = bg->rv_reg;

    bg->tco = RJS_FALSE;

    ast = bc_ast_get(rt, &ts->try_block);
    bc_gen_block(rt, bg, (RJS_AstBlock*)ast);

    cid = bc_cmd_add(rt, bg, RJS_BC_jump, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->jump.label = lf;

    /*Catch block.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lc;

    if (!bc_ast_get(rt, &ts->final_block))
        bg->tco = old_tco;

    ast = bc_ast_get(rt, &ts->catch_block);
    if (ast) {
        int rid;

        rid  = bc_reg_add(rt, bg);
        line = ast->location.first_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_catch_error, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = rid;

        if (ts->catch_decl) {
            RJS_Ast *binding;

            binding = bc_ast_get(rt, &ts->catch_binding);

            if (binding) {
                RJS_BcRef ref;

                rjs_code_gen_binding_init_table(rt, &ts->catch_table, ts->catch_decl);

                cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->decl.decl = ts->catch_decl;

                if (bc_ast_get(rt, &ts->catch_table)) {
                    cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
                    cmd = bc_cmd_get(bg, cid);
                    cmd->binding_table.table = bc_ast_get(rt, &ts->catch_table);
                }

                rjs_code_gen_push_decl(rt, ts->catch_decl);

                bc_gen_binding_assi_ref(rt, bg, binding, &ref);
                bc_gen_binding_assi(rt, bg, binding, rid, RJS_TRUE, &ref);
            }
        }

        bc_gen_block(rt, bg, (RJS_AstBlock*)ast);

        if (ts->catch_decl) {
            rjs_code_gen_pop_decl(rt);

            cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);
        }
    }

    bg->tco = old_tco;

    /*Finally block.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lf;

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = lb;

    ast = bc_ast_get(rt, &ts->final_block);
    if (ast) {
        /*
         * Add a new register to store the return value.
         * Finally will update the return value when break/continue/return.
         */
        int old_rv_reg = bg->rv_reg;

        line = ast->location.first_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_finally, line);

        if (old_rv_reg != -1) {
            bg->rv_reg = bc_reg_add(rt, bg);

            cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->load.dest = bg->rv_reg;
        }

        bc_gen_block(rt, bg, (RJS_AstBlock*)ast);
        
        bg->rv_reg = old_rv_reg;
    }

    line = ts->ast.location.last_line;

    cid = bc_cmd_add(rt, bg, RJS_BC_try_end, line);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = le;

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Generate a with statement.*/
static RJS_Result
bc_gen_with_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstWithStmt *ws)
{
    int            line;
    int            cid;
    int            rid;
    int            le;
    RJS_BcCommand *cmd;
    RJS_Ast       *ast;

    rid = bc_reg_add(rt, bg);

    ast = bc_ast_get(rt, &ws->with_expr);
    bc_gen_expr(rt, bg, ast, rid);

    line = ws->ast.location.first_line;

    if (bg->rv_reg != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_push_with, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->push_with.value = rid;
    cmd->push_with.decl  = ws->decl;

    le = bc_label_add(rt, bg);
    ws->break_js.label  = le;
    ws->break_js.rv_reg = bg->rv_reg;

    rjs_code_gen_push_decl(rt, ws->decl);

    ast = bc_ast_get(rt, &ws->with_stmt);
    bc_gen_stmt(rt, bg, ast);

    rjs_code_gen_pop_decl(rt);

    cid = bc_cmd_add(rt, bg, RJS_BC_stub, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->stub.label = le;

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    return RJS_OK;
}

/*Generate property name.*/
static RJS_Result
bc_gen_prop_name (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *ast, int rr)
{
    int            line;
    int            cid;
    RJS_BcCommand *cmd;
    int            tr;

    if (ast->type == RJS_AST_ValueExpr) {
        RJS_AstValueExpr *ve = (RJS_AstValueExpr*)ast;

        if (rjs_value_is_string(rt, &ve->ve->value)) {
            bc_gen_expr(rt, bg, ast, rr);
            return RJS_OK;
        }
    }

    tr   = bc_reg_add(rt, bg);
    line = ast->location.first_line;

    bc_gen_expr(rt, bg, ast, tr);

    cid  = bc_cmd_add(rt, bg, RJS_BC_to_prop, line);
    cmd  = bc_cmd_get(bg, cid);
    cmd->unary.operand = tr;
    cmd->unary.result  = rr;

    return RJS_OK;
}

/*Generate a class.*/
static RJS_Result
bc_gen_class (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstClassRef *cr, int rr)
{
    int               line;
    int               cid;
    int               pp_rid, cp_rid, c_rid, p_rid, lex_rid = -1;
    RJS_BcCommand    *cmd;
    RJS_AstClassElem *ce;
    RJS_Ast          *ast;
    RJS_AstClass     *c;

    c = cr->clazz;

    rjs_code_gen_binding_init_table(rt, &c->name_table, c->decl);

    line = c->ast.location.first_line;

    if (rr == -1)
        c_rid = bc_reg_add(rt, bg);
    else
        c_rid = rr;

    p_rid  = bc_reg_add(rt, bg);
    pp_rid = bc_reg_add(rt, bg);
    cp_rid = bc_reg_add(rt, bg);

    /*Extends.*/
    ast = bc_ast_get(rt, &c->extends);
    if (ast) {
        rjs_code_gen_push_decl(rt, c->decl);

        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = c->decl;

        if (bc_ast_get(rt, &c->name_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &c->name_table);
        }

        bc_gen_expr(rt, bg, ast, cp_rid);

        cid = bc_cmd_add(rt, bg, RJS_BC_get_proto, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->unary.operand = cp_rid;
        cmd->unary.result  = pp_rid;

        lex_rid = bc_reg_add(rt, bg);
        cid = bc_cmd_add(rt, bg, RJS_BC_save_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = lex_rid;

        rjs_code_gen_pop_decl(rt);
    } else {
        cid = bc_cmd_add(rt, bg, RJS_BC_load_object_proto, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = pp_rid;

        cid = bc_cmd_add(rt, bg, RJS_BC_load_func_proto, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = cp_rid;
    }

    /*Create prototype.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_object_create, line);
    cmd = bc_cmd_get(bg, cid);
    cmd->unary.operand = pp_rid;
    cmd->unary.result  = p_rid;

    cid = bc_cmd_add(rt, bg, RJS_BC_push_class, line);

#if ENABLE_PRIV_NAME
    rjs_code_gen_priv_env_idx(rt, c->priv_env);
    if (c->priv_env->id != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_set_priv_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->set_priv_env.priv_env = c->priv_env;
    }
#endif /*ENABLE_PRIV_NAME*/

    /*Create constructor.*/
    rjs_code_gen_push_decl(rt, c->decl);

    if (lex_rid != -1) {
        cid = bc_cmd_add(rt, bg, RJS_BC_restore_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = lex_rid;
    } else {
        cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->decl.decl = c->decl;

        if (bc_ast_get(rt, &c->name_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &c->name_table);
        }
    }

    if (c->constructor) {
        cid = bc_cmd_add(rt, bg, RJS_BC_constr_create, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->constr_create.proto         = p_rid;
        cmd->constr_create.constr_parent = cp_rid;
        cmd->constr_create.func          = c->constructor->func;
        cmd->constr_create.obj           = c_rid;
    } else {
        if (bc_ast_get(rt, &c->extends))
            cid = bc_cmd_add(rt, bg, RJS_BC_derived_default_constr, line);
        else
            cid = bc_cmd_add(rt, bg, RJS_BC_default_constr, line);
            
        cmd = bc_cmd_get(bg, cid);
        cmd->default_constr.proto         = p_rid;
        cmd->default_constr.constr_parent = cp_rid;
        cmd->default_constr.name          = c->name;
        cmd->default_constr.obj           = c_rid;
    }

    /*Initialize elements.*/
    rjs_list_foreach_c(&c->elem_list, ce, RJS_AstClassElem, ast.ln) {
        int                n_rid   = -1;
#if ENABLE_PRIV_NAME
        RJS_AstValueEntry *ve      = NULL;
        RJS_Bool           is_priv = RJS_FALSE;
#endif /*ENABLE_PRIV_NAME*/

        if (ce == c->constructor)
            continue;

        if (ce->type != RJS_AST_CLASS_ELEM_BLOCK) {
            RJS_Ast *ast = bc_ast_get(rt, &ce->name);

#if ENABLE_PRIV_NAME
            if (ast->type == RJS_AST_PrivId) {
                RJS_AstPrivId *pi = (RJS_AstPrivId*)ast;

                is_priv = RJS_TRUE;
                ve      = pi->ve;
            } else
#endif /*ENABLE_PRIV_NAME*/
            {
                n_rid = bc_reg_add(rt, bg);
                bc_gen_prop_name(rt, bg, ast, n_rid);
            }
        }

        switch (ce->type) {
        case RJS_AST_CLASS_ELEM_FIELD:
#if ENABLE_PRIV_NAME
            if (is_priv) {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_priv_field_add : RJS_BC_priv_inst_field_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->priv_class_elem.priv = ve;
                cmd->priv_class_elem.func = ce->func;
            } else
#endif /*ENABLE_PRIV_NAME*/
            {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_field_add : RJS_BC_inst_field_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = n_rid;
                cmd->class_elem.func = ce->func;
            }

            if (ce->is_af) {
                cid = bc_cmd_add(rt, bg, RJS_BC_set_af_field, line);
            }
            break;
        case RJS_AST_CLASS_ELEM_METHOD:
#if ENABLE_PRIV_NAME
            if (is_priv) {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_static_priv_method_add : RJS_BC_priv_method_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->priv_class_elem.priv = ve;
                cmd->priv_class_elem.func = ce->func;
            } else
#endif /*ENABLE_PRIV_NAME*/
            {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_static_method_add : RJS_BC_method_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = n_rid;
                cmd->class_elem.func = ce->func;
            }
            break;
        case RJS_AST_CLASS_ELEM_GET:
#if ENABLE_PRIV_NAME
            if (is_priv) {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_static_priv_getter_add : RJS_BC_priv_getter_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->priv_class_elem.priv = ve;
                cmd->priv_class_elem.func = ce->func;
            } else
#endif /*ENABLE_PRIV_NAME*/
            {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_static_getter_add : RJS_BC_getter_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = n_rid;
                cmd->class_elem.func = ce->func;
            }
            break;
        case RJS_AST_CLASS_ELEM_SET:
#if ENABLE_PRIV_NAME
            if (is_priv) {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_static_priv_setter_add : RJS_BC_priv_setter_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->priv_class_elem.priv = ve;
                cmd->priv_class_elem.func = ce->func;
            } else
#endif /*ENABLE_PRIV_NAME*/
            {
                cid = bc_cmd_add(rt, bg, ce->is_static ? RJS_BC_static_setter_add : RJS_BC_setter_add, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->class_elem.name = n_rid;
                cmd->class_elem.func = ce->func;
            }
            break;
        case RJS_AST_CLASS_ELEM_BLOCK:
            cid = bc_cmd_add(rt, bg, RJS_BC_static_block_add, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->static_block_add.func = ce->func;
            break;
        }
    }

    line = c->ast.location.last_line;

    /*Initialize the class binding.*/
    if (bc_ast_get(rt, &c->name_table)) {
        RJS_AstBindingRef *br;

        br = rjs_code_gen_binding_ref(rt, &c->ast.location, &c->name->value);
        bc_gen_binding_init(rt, bg, line, br, c_rid);
    }

    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    /*Initialize the class.*/
    cid = bc_cmd_add(rt, bg, RJS_BC_class_init, line);
    cid = bc_cmd_add(rt, bg, RJS_BC_pop_state, line);

    rjs_code_gen_pop_decl(rt);

    /*Initialize the binding.*/
    if (rr == -1) {
        RJS_AstBindingRef *br;

        br = rjs_code_gen_binding_ref(rt, &c->ast.location, &c->binding_name->value);
        bc_gen_binding_init(rt, bg, line, br, c_rid);
    }

    return RJS_OK;
}

/*Generate a default export statement.*/
static RJS_Result
bc_gen_default_expr_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstExprStmt *es)
{
    int                line;
    int                rid;
    RJS_Ast           *expr;
    RJS_AstBindingRef *br;

    rid  = bc_reg_add(rt, bg);
    expr = bc_ast_get(rt, &es->expr);

    bc_gen_expr(rt, bg, expr, rid);

    line = es->ast.location.first_line;

    br = rjs_code_gen_binding_ref(rt, &es->ast.location, rjs_s_star_default_star(rt));
    bc_gen_binding_init(rt, bg, line, br, rid);

    return RJS_OK;
}

/*Generate a statement.*/
static RJS_Result
bc_gen_stmt (RJS_Runtime *rt, RJS_BcGen *bg, RJS_Ast *stmt)
{
    switch (stmt->type) {
    case RJS_AST_EmptyStmt:
        break;
    case RJS_AST_FuncDecl: {
        RJS_AstFuncRef *fr = (RJS_AstFuncRef*)stmt;

        rjs_code_gen_func_idx(rt, fr->func);
        break;
    }
    case RJS_AST_ExprStmt: {
        RJS_AstExprStmt *es = (RJS_AstExprStmt*)stmt;

        bc_gen_expr_stmt(rt, bg, es);
        break;
    }
    case RJS_AST_Block: {
        RJS_AstBlock *block = (RJS_AstBlock*)stmt;

        bc_gen_block(rt, bg, block);
        break;
    }
    case RJS_AST_IfStmt: {
        RJS_AstIfStmt *ifs = (RJS_AstIfStmt*)stmt;

        bc_gen_if_stmt(rt, bg, ifs);
        break;
    }
    case RJS_AST_DoWhileStmt: {
        RJS_AstLoopStmt *ls = (RJS_AstLoopStmt*)stmt;

        bc_gen_do_while_stmt(rt, bg, ls);
        break;
    }
    case RJS_AST_WhileStmt: {
        RJS_AstLoopStmt *ls = (RJS_AstLoopStmt*)stmt;

        bc_gen_while_stmt(rt, bg, ls);
        break;
    }
    case RJS_AST_ForStmt: {
        RJS_AstForStmt *fs = (RJS_AstForStmt*)stmt;

        bc_gen_for_stmt(rt, bg, fs);
        break;
    }
    case RJS_AST_ForInStmt:
    case RJS_AST_ForOfStmt:
    case RJS_AST_AwaitForOfStmt: {
        RJS_AstForStmt *fs = (RJS_AstForStmt*)stmt;

        bc_gen_for_in_of_stmt(rt, bg, fs);
        break;
    }
    case RJS_AST_SwitchStmt: {
        RJS_AstSwitchStmt *ss = (RJS_AstSwitchStmt*)stmt;

        bc_gen_switch_stmt(rt, bg, ss);
        break;
    }
    case RJS_AST_TryStmt: {
        RJS_AstTryStmt *ts = (RJS_AstTryStmt*)stmt;

        bc_gen_try_stmt(rt, bg, ts);
        break;
    }
    case RJS_AST_WithStmt: {
        RJS_AstWithStmt *ws = (RJS_AstWithStmt*)stmt;

        bc_gen_with_stmt(rt, bg, ws);
        break;
    }
    case RJS_AST_DebuggerStmt: {
        bc_gen_debugger_stmt(rt, bg, stmt);
        break;
    }
    case RJS_AST_LabelStmt: {
        RJS_AstLabelStmt *ls  = (RJS_AstLabelStmt*)stmt;
        RJS_Ast          *ast = bc_ast_get(rt, &ls->stmt);

        bc_gen_stmt(rt, bg, ast);
        break;
    }
    case RJS_AST_ThrowStmt: {
        RJS_AstExprStmt *es = (RJS_AstExprStmt*)stmt;

        bc_gen_throw_stmt(rt, bg, es);
        break;
    }
    case RJS_AST_ReturnStmt: {
        RJS_AstExprStmt *es = (RJS_AstExprStmt*)stmt;

        bc_gen_return_stmt(rt, bg, es);
        break;
    }
    case RJS_AST_BreakStmt:
    case RJS_AST_ContinueStmt: {
        RJS_AstJumpStmt *js = (RJS_AstJumpStmt*)stmt;

        bc_gen_jump_stmt(rt, bg, js);
        break;
    }
    case RJS_AST_VarDecl:
    case RJS_AST_LetDecl:
    case RJS_AST_ConstDecl: {
        RJS_AstList *l = (RJS_AstList*)stmt;

        bc_gen_decl_stmt(rt, bg, l);
        break;
    }
    case RJS_AST_ClassDecl: {
        RJS_AstClassRef *cr = (RJS_AstClassRef*)stmt;

        bc_gen_class(rt, bg, cr, -1);
        break;
    }
    case RJS_AST_DefaultExprStmt: {
        RJS_AstExprStmt *es = (RJS_AstExprStmt*)stmt;

        bc_gen_default_expr_stmt(rt, bg, es);
        break;
    }
    default:
        assert(0);
    }

    return RJS_OK;
}

/*Generate the statement list.*/
static RJS_Result
bc_gen_stmt_list (RJS_Runtime *rt, RJS_BcGen *bg, RJS_List *list)
{
    RJS_Ast *ast;

    rjs_list_foreach_c(list, ast, RJS_Ast, ln) {
        bc_gen_stmt(rt, bg, ast);
    }

    return RJS_OK;
}

/*Allocate a register.*/
static RJS_Result
bc_gen_alloc_reg (RJS_Runtime *rt, RJS_BcRegMap *rmap, RJS_BcRegister *reg, int off)
{
    int i;

    if (reg->id != -1)
        return RJS_OK;

    for (i = 0; i < RJS_N_ELEM(rmap->reg_off); i ++) {
        int acc_off = rmap->reg_off[i];

        if ((acc_off == -1) || (acc_off < off)) {
            rmap->reg_off[i] = reg->last_acc_off;
            rmap->reg_num    = RJS_MAX(rmap->reg_num, i + 1);
            reg->id = i;

            return RJS_OK;
        }
    }

    return RJS_ERR;
}

/*Store the label.*/
static RJS_Result
bc_label_store (RJS_Runtime *rt, RJS_BcGen *bg, uint8_t *bc, int lid, int off)
{
    int v;

    v = bg->label.items[lid].cmd_off - off;

    if ((v > 32767) || (v < -32768)) {
        bc_error(rt, _("jump offset must <= 32767 and >= -32768"));
        return RJS_ERR;
    }

    bc[0] = v >> 8;
    bc[1] = v & 0xff;

    return 2;
}

/*Store the register.*/
static RJS_Result
bc_reg_store (RJS_Runtime *rt, RJS_BcGen *bg, uint8_t *bc, int rid)
{
    int v;

    v = bg->reg.items[rid].id;

    bc[0] = v;

    return 1;
}

/*Store the argument index.*/
static RJS_Result
bc_arg_index_store (RJS_Runtime *rt, RJS_BcGen *bg, uint8_t *bc, int id)
{
    int v;

    v = id;

    if (v > 0xff) {
        bc_error(rt, _("argument index must <= 255"));
        return RJS_ERR;
    }

    bc[0] = v;

    return 1;
}

/*Store the index.*/
static RJS_Result
bc_index_store (RJS_Runtime *rt, RJS_BcGen *bg, uint8_t *bc, int id, const char *name)
{
    int v;

    if (id == -1) {
        v = 0xffff;
    } else {
        if (id > 0xfffe) {
            bc_error(rt, _("%s index must <= 65534"), name);
            return RJS_ERR;
        }

        v = id;
    }

    bc[0] = v >> 8;
    bc[1] = v & 0xff;

    return 2;
}

#include <rjs_bc_inc.c>

/*Allocate function.*/
static RJS_Result
bc_gen_alloc_func (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstFunc *ast, RJS_BcFunc *func)
{
    RJS_BcRegMap    rmap;
    int             bsize     = 0;
    int             lsize     = 0;
    int             last_line = 0;
    int             off, ioff;
    uint8_t        *bc;
    RJS_BcLineInfo *li;
    RJS_Result      r;
    RJS_BcCommand  *cmd, *last_cmd;

    /*Set the register's last access offset.*/
    cmd      = bg->cmd.items;
    last_cmd = cmd + bg->cmd.item_num;
    off      = 0;
    while (cmd < last_cmd) {
        /*Set the registers' last access offset.*/
        bc_cmd_set_regs_last_acc_off(rt, bg, cmd, off);

        /*Mark binding references.*/
        bc_cmd_binding_ref(rt, bg, cmd);

        cmd ++;
        off ++;
    }

    /*Remove the unused commands.*/
    cmd      = bg->cmd.items;
    last_cmd = cmd + bg->cmd.item_num;
    while (cmd < last_cmd) {
        switch (cmd->type) {
        case RJS_BC_set_decl:
            rjs_code_gen_decl_idx(rt, cmd->decl.decl);
            if (cmd->decl.decl->id == -1)
                cmd->type = RJS_BC_nop;
            break;
        default:
            break;
        }

        cmd ++;
    }

    /*Allocate registers.*/
    memset(&rmap.reg_off, 0xff, sizeof(rmap.reg_off));
    rmap.reg_num = 0;

    cmd      = bg->cmd.items;
    last_cmd = cmd + bg->cmd.item_num;
    off      = 0;
    ioff     = 0;
    while (cmd < last_cmd) {
        int bs;

        /*Allocate registers.*/
        if ((r = bc_cmd_alloc_regs(rt, bg, cmd, &rmap, off)) == RJS_ERR)
            return r;

        /*Calculate the line information buffer size.*/
        bs = bc_size_table[bc_model_table[cmd->type]];

        if (bs && (cmd->gen.line != last_line)) {
            last_line = cmd->gen.line;
            lsize ++;
        }

        /*Store the label's offset.*/
        if (cmd->type == RJS_BC_stub) {
            RJS_BcCmd_stub *stub  = (RJS_BcCmd_stub*)cmd;
            RJS_BcLabel    *label = &bg->label.items[stub->label];

            label->cmd_off = ioff;
        }

        bsize += bs;
        ioff  += bs;
        cmd   ++;
        off   ++;
    }

    if (bsize > 0xffff) {
        bc_error(rt, _("byte code size > 0xffff"));
        return RJS_ERR;
    }

    if (lsize > 0xffff) {
        bc_error(rt, _("line information size > 0xffff"));
        return RJS_ERR;
    }

    /*Allocate the function.*/
    func->reg_num  = rmap.reg_num;
    func->bc_size  = bsize;
    func->li_size  = lsize;

    rjs_vector_resize(&bg->bc, func->bc_start + func->bc_size, rt);
    rjs_vector_resize(&bg->li, func->li_start + func->li_size, rt);

    /*Store the byte code and line information.*/
    cmd       = bg->cmd.items;
    last_cmd  = cmd + bg->cmd.item_num;
    last_line = 0;
    ioff      = 0;
    bc        = bg->bc.items + func->bc_start;
    li        = bg->li.items + func->li_start;
    while (cmd < last_cmd) {
        int bs;

        bs = bc_size_table[bc_model_table[cmd->type]];

        if (bs) {
            if (cmd->gen.line != last_line) {
                last_line  = cmd->gen.line;
                li->line   = cmd->gen.line;
                li->offset = ioff;
                li ++;
            }

            bc[0] = cmd->type;

            if ((r = bc_cmd_store_bc(rt, bg, cmd, bc + 1, ioff)) == RJS_ERR)
                return r;

            bc += bs;
        }

        ioff += bs;
        cmd  ++;
    }

    return RJS_OK;
}

/**
 * Generate the byte code of the function.
 * \param rt The current runtime.
 * \param bg The code generator.
 * \param func The function's AST.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_bc_gen_func (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstFunc *func)
{
    RJS_BcCommand *cmd;
    int            cid;
    int            line;
    RJS_Result     r;
    RJS_BcFunc    *bcf;

    /*Allocate the function.*/
    RJS_NEW(rt, bcf);

    bcf->bc_start = bg->bc.item_num;
    bcf->li_start = bg->li.item_num;
    bcf->pr_start = func->prop_ref_start;

    rjs_list_append(&bg->func_list, &bcf->ln);
    func->data = bcf;

    /*Clear the byte code generator.*/
    bg->reg.item_num   = 0;
    bg->label.item_num = 0;
    bg->cmd.item_num   = 0;
    bg->stack_depth    = 0;
    bg->func_ast       = func;
    bg->rv_reg         = -1;

    /*Generate the function's byte code.*/
#if ENABLE_SCRIPT
    if (func->flags & RJS_AST_FUNC_FL_SCRIPT) {
        bg->rv_reg = bc_reg_add(rt, bg);

        if (!rjs_list_is_empty(&func->stmt_list)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_script_init, 1);
            cmd = bc_cmd_get(bg, cid);
            cmd->init.decl       = func->var_decl;
            cmd->init.var_table  = bc_ast_get(rt, &func->var_table);
            cmd->init.lex_table  = bc_ast_get(rt, &func->lex_table);
            cmd->init.func_table = bc_ast_get(rt, &func->func_table);
        }
    } else
#endif /*ENABLE_SCRIPT*/
#if ENABLE_MODULE
    if (func->flags & RJS_AST_FUNC_FL_MODULE) {
        bg->mod_decl       = func->var_decl;
        bg->mod_var_table  = bc_ast_get(rt, &func->var_table);
        bg->mod_lex_table  = bc_ast_get(rt, &func->lex_table);
        bg->mod_func_table = bc_ast_get(rt, &func->func_table);
    } else
#endif /*ENABLE_MODULE*/
#if ENABLE_EVAL
    if (func->flags & RJS_AST_FUNC_FL_EVAL) {
        bg->rv_reg = bc_reg_add(rt, bg);

        cid = bc_cmd_add(rt, bg, RJS_BC_load_undefined, 1);
        cmd = bc_cmd_get(bg, cid);
        cmd->load.dest = bg->rv_reg;

        cid = bc_cmd_add(rt, bg, RJS_BC_eval_init, 1);
        cmd = bc_cmd_get(bg, cid);
        cmd->init.decl       = func->var_decl;
        cmd->init.var_table  = bc_ast_get(rt, &func->var_table);
        cmd->init.lex_table  = bc_ast_get(rt, &func->lex_table);
        cmd->init.func_table = bc_ast_get(rt, &func->func_table);
    } else
#endif /*ENABLE_EVAL*/
    {
        line = func->ast.location.first_line;

        /*Parameters.*/
        if (!(func->flags & (RJS_AST_FUNC_FL_SCRIPT|RJS_AST_FUNC_FL_EVAL))
                && (func->flags & RJS_AST_FUNC_FL_EXPR_PARAM)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->decl.decl = func->param_decl;
        } else {
            cid = bc_cmd_add(rt, bg, RJS_BC_set_decl, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->decl.decl = func->param_decl;
        }

        rjs_code_gen_push_decl(rt, func->param_decl);

        /*Parameters binding table initialized.*/
        if (bc_ast_get(rt, &func->param_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &func->param_table);
        }

        /*Arguments object.*/
        if (func->flags & RJS_AST_FUNC_FL_NEED_ARGS) {
            if (func->flags & RJS_AST_FUNC_FL_UNMAP_ARGS) {
                cid = bc_cmd_add(rt, bg, RJS_BC_unmapped_args, line);
            } else {
                cid = bc_cmd_add(rt, bg, RJS_BC_mapped_args, line);
                cmd = bc_cmd_get(bg, cid);
                cmd->binding_table.table = bc_ast_get(rt, &func->param_table);
            }
        }

        /*Parameters.*/
        bc_gen_params_init(rt, bg, func);

        /*Variable declarations.*/
        if (func->var_decl != func->param_decl) {
            cid = bc_cmd_add(rt, bg, RJS_BC_set_var_env, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->decl.decl = func->var_decl;
        }

        if (bc_ast_get(rt, &func->var_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &func->var_table);
        }

        /*Lexically declaraions.*/
        if (func->lex_decl != func->var_decl) {
            cid = bc_cmd_add(rt, bg, RJS_BC_push_lex_env, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->decl.decl = func->lex_decl;
        }

        if (bc_ast_get(rt, &func->lex_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_binding_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->binding_table.table = bc_ast_get(rt, &func->lex_table);
        }

        /*Functions.*/
        if (bc_ast_get(rt, &func->func_table)) {
            cid = bc_cmd_add(rt, bg, RJS_BC_top_func_table_init, line);
            cmd = bc_cmd_get(bg, cid);
            cmd->func_table.table = bc_ast_get(rt, &func->func_table);
        }

#if ENABLE_GENERATOR
        if (func->flags & RJS_AST_FUNC_FL_GENERATOR) {
            cid = bc_cmd_add(rt, bg, RJS_BC_generator_start, line);
        }
#endif /*ENABLE_GENERATOR*/
    }

    rjs_code_gen_push_decl(rt, func->lex_decl);

    /*Statements.*/
    bc_gen_stmt_list(rt, bg, &func->stmt_list);

    /*Return the eval result.*/
    if (func->flags & (RJS_AST_FUNC_FL_EVAL|RJS_AST_FUNC_FL_SCRIPT)) {
        line = func->ast.location.last_line;

        cid = bc_cmd_add(rt, bg, RJS_BC_return_value, line);
        cmd = bc_cmd_get(bg, cid);
        cmd->store.value = bg->rv_reg;
    }

    /*Allocate function.*/
    if ((r = bc_gen_alloc_func(rt, bg, func, bcf)) == RJS_ERR)
        return r;

    bcf->pr_size = func->prop_ref_num;

    return RJS_OK;
}

/**
 * Initialize the byte code generator.
 * \param rt The current runtime.
 * \param bg The byte code generator.
 */
void
rjs_bc_gen_init (RJS_Runtime *rt, RJS_BcGen *bg)
{
    rjs_vector_init(&bg->bc);
    rjs_vector_init(&bg->li);
    rjs_vector_init(&bg->reg);
    rjs_vector_init(&bg->label);
    rjs_vector_init(&bg->cmd);

    rjs_list_init(&bg->func_list);

    bg->rv_reg         = -1;
    bg->opt_end_label  = -1;
    bg->opt_res_reg    = -1;
    bg->tco            = RJS_TRUE;

#if ENABLE_MODULE
    bg->mod_decl       = NULL;
    bg->mod_var_table  = NULL;
    bg->mod_lex_table  = NULL;
    bg->mod_func_table = NULL;
#endif /*ENABLE_MODULE*/
}

/**
 * Release the byte code generator.
 * \param rt The current runtime.
 * \param bg The byte code generator.
 */
void
rjs_bc_gen_deinit (RJS_Runtime *rt, RJS_BcGen *bg)
{
    RJS_BcFunc *f, *nf;

    rjs_vector_deinit(&bg->bc, rt);
    rjs_vector_deinit(&bg->li, rt);
    rjs_vector_deinit(&bg->reg, rt);
    rjs_vector_deinit(&bg->label, rt);
    rjs_vector_deinit(&bg->cmd, rt);

    rjs_list_foreach_safe_c(&bg->func_list, f, nf, RJS_BcFunc, ln) {
        RJS_DEL(rt, f);
    }
}

/*Get the line number from the instruction pointer.*/
static int
bc_func_get_line (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFunc *func, int ip)
{
    size_t begin = func->line_info_start;
    size_t end   = begin + func->line_info_len;
    int    line;

    while (1) {
        size_t mid = (begin + end) >> 1;
        int    off;

        if (mid == begin) {
            line = script->line_info[mid].line;
            break;
        }

        off = script->line_info[mid].offset;

        if (off == ip) {
            line = script->line_info[mid].line;
            break;
        } else if (off < ip) {
            begin = mid;
        } else {
            end = mid;
        }
    }

    return line;
}

/**
 * Disassemble the byte code.
 * \param rt The current runtime.
 * \param fp The output file.
 * \param bc The byte code pointer.
 * \return The size of the byte code.
 */
int
rjs_bc_disassemble (RJS_Runtime *rt, FILE *fp, uint8_t *bc)
{
    return bc_disassemble(rt, fp, bc);
}

/**
 * Disassemble the byte code function.
 * \param rt The current runtime.
 * \param v The script value.
 * \param func The function.
 * \param fp The output file.
 * \param flags The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_function_disassemble (RJS_Runtime *rt, RJS_Value *v,
        RJS_ScriptFunc *func, FILE *fp, int flags)
{
    RJS_Script *script;

    script = rjs_value_get_gc_thing(rt, v);

#if ENABLE_ASYNC
    if (func->flags & RJS_FUNC_FL_ASYNC)
        fprintf(fp, "async ");
#endif /*ENABLE_ASYNC*/
#if ENABLE_GENERATOR
    if (func->flags & RJS_FUNC_FL_GENERATOR)
        fprintf(fp, "* ");
#endif /*ENABLE_GENERATOR*/

    /*Output the function's header.*/
    fprintf(fp, "function %"PRIdPTR" ", func - script->func_table);

    if (func->name_idx != RJS_INVALID_VALUE_INDEX) {
        fprintf(fp, "name: ");
        rjs_script_print_value(rt, script, fp, func->name_idx);
        fprintf(fp, " ");
    }

    fprintf(fp, "length: %d\n", func->param_len);

    if ((flags & RJS_DISASSEMBLE_PROP_REF) && func->prop_ref_len) {
        /*Output property reference.*/
        RJS_ScriptPropRef *pr, *pr_end;
        int                i;

        fprintf(fp, "  property reference:\n");

        pr     = script->prop_ref_table + func->prop_ref_start;
        pr_end = pr + func->prop_ref_len;
        i      = 0;

        while (pr < pr_end) {
            fprintf(fp, "    %d: ", i);
            rjs_script_print_value_pointer(rt, script, fp, pr->prop_name.name);
            fprintf(fp, "\n");

            pr ++;
            i  ++;
        }
    }

    if (flags & RJS_DISASSEMBLE_CODE) {
        /*Output the byte codes.*/
        int      off, line;
        uint8_t *bc, *bc_end;

        fprintf(fp, "  byte code:\n");

        bc     = script->byte_code + func->byte_code_start;
        bc_end = bc + func->byte_code_len;
        off    = 0;

        while (bc < bc_end) {
            int bs;

            line = bc_func_get_line(rt, script, func, off);

            fprintf(fp, "    %05d|%05d: ", line, off);

            bs = bc_disassemble(rt, fp, bc);

            bc  += bs;
            off += bs;

            fprintf(fp, "\n");
        }
    }

    return RJS_OK;
}

/**
 * Get the line number from the instruction pointer.
 * \param rt The current runtime.
 * \param Script Thje script.
 * \param func The function.
 * \param ip The instruction pointer.
 * \return The line number.
 */
int
rjs_function_get_line (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFunc *func, int ip)
{
    return bc_func_get_line(rt, script, func, ip);
}
