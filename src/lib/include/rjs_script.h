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
 * Script internal header.
 */

#ifndef _RJS_SCRIPT_INTERNAL_H_
#define _RJS_SCRIPT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Value index.*/
typedef uint16_t RJS_ValueIndex;
/**Function index.*/
typedef uint16_t RJS_FuncIndex;
/**Function declaration's index.*/
typedef uint16_t RJS_FuncDeclIndex;
/**Declaration index.*/
typedef uint16_t RJS_DeclIndex;
/**Binding's index.*/
typedef uint16_t RJS_BindingIndex;
/**Binding reference's index.*/
typedef uint16_t RJS_BindingRefIndex;
/**Property reference's index.*/
typedef uint16_t RJS_PropRefIndex;
/**Byte code buffer's length.*/
typedef uint16_t RJS_ByteCodeLength;
/**Line information buffer's length.*/
typedef uint16_t RJS_LineInfoLength;
/**Parameters' length.*/
typedef uint8_t  RJS_ParamLength;

#define RJS_INVALID_VALUE_INDEX          0xffff
#define RJS_INVALID_FUNC_INDEX           0xffff
#define RJS_INVALID_DECL_INDEX           0xffff
#define RJS_INVALID_BINDING_INDEX        0xffff
#define RJS_INVALID_BINDING_REF_INDEX    0xffff
#define RJS_INVALID_PROP_REF_INDEX       0xffff
#define RJS_INVALID_BINDING_GROUP_INDEX  0xffff
#define RJS_INVALID_FUNC_GROUP_INDEX     0xffff

/**Function information.*/
struct RJS_ScriptFunc_s {
    uint16_t           flags;           /**< Function flags.*/
    uint16_t           reg_num;         /**< Registers' number.*/
    size_t             byte_code_start; /**< The byte code start position.*/
    size_t             line_info_start; /**< Line information start position.*/
    size_t             prop_ref_start;  /**< Property reference start position.*/
    RJS_ByteCodeLength byte_code_len;   /**< Byte code buffer length of the function.*/
    RJS_LineInfoLength line_info_len;   /**< Line information buffer length of the function.*/
    RJS_PropRefIndex   prop_ref_len;    /**< Property reference buffer length of the function.*/
    RJS_ParamLength    param_len;       /**< Parameters' length.*/
    RJS_ValueIndex     name_idx;        /**< Name of the function.*/
#if ENABLE_FUNC_SOURCE
    RJS_ValueIndex     source_idx;      /**< Source of the function.*/
#endif /*ENABLE_FUNC_SOURCE*/
};

/**Declaration information.*/
struct RJS_ScriptDecl_s {
    size_t               binding_ref_start; /**< The binding reference's start position in the table.*/
    RJS_BindingRefIndex  binding_ref_num;   /**< The binding references' number in the declaration.*/
};

/**Binding group.*/
typedef struct {
    size_t            binding_start; /**< The binding start position in the table.*/
    RJS_BindingIndex  binding_num;   /**< The bindings' number in the table.*/
    RJS_DeclIndex     decl_idx;      /**< The declaration's index.*/
} RJS_ScriptBindingGroup;

/**Function declaration group.*/
typedef struct {
    size_t            func_decl_start; /**< The function declaration start position in the table.*/
    RJS_FuncDeclIndex func_decl_num;   /**< The function declarations' number in the table.*/
    RJS_DeclIndex     decl_idx;        /**< The declaration's index.*/
} RJS_ScriptFuncDeclGroup;

/**The constant binding.*/
#define RJS_SCRIPT_BINDING_FL_CONST  1
/**Initialize with undefined.*/
#define RJS_SCRIPT_BINDING_FL_UNDEF  2
/**Initialized with bottom binding reference.*/
#define RJS_SCRIPT_BINDING_FL_BOT    4
/**The binding is strict immutable binding.*/
#define RJS_SCRIPT_BINDING_FL_STRICT 8

/**Declaration item information.*/
typedef struct {
    uint16_t            flags;       /**< Flags.*/
    RJS_BindingRefIndex ref_idx;     /**< The name value's index.*/
    RJS_BindingRefIndex bot_ref_idx; /**< The bottom binding reference's index.*/
} RJS_ScriptBinding;

/**Binding reference.*/
typedef struct {
    RJS_BindingName binding_name; /**< The binding name.*/
} RJS_ScriptBindingRef;

/**Function declaration's information.*/
typedef struct {
    RJS_BindingRefIndex binding_ref_idx; /**< The binding reference's index.*/ 
    RJS_FuncIndex       func_idx;        /**< The function's index.*/
} RJS_ScriptFuncDecl;

/**The property reference information.*/
typedef struct {
    RJS_PropertyName prop_name; /**< The property name.*/
} RJS_ScriptPropRef;

/**The private identifier.*/
typedef struct {
    RJS_ValueIndex idx; /**< Index of the identifier in the value table.*/
} RJS_ScriptPrivId;

/**The private environment.*/
struct RJS_ScriptPrivEnv_s {
    size_t          priv_id_start; /**< Start of the first identifier in the environment.*/
    RJS_ValueIndex  priv_id_num;   /**< Number of private identifiers in the environment.*/
};

/**Line information.*/
typedef struct {
    int  line;    /**< The line number.*/
    int  offset;  /**< The byte code offset.*/
} RJS_BcLineInfo;

/**Script.*/
struct RJS_Script_s {
    RJS_GcThing              gc_thing;              /**< GC thing's data.*/
    RJS_Script              *base_script;           /**< If the script is eval, pointed to the base script.*/
    char                    *path;                  /**< The path of the sourece file.*/
    RJS_Realm               *realm;                 /**< The realm.*/
#if ENABLE_MODULE
    int                      mod_decl_idx;          /**< Module environment's declaration index.*/
    int                      mod_var_grp_idx;       /**< Module environment's variable group index.*/
    int                      mod_lex_grp_idx;       /**< Module environment's lexical declaration group index.*/
    int                      mod_func_grp_idx;      /**< Module environment's function group index.*/
#endif /*ENABLE_MODULE.*/
    RJS_Value               *value_table;           /**< The value table.*/
    size_t                   value_num;             /**< The values' number in the table.*/
    uint8_t                 *byte_code;             /**< Byte code buffer.*/
    size_t                   byte_code_len;         /**< Byte code buffer's length.*/
    RJS_BcLineInfo          *line_info;             /**< Line information buffer.*/
    size_t                   line_info_num;         /**< Line information entries in the buffer.*/
    RJS_ScriptFunc          *func_table;            /**< Functions table.*/
    size_t                   func_num;              /**< Functions' number in the table.*/
    RJS_ScriptDecl          *decl_table;            /**< Declarations' table.*/
    size_t                   decl_num;              /**< Declarations' number in the table.*/
    RJS_ScriptBinding       *binding_table;         /**< Binding table.*/
    size_t                   binding_num;           /**< Bindings' number in the table.*/
    RJS_ScriptBindingGroup  *binding_group_table;   /**< Binding group table.*/
    size_t                   binding_group_num;     /**< Binding groups' number.*/
    RJS_ScriptFuncDecl      *func_decl_table;       /**< Function declaration table.*/
    size_t                   func_decl_num;         /**< Function declarations' in the table.*/
    RJS_ScriptFuncDeclGroup *func_decl_group_table; /**< Function declaration group table.*/
    size_t                   func_decl_group_num;   /**< Function declaration groups' number.*/
    RJS_ScriptBindingRef    *binding_ref_table;     /**< Binding reference table.*/
    size_t                   binding_ref_num;       /**< Binding refefence items' number in the table.*/
    RJS_ScriptPropRef       *prop_ref_table;        /**< Property reference table.*/
    size_t                   prop_ref_num;          /**< Property reference items' number in the table.*/
#if ENABLE_PRIV_NAME
    RJS_ScriptPrivId        *priv_id_table;         /**< Private identifier table.*/
    size_t                   priv_id_num;           /**< Number of private identifiers in the table.*/
    RJS_ScriptPrivEnv       *priv_env_table;        /**< Private environment table.*/
    size_t                   priv_env_num;          /**< Private environments' number.*/
#endif /*ENABLE_PRIV_NAME*/
};

/**
 * Initialize the script.
 * \param rt The current runtime.
 * \param script The script to be initialized.
 * \param realm The realm.
 */
extern void
rjs_script_init (RJS_Runtime *rt, RJS_Script *script, RJS_Realm *realm);

/**
 * Release the script.
 * \param rt The current runtime.
 * \param script The script to be released.
 */
extern void
rjs_script_deinit (RJS_Runtime *rt, RJS_Script *script);

/**
 * Scan the referenced things in the script.
 * \param rt The current runtime.
 * \param ptr The script's pointer.
 */
extern void
rjs_script_op_gc_scan (RJS_Runtime *rt, void *ptr);

/**
 * Create a new script.
 * \param rt The current runtime.
 * \param[out] v Return the script value.
 * \param realm The realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Script*
rjs_script_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm);

/**
 * Global declaration instantiation.
 * \param rt The current runtime.
 * \param script The script.
 * \param decl The declaration.
 * \param var_grp The variable declaration group.
 * \param lex_grp The lexical declaration group.
 * \param func_grp The functions to be initialized.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_global_declaration_instantiation (RJS_Runtime *rt,
        RJS_Script *script,
        RJS_ScriptDecl *decl,
        RJS_ScriptBindingGroup *var_grp,
        RJS_ScriptBindingGroup *lex_grp,
        RJS_ScriptFuncDeclGroup *func_grp);

/**
 * Initialize the binding group in the environment.
 * \param rt The current runtime.
 * \param script The script.
 * \param grp The binding group to be initialized.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_binding_group_init (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptBindingGroup *grp);

/**
 * Duplicate the bindings from the source environemnt to current environment.
 * \param rt The current runtime.
 * \param scrip The script.
 * \param grp The binding group to be initialized.
 * \param env The current environemnt.
 * \param src The source environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_binding_group_dup (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptBindingGroup *grp,
        RJS_Environment *env, RJS_Environment *src);

/**
 * Initialize the function group.
 * \param rt The current runtime.
 * \param script The script.
 * \param grp The function group.
 * \param is_top The function is in top level of the container function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_func_group_init (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFuncDeclGroup *grp, RJS_Bool is_top);

/**
 * Call the script function.
 * \param rt The current runtime.
 * \param type The call type.
 * \param v The input value.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE The async function is waiting a promise.
 */
extern RJS_Result
rjs_script_func_call (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *v, RJS_Value *rv);

/**
 * Print the value in the script's value table.
 * \param rt The current runtime.
 * \param script The script.
 * \param fp The output file.
 * \param id The value's index.
 */
extern void
rjs_script_print_value (RJS_Runtime *rt, RJS_Script *script, FILE *fp, int id);

/**
 * Print the value in the script's value table.
 * \param rt The current runtime.
 * \param script The script.
 * \param fp The output file.
 * \param v The value pointer.
 */
extern void
rjs_script_print_value_pointer (RJS_Runtime *rt, RJS_Script *script, FILE *fp, RJS_Value *v);

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
extern RJS_Result
rjs_function_disassemble (RJS_Runtime *rt, RJS_Value *v,
        RJS_ScriptFunc *func, FILE *fp, int flags);

/**
 * Get the line number from the instruction pointer.
 * \param rt The current runtime.
 * \param Script Thje script.
 * \param func The function.
 * \param ip The instruction pointer.
 * \return The line number.
 */
extern int
rjs_function_get_line (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFunc *func, int ip);

#ifdef __cplusplus
}
#endif

#endif

