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
 * Byte code generator internal header.
 */

#ifndef _RJS_BC_GEN_INTERNAL_H_
#define _RJS_BC_GEN_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Byte code register.*/
typedef struct {
    int  id;           /**< The register's index.*/
    int  last_acc_off; /**< The last access operation's offset.*/
} RJS_BcRegister;

/**Registers map.*/
typedef struct {
    int  reg_off[256]; /**< The register's last accessed offset.*/
    int  reg_num;      /**< Registers' number.*/
} RJS_BcRegMap;

/**Byte code jump label.*/
typedef struct {
    int  cmd_off;     /**< The command's offset.*/
    int  stack_depth; /**< State stack depth.*/
} RJS_BcLabel;

#include <rjs_bc.h>

/**Byte code function.*/
typedef struct {
    RJS_List ln;       /**< List node data.*/
    size_t   bc_start; /**< Byte code start position.*/
    size_t   bc_size;  /**< Byte code buffer size.*/
    size_t   li_start; /**< Line information start position.*/
    size_t   li_size;  /**< Line information buffer size.*/
    size_t   pr_start; /**< Property reference start position.*/
    size_t   pr_size;  /**< Property reference items' number.*/
    int      reg_num;  /**< Registers' number.*/
} RJS_BcFunc;

/**Byte code generator.*/
typedef struct {
    RJS_VECTOR_DECL(uint8_t)        bc;    /**< Byte code buffer.*/
    RJS_VECTOR_DECL(RJS_BcLineInfo) li;    /**< Line information buffer.*/
    RJS_VECTOR_DECL(RJS_BcLabel)    label; /**< The labels' buffer.*/
    RJS_VECTOR_DECL(RJS_BcRegister) reg;   /**< The registers' buffer.*/
    RJS_VECTOR_DECL(RJS_BcCommand)  cmd;   /**< The commands' buffer.*/
    RJS_List             func_list;     /**< Functions' list.*/
    RJS_AstFunc         *func_ast;      /**< The current function AST.*/
    int                  stack_depth;   /**< The state stack depth.*/
    int                  rv_reg;        /**< Return value register.*/
    int                  opt_end_label; /**< Optional chain end label.*/
    int                  opt_res_reg;   /**< Optional result register.*/
    RJS_Bool             tco;           /**< Tail call optimize enabled.*/
#if ENABLE_MODULE
    RJS_AstDecl         *mod_decl;      /**< The module's declaration.*/
    RJS_AstBindingTable *mod_var_table; /**< The module's variable table.*/
    RJS_AstBindingTable *mod_lex_table; /**< The module's lexical declaration table.*/
    RJS_AstFuncTable    *mod_func_table;/**< The module's function table.*/
#endif /*ENABLE_MODULE*/
} RJS_BcGen;

/**
 * Generate the byte code of the function.
 * \param rt The current runtime.
 * \param bg The code generator.
 * \param func The function's AST.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_bc_gen_func (RJS_Runtime *rt, RJS_BcGen *bg, RJS_AstFunc *func);

/**
 * Initialize the byte code generator.
 * \param rt The current runtime.
 * \param bg The byte code generator.
 */
RJS_INTERNAL void
rjs_bc_gen_init (RJS_Runtime *rt, RJS_BcGen *bg);

/**
 * Release the byte code generator.
 * \param rt The current runtime.
 * \param bg The byte code generator.
 */
RJS_INTERNAL void
rjs_bc_gen_deinit (RJS_Runtime *rt, RJS_BcGen *bg);

/**
 * Call the byte code function.
 * \param relam The current runtime.
 * \param type Call type.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On sucess.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_bc_call (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *v, RJS_Value *rv);

/**
 * Disassemble the byte code.
 * \param rt The current runtime.
 * \param fp The output file.
 * \param bc The byte code pointer.
 * \return The size of the byte code.
 */
RJS_INTERNAL int
rjs_bc_disassemble (RJS_Runtime *rt, FILE *fp, uint8_t *bc);

#ifdef __cplusplus
}
#endif

#endif

