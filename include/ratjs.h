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
 * Rat javascript engine.
 */

#ifndef _RATJS_H_
#define _RATJS_H_

#include <ratjs/rjs_config.h>

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>

#include <ratjs/rjs_macros.h>
#include <ratjs/rjs_vector.h>
#include <ratjs/rjs_log.h>
#include <ratjs/rjs_types.h>
#include <ratjs/rjs_char_buffer.h>
#include <ratjs/rjs_uchar_buffer.h>
#include <ratjs/rjs_list.h>
#include <ratjs/rjs_hash.h>
#include <ratjs/rjs_input.h>
#include <ratjs/rjs_gc.h>
#include <ratjs/rjs_value.h>
#include <ratjs/rjs_value_list.h>
#include <ratjs/rjs_native_stack.h>
#include <ratjs/rjs_string.h>
#include <ratjs/rjs_symbol.h>
#include <ratjs/rjs_runtime.h>
#include <ratjs/rjs_realm.h>
#include <ratjs/rjs_object.h>
#include <ratjs/rjs_primitive_object.h>
#include <ratjs/rjs_native_object.h>
#include <ratjs/rjs_regexp.h>
#include <ratjs/rjs_array.h>
#include <ratjs/rjs_environment.h>
#include <ratjs/rjs_promise.h>
#include <ratjs/rjs_context.h>
#include <ratjs/rjs_iterator.h>

#if ENABLE_SCRIPT || ENABLE_MODULE
    #include <ratjs/rjs_script.h>
#endif

#if ENABLE_MODULE
    #include <ratjs/rjs_module.h>
#endif

#include <ratjs/rjs_script_func_object.h>
#include <ratjs/rjs_builtin_func_object.h>

#if ENABLE_ARRAY_BUFFER
    #include <ratjs/rjs_data_block.h>
    #include <ratjs/rjs_array_buffer.h>
#endif

#if ENABLE_INT_INDEXED_OBJECT
    #include <ratjs/rjs_typed_array.h>
#endif

#if ENABLE_EVAL
    #include <ratjs/rjs_eval.h>
#endif

#if ENABLE_WEAK_REF
    #include <ratjs/rjs_weak_ref.h>
#endif

#if ENABLE_FINALIZATION_REGISTRY
    #include <ratjs/rjs_finalization_registry.h>
#endif

#include <ratjs/rjs_error.h>
#include <ratjs/rjs_job.h>
#include <ratjs/rjs_operation.h>

#if ENABLE_JSON
    #include <ratjs/rjs_json.h>
#endif

#if ENABLE_CTYPE
    #include <ratjs/rjs_ctype.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif

