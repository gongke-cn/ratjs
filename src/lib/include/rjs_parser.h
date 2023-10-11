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

/**
 * \file
 * Parser internal header.
 */

#ifndef _RJS_PARSER_INTERNAL_H_
#define _RJS_PARSER_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Operation functions of the AST node.*/
typedef struct {
    RJS_GcThingOps gc_thing_ops; /**< GC thing*/
    size_t         size;         /**< The node's size in bytes.*/
    size_t         value_offset; /**< Value fields' offset.*/
    size_t         value_num;    /**< Value number in the node.*/
    size_t         list_offset;  /**< List fields' offset.*/
    size_t         list_num;     /**< List number in the node.*/
    size_t         hash_offset;  /**< Hash table fields' offset.*/
    size_t         hash_num;     /**< Hash table number in the node.*/
} RJS_AstOps;

/**AST node.*/
typedef struct {
    RJS_GcThing  gc_thing; /**< Base GC thing data.*/
    RJS_Location location; /**< Location of the node.*/
    RJS_List     ln;       /**< List node data.*/
    int          type;     /**< AST node type.*/
} RJS_Ast;

/**Class element type.*/
typedef enum {
    RJS_AST_CLASS_ELEM_METHOD, /**< Method.*/
    RJS_AST_CLASS_ELEM_GET,    /**< Getter of accessor.*/
    RJS_AST_CLASS_ELEM_SET,    /**< Setter of accessor.*/
    RJS_AST_CLASS_ELEM_FIELD,  /**< Field.*/
    RJS_AST_CLASS_ELEM_BLOCK   /**< Block.*/
} RJS_AstClassElemType;

/**Jump stack entry.*/
typedef struct RJS_AstJumpStack_s RJS_AstJumpStack;
/**Jump stack entry.*/
struct RJS_AstJumpStack_s {
    RJS_AstJumpStack *bot;    /**< The bottom entry in the stack.*/
    RJS_Ast          *stmt;   /**< The statement contains this jump position.*/
    int               label;  /**< The label index.*/
    int               rv_reg; /**< Return value register.*/
};

/**Declaration type.*/
typedef enum {
    RJS_AST_DECL_PARAMETER, /**< Parameter.*/
    RJS_AST_DECL_VAR,       /**< var declaration.*/
    RJS_AST_DECL_LET,       /**< let declaration.*/
    RJS_AST_DECL_CONST,     /**< const declaration.*/
    RJS_AST_DECL_STRICT,    /**< Strict const declaration.*/
    RJS_AST_DECL_CLASS,     /**< class declaration.*/
    RJS_AST_DECL_FUNCTION   /**< function declaration.*/
} RJS_AstDeclType;

#include <rjs_ast.h>

/**Immutable binding.*/
#define RJS_AST_BINDING_INIT_IMMUT  1
/**Initialize the binding with undefined.*/
#define RJS_AST_BINDING_INIT_UNDEF  2
/**Initialize the binding with the binding in bottom environment.*/
#define RJS_AST_BINDING_INIT_BOT    4
/**The binding is a strict immutable binding.*/
#define RJS_AST_BINDING_INIT_STRICT 8

/**Getter class element.*/
#define RJS_AST_CLASS_ELEM_FL_GET   1
/**Setter class element.*/
#define RJS_AST_CLASS_ELEM_FL_SET   2
/**Other class element.*/
#define RJS_AST_CLASS_ELEM_FL_OTHER 3

/**Function in strict mode.*/
#define RJS_AST_FUNC_FL_STRICT       1
/**Function need arguments object.*/
#define RJS_AST_FUNC_FL_NEED_ARGS    2

#if ENABLE_ARROW_FUNC
/**Arrow function.*/
#define RJS_AST_FUNC_FL_ARROW        4
#endif /*ENABLE_ARROW_FUNC*/

/**Method.*/
#define RJS_AST_FUNC_FL_METHOD       8
/**The function has "use strict" in it.*/
#define RJS_AST_FUNC_FL_USE_STRICT   16
/**The function has "arguments" parameter.*/
#define RJS_AST_FUNC_FL_ARGS_PARAM   32
/**The function's parameters has expression.*/
#define RJS_AST_FUNC_FL_EXPR_PARAM   64
/**The function's parameters is simple.*/
#define RJS_AST_FUNC_FL_SIMPLE_PARAM 128
/**Need unmapped arguments object.*/
#define RJS_AST_FUNC_FL_UNMAP_ARGS   256
/**Script function.*/
#define RJS_AST_FUNC_FL_SCRIPT       512
/**Module initialize function.*/
#define RJS_AST_FUNC_FL_MODULE       1024
/**Function has duplicate parameters.*/
#define RJS_AST_FUNC_FL_DUP_PARAM    2048
/**Function is a class constructor.*/
#define RJS_AST_FUNC_FL_CLASS_CONSTR 4096
/**Function is a derived constructor.*/
#define RJS_AST_FUNC_FL_DERIVED      8192

#if ENABLE_GENERATOR
/**Generator function.*/
#define RJS_AST_FUNC_FL_GENERATOR    16384
#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC
/**Async function.*/
#define RJS_AST_FUNC_FL_ASYNC        32768
#endif /*ENABLE_ASYNC*/

/**Class field initializer function.*/
#define RJS_AST_FUNC_FL_CLASS_FIELD_INIT 65536
/**Eval function.*/
#define RJS_AST_FUNC_FL_EVAL         131072
/**The function is an expression.*/
#define RJS_AST_FUNC_FL_EXPR         262144
/**The function is getter of an accessor.*/
#define RJS_AST_FUNC_FL_GET          524288
/**The function is setter of an accessor.*/
#define RJS_AST_FUNC_FL_SET          1048576

/**Error flag.*/
#define RJS_PARSE_ST_ERROR      1
/**Current token is cached.*/
#define RJS_PARSE_ST_CURR_TOKEN 2
/**Next token is cached.*/
#define RJS_PARSE_ST_NEXT_TOKEN 4

/**In strict mode.*/
#define RJS_PARSE_FL_STRICT     1
/**Yield expression can be here.*/
#define RJS_PARSE_FL_YIELD      2
/**Await expression can be here.*/
#define RJS_PARSE_FL_AWAIT      4
/**In expression can be here.*/
#define RJS_PARSE_FL_IN         8
/**Return statement can be here.*/
#define RJS_PARSE_FL_RETURN     16
/**Default export.*/
#define RJS_PARSE_FL_DEFAULT    32
/**Parse the module.*/
#define RJS_PARSE_FL_MODULE     64
/**Called by "eval".*/
#define RJS_PARSE_FL_EVAL       128
/**Super call can be here.*/
#define RJS_PARSE_FL_SUPER_CALL 256
/**Super property can be here.*/
#define RJS_PARSE_FL_SUPER_PROP 512
/**Arguments can be here.*/
#define RJS_PARSE_FL_ARGS       1024
/**New target can be here.*/
#define RJS_PARSE_FL_NEW_TARGET 2048
/**The code is in ths class.*/
#define RJS_PARSE_FL_CLASS      4096

/**Parser.*/
struct RJS_Parser_s {
    RJS_Lex       lex;         /**< The lexical analyzer.*/
    int           flags;       /**< Flags.*/
    int           status;      /**< Status.*/
    RJS_Token     curr_token;  /**< Current token.*/
    RJS_Token     next_token;  /**< Next token.*/
    int           last_line;   /**< The last line number.*/
    RJS_AstFunc  *func_stack;  /**< The function stack.*/
    RJS_AstClass *class_stack; /**< The class stack.*/
    RJS_AstNoStrictListStack *no_strict_list_stack; /**< No strict token list stack.*/
    RJS_AstContainsListStack *contains_list_stack;  /**< Contains nodes list stack.*/
    RJS_AstDecl  *decl_stack;  /**< The declaration stack.*/
    RJS_List      func_list;         /**< The functions list.*/
    RJS_List      decl_list;         /**< The declarations list.*/
    RJS_List      class_list;        /**< The classes list.*/
    RJS_List      binding_table_list;/**< The binding table list.*/
    RJS_List      func_table_list;   /**< The function table list.*/
    RJS_List      prop_ref_list;     /**< The property reference list.*/
    size_t        func_num;          /**< The functions' number.*/
    size_t        decl_num;          /**< The declarations' number.*/
    size_t        binding_table_num; /**< Binding table number.*/
    size_t        func_table_num;    /**< Function table number.*/
    size_t        prop_ref_num;      /**< Property reference number.*/
    RJS_List      value_entry_list;  /**< The value entry list.*/
    RJS_Hash      value_entry_hash;  /**< The value entry hash table.*/
#if ENABLE_MODULE
    RJS_List      module_request_list;  /**< Module request entries list.*/
    RJS_List      import_list; /**< Import entries list.*/
    RJS_List      local_export_list; /**< Local export entries list.*/
    RJS_List      indir_export_list; /**< Indirect export entries list.*/
    RJS_List      star_export_list;  /**< Star export entries list.*/
    RJS_Hash      module_request_hash; /**< Requested module hash table.*/
    RJS_Hash      export_hash;       /**< Export entries hash table.*/
    size_t        import_num;        /**< Import entries' number.*/
    size_t        local_export_num;  /**< Local export entries' number.*/
    size_t        indir_export_num;  /**< Indirect export entries' number.*/
    size_t        star_export_num;   /**< Star export entries' number.*/
#endif /*ENABLE_MODULE*/
    size_t        value_entry_num;   /**< Value entries' number.*/
    void         *code_gen;          /**< Code generator.*/
#if ENABLE_PRIV_NAME
    RJS_PrivateEnv *bot_priv_env;    /**< The bottom private environment.*/
    RJS_AstPrivEnv *priv_env_stack;  /**< Private environment stack.*/
    RJS_List        priv_env_list;   /**< Private environment AST node list.*/
    size_t          priv_env_num;    /**< The number of the private environments.*/
    size_t          priv_id_num;     /**< The number of the private identifiers.*/
    RJS_List        priv_id_ref_list;/**< Private identifier reference list.*/
#endif /*ENABLE_PRIV_NAME*/
};

#if ENABLE_EVAL
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
RJS_INTERNAL RJS_Result
rjs_parse_eval (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, int flags, RJS_PrivateEnv *priv_env, RJS_Value *rv);
#endif /*ENABLE_EVAL*/

/**
 * Parse a function.
 * \param rt The current runtime.
 * \param input The input.
 * \param realm The realm.
 * \param[out] rv Return the script value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_parse_function (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, RJS_Value *rv);

#if ENABLE_SCRIPT
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
RJS_INTERNAL RJS_Result
rjs_parse_script (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, int flags, RJS_Value *rv);
#endif /*ENABLE_SCRIPT*/

#if ENABLE_MODULE
/**
 * Parse the module.
 * \param rt The current runtime.
 * \param input The input.
 * \param realm The realm.
 * \param[out] rv The return module value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_parse_module (RJS_Runtime *rt, RJS_Input *input, RJS_Realm *realm, RJS_Value *rv);
#endif /*ENABLE_MODULE*/

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
RJS_INTERNAL RJS_Result
rjs_template_entry_new (RJS_Runtime *rt, RJS_Location *loc,
        RJS_Value *str, RJS_Value *raw, RJS_Token *tok);

/**
 * Output parse error message.
 * \param rt The current runtime.
 * \param loc Location where generate the message.
 * \param fmt Output format.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_parse_error (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...);

/**
 * Output parse warning message.
 * \param rt The current runtime.
 * \param loc Location where generate the message.
 * \param fmt Output format.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_parse_warning (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...);

/**
 * Output parse note message.
 * \param rt The current runtime.
 * \param loc Location where generate the message.
 * \param fmt Output format.
 * \param ... Arguments.
 */
RJS_INTERNAL void
rjs_parse_note (RJS_Runtime *rt, RJS_Location *loc, const char *fmt, ...);

/**
 * Push a declaration to the stack.
 * \param rt The current runtime.
 * \param decl The declaration to be pushed.
 */
RJS_INTERNAL void
rjs_code_gen_push_decl (RJS_Runtime *rt, RJS_AstDecl *decl);

/**
 * Popup the top declaration from the stack.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_code_gen_pop_decl (RJS_Runtime *rt);

/**
 * Create the binding initialize table.
 * \param rt The current runtime.
 * \param tab The binding initlaize table.
 * \param decl The declaration.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error. 
 */
RJS_INTERNAL RJS_Result
rjs_code_gen_binding_init_table (RJS_Runtime *rt, RJS_Value *tab, RJS_AstDecl *decl);

/**
 * Get the value entry.
 * \param rt The current runtime.
 * \param loc The location.
 * \param id The identifier.
 * \return The binding reference.
 */
RJS_INTERNAL RJS_AstValueEntry*
rjs_code_gen_value_entry (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *v);

/**
 * Get the binding reference from the identifier.
 * \param rt The current runtime.
 * \param loc The location.
 * \param id The identifier.
 * \return The binding reference.
 */
RJS_INTERNAL RJS_AstBindingRef*
rjs_code_gen_binding_ref (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *id);

/**
 * Get the property reference from the identifier.
 * \param rt The current runtime.
 * \param v The value to store the reference.
 * \param loc The location.
 * \param func The function contains this property reference.
 * \param id The identifier.
 * \return The binding reference.
 */
RJS_INTERNAL RJS_AstPropRef*
rjs_code_gen_prop_ref (RJS_Runtime *rt, RJS_Value *v, RJS_Location *loc, RJS_AstFunc *func, RJS_Value *id);

/**
 * Get the value entry's index.
 * \param rt The current runtime.
 * \param ve The value entry.
 * \return The value entry's index.
 */
static inline int
rjs_code_gen_value_entry_idx (RJS_Runtime *rt, RJS_AstValueEntry *ve)
{
    RJS_Parser *parser = rt->parser;

    if (!ve)
        return -1;

    if (ve->id == -1)
        ve->id = parser->value_entry_num ++;

    return ve->id;
}

/**
 * Get the identifier value entry's index.
 * \param rt The current runtime.
 * \param ve The value entry.
 * \return The value entry's index.
 */
static inline int
rjs_code_gen_id_entry_idx (RJS_Runtime *rt, RJS_AstValueEntry *ve)
{
    RJS_Parser *parser = rt->parser;

    if (!ve)
        return -1;

    if (ve->id == -1) {
        assert(rjs_value_is_string(rt, &ve->value));

        rjs_string_to_property_key(rt, &ve->value);

        ve->id = parser->value_entry_num ++;
    }

    return ve->id;
}

/**
 * Get the property reference's index.
 * \param rt The current runtime.
 * \param pr The property reference.
 * \return The property reference's index.
 */
static inline int
rjs_code_gen_prop_ref_idx (RJS_Runtime *rt, RJS_AstPropRef *pr)
{
    RJS_Parser *parser = rt->parser;

    if (!pr)
        return -1;

    if (pr->id == -1) {
        pr->id = pr->func->prop_ref_num ++;

        parser->prop_ref_num ++;

#if ENABLE_PRIV_NAME
        if (rjs_value_is_private_name(rt, &pr->prop->value))
            rjs_code_gen_value_entry_idx(rt, pr->prop);
        else
#endif /*ENABLE_PRIV_NAME*/
            rjs_code_gen_id_entry_idx(rt, pr->prop);
    }

    return pr->id;
}

/**
 * Get the function's index.
 * \param rt The current runtime.
 * \param func The function.
 * \return The function's index.
 */
static inline int
rjs_code_gen_func_idx (RJS_Runtime *rt, RJS_AstFunc *func)
{
    RJS_Parser *parser = rt->parser;

    if (!func)
        return -1;

    if (func->id == -1) {
        func->id = parser->func_num ++;

        if (func->name)
            rjs_code_gen_id_entry_idx(rt, func->name);
    }

    return func->id;
}

/**
 * Get the declaration's index.
 * \param rt The current runtime.
 * \param decl The declaration.
 * \return The declaraion's index.
 */
static inline int
rjs_code_gen_decl_idx (RJS_Runtime *rt, RJS_AstDecl *decl)
{
    RJS_Parser *parser = rt->parser;

    if (!decl)
        return -1;

    if ((decl->id == -1) && decl->binding_ref_num)
        decl->id = parser->decl_num ++;

    return decl->id;
}

/**
 * Get the binding reference's index.
 * \param rt The current runtime.
 * \param br The binding reference.
 * \return The binding reference's index.
 */
static inline int
rjs_code_gen_binding_ref_idx (RJS_Runtime *rt, RJS_AstBindingRef *br)
{
    if (!br)
        return -1;

    if (br->id == -1) {
        RJS_AstDecl *decl = br->decl;

        br->id = decl->binding_ref_num ++;
        rjs_code_gen_id_entry_idx(rt, br->name);
    }

    return br->id;
}

/**
 * Get the binding table's index.
 * \param rt The current runtime.
 * \param bt The binding table.
 * \return The binding table's index.
 */
RJS_INTERNAL int
rjs_code_gen_binding_table_idx (RJS_Runtime *rt, RJS_AstBindingTable *bt);

/**
 * Get the function table's index.
 * \param rt The current runtime.
 * \param ft The function table.
 * \return The function table's index.
 */
RJS_INTERNAL int
rjs_code_gen_func_table_idx (RJS_Runtime *rt, RJS_AstFuncTable *ft);

#if ENABLE_PRIV_NAME

/**
 * Get the private environment's index.
 * \param rt The current runtime.
 * \param pe The private environment AST node.
 * \return The private environment's index.
 */
RJS_INTERNAL int
rjs_code_gen_priv_env_idx (RJS_Runtime *rt, RJS_AstPrivEnv *pe);

#endif /*ENABLE_PRIV_NAME*/

/**
 * Scan the referenced GC things in the parser of the rt.
 * \param rt The current runtime.
 * \param parser The parser to be scanned.
 */
RJS_INTERNAL void
rjs_gc_scan_parser (RJS_Runtime *rt, RJS_Parser *parser);

#ifdef __cplusplus
}
#endif

#endif

