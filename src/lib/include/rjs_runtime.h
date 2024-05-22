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
 * Runtime internal header.
 */

#ifndef _RJS_RUNTIME_INTERNAL_H_
#define _RJS_RUNTIME_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Parser.*/
typedef struct RJS_Parser_s RJS_Parser;

/**Runtime.*/
struct RJS_Runtime_s {
    RJS_RuntimeBase  rb;                   /**< The runtime base data.*/
    size_t           mem_size;             /**< Used memory size now.*/
    size_t           mem_max_size;         /**< Maximum used memory size.*/
    RJS_NativeStack  native_stack;         /**< The base native stack.*/
    RJS_Realm       *main_realm;           /**< The main realm.*/
    RJS_GcThing     *gc_thing_list;        /**< The GC managed things' list.*/
    int              gc_scan_count;        /**< Scan count.*/
    size_t           gc_last_mem_size;     /**< Memory size after the last collection.*/
    RJS_Bool         agent_can_block;      /**< The agent can be blocked.*/
    RJS_Hash         str_prop_key_hash;    /**< String property keys hash table.*/
    RJS_Value        error;                /**< Error value.*/
    RJS_Bool         error_flag;           /**< Error set flag.*/
    RJS_Bool         throw_dump;           /**< Dump stack when throwing error.*/
    RJS_Context     *error_context;        /**< The context throw the error.*/
    size_t           error_ip;             /**< The instruction pointer where throw the error.*/
    RJS_Value        strings[RJS_S_MAX];   /**< Internal strings.*/
    RJS_Value        prop_name_values[RJS_PN_MAX]; /**< Internal property name values.*/
    RJS_PropertyName prop_names[RJS_PN_MAX];       /**< Internal property names.*/
    RJS_CharBuffer   tmp_cb;               /**< Temporary character buffer.*/
    RJS_Parser      *parser;               /**< The parser.*/
    RJS_Environment *env;                  /**< The environment.*/
    RJS_List         job_list;             /**< Job list.*/
    RJS_ModuleLookupFunc mod_lookup_func;  /**< Module lookup function.*/
    RJS_ModuleLoadFunc   mod_load_func;    /**< Module load function.*/
    RJS_NativeData   native_data;          /**< Native data of the runtime.*/
    RJS_Hash         sym_reg_key_hash;     /**< Symbol registry key hash table.*/
    RJS_Hash         sym_reg_sym_hash;     /**< Symbol registry symbol hash table.*/
#if ENABLE_GENERATOR || ENABLE_ASYNC
    RJS_List         gen_ctxt_list;        /**< Generator context list.*/
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/
#if ENABLE_MODULE
    RJS_Hash         mod_hash;             /**< Module hash table.*/
#endif /*ENABLE_MODULE*/
#if ENABLE_WEAK_REF
    RJS_List         weak_ref_list;        /**< Weak references' list.*/
#endif /*ENABLE_WEAK_REF*/
#if ENABLE_FINALIZATION_REGISTRY
    RJS_List         final_cb_list;        /**< Finalization callback list.*/
#endif /*ENABLE_FINALIZATION_REGISTRY*/
#if ENABLE_CTYPE
    RJS_Hash         ctype_hash;           /**< C type hash table.*/
    RJS_Hash         cptr_hash;            /**< C pointer hash table.*/
#endif /*ENABLE_CTYPE*/
};

#ifdef __cplusplus
}
#endif

#endif

