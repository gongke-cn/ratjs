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
 * RJS internal header
 */

#ifndef _RJS_INTERNAL_H_
#define _RJS_INTERNAL_H_

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdatomic.h>
#include <ratjs.h>

#if ENC_CONV == ENC_CONV_ICU
    #include <unicode/uchar.h>
    #include <unicode/ucnv.h>
    #include <unicode/unorm2.h>
    #include <unicode/uscript.h>
#elif ENC_CONV == ENC_CONV_ICONV
    #include <iconv.h>
#endif

#if OPTIMIZE_FOR_SIZE == 1
    #define RJS_INTERNAL static
#elif defined __GNUC__ || defined __clang__
    #define RJS_INTERNAL __attribute__ ((visibility("hidden"))) extern
#else
    #define RJS_INTERNAL extern
#endif

#if defined WIN32 || defined WIN64
    #include "include/rjs_arch_win.h"
#else
    #include "include/rjs_arch_linux.h"
#endif

#include <rjs_string_table.h>
#include <rjs_object_table.h>

#include "include/rjs_native_data.h"
#include "include/rjs_runtime.h"
#include "include/rjs_realm.h"

#include <rjs_string_table_function.h>
#include <rjs_object_table_function.h>

#include "include/rjs_rbt.h"
#include "include/rjs_mem.h"
#include "include/rjs_gc.h"
#include "include/rjs_conv.h"
#include "include/rjs_strtoi.h"
#include "include/rjs_dtoa.h"
#include "include/rjs_uchar.h"
#include "include/rjs_sort.h"
#include "include/rjs_string.h"
#include "include/rjs_symbol.h"
#include "include/rjs_object.h"

#if ENABLE_CTYPE
    #include "include/rjs_ctype.h"
#endif

#include "include/rjs_function.h"
#include "include/rjs_for_in_iterator.h"
#include "include/rjs_context.h"
#include "include/rjs_template.h"
#include "include/rjs_environment.h"
#include "include/rjs_decl_env.h"
#include "include/rjs_function_env.h"
#include "include/rjs_object_env.h"
#include "include/rjs_global_env.h"
#include "include/rjs_global_object.h"
#include "include/rjs_job.h"
#include "include/rjs_promise.h"
#include "include/rjs_primitive_object.h"
#include "include/rjs_regexp.h"
#include "include/rjs_script_func_object.h"
#include "include/rjs_builtin_func_object.h"
#include "include/rjs_input.h"
#include "include/rjs_lex.h"
#include "include/rjs_parser.h"
#include "include/rjs_script.h"
#include "include/rjs_bc_gen.h"
#include "include/rjs_arguments_object.h"
#include "include/rjs_operation.h"
#include "include/rjs_native_stack.h"
#include "include/rjs_number.h"

#if ENABLE_BIG_INT
    #include "include/rjs_big_int.h"
#endif

#if ENABLE_PRIV_NAME
    #include "include/rjs_private_name.h"
#endif

#if ENABLE_MODULE
    #include "include/rjs_module.h"
    #include "include/rjs_module_env.h"
    #include "include/rjs_module_ns_object.h"
#endif

#if ENABLE_ASYNC
    #include "include/rjs_async_function.h"
#endif

#if ENABLE_GENERATOR
    #include "include/rjs_generator.h"
#endif

#if ENABLE_BOUND_FUNC
    #include "include/rjs_bound_func_object.h"
#endif

#if ENABLE_EVAL
    #include "include/rjs_eval.h"
#endif

#if ENABLE_ARRAY_BUFFER
    #include "include/rjs_data_block.h"
    #include "include/rjs_array_buffer.h"
#endif

#if ENABLE_FINALIZATION_REGISTRY
    #include "include/rjs_finalization_registry.h"
#endif

#if ENABLE_WEAK_REF
    #include "include/rjs_weak_ref.h"
#endif

#if ENABLE_INT_INDEXED_OBJECT
    #include "include/rjs_int_indexed_object.h"
#endif

#if ENABLE_PROXY
    #include "include/rjs_proxy_object.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif

