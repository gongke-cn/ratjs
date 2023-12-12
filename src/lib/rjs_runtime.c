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

static  RJS_ONCE_DECL(once);

/*Release function.*/
static void
deinit (void)
{
    rjs_dtoa_deinit();
}

/*Initialize function.*/
static void
init (void)
{
#if ENABLE_MATH
    struct timeval tv;

    gettimeofday(&tv, NULL);

    srand(tv.tv_sec * 1000000 + tv.tv_usec);
#endif /*ENABLE_MATH*/

#if ENABLE_DATE
    tzset();
#endif /*ENABLE_DATE*/

    rjs_dtoa_init();

    atexit(deinit);
}

/**
 * Create a new rt.
 * \return The new rt.
 */
RJS_Runtime*
rjs_runtime_new (void)
{
    RJS_Runtime *rt;
    size_t       top;
    RJS_Value   *tmp;

    pthread_once(&once, init);

    rt = malloc(sizeof(RJS_Runtime));
    assert(rt);

    RJS_LOGD("create rt %p", rt);

    rjs_char_buffer_init(rt, &rt->tmp_cb);

    rjs_runtime_mem_init(rt);

    rjs_native_stack_init(&rt->native_stack);
    rt->rb.curr_native_stack = &rt->native_stack;

    rjs_runtime_gc_init(rt);
    rjs_runtime_string_init(rt);
    rjs_runtime_context_init(rt);
    rjs_runtime_job_init(rt);

    rt->agent_can_block = RJS_TRUE;

    rt->main_realm    = NULL;
    rt->rb.bot_realm  = NULL;
    rt->parser        = NULL;
    rt->env           = NULL;
    rt->error_ip      = 0;
    rt->error_flag    = RJS_FALSE;
    rt->throw_dump    = RJS_FALSE;
    rt->error_context = NULL;
    rt->mod_path_func = NULL;

    rjs_native_data_init(&rt->native_data);

    rjs_value_set_null(rt, &rt->rb.v_null);
    rjs_value_set_undefined(rt, &rt->rb.v_undefined);
    rjs_value_set_undefined(rt, &rt->error);

#if ENABLE_GENERATOR || ENABLE_ASYNC
    rjs_list_init(&rt->gen_ctxt_list);
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

#if ENABLE_FINALIZATION_REGISTRY
    rjs_runtime_finalization_registry_init(rt);
#endif /*ENABLE_FINALIZATION_REGISTRY*/

#if ENABLE_WEAK_REF
    rjs_runtime_weak_ref_init(rt);
#endif /*ENABLE_WEAK_REF*/

#if ENABLE_MODULE
    rjs_runtime_module_init(rt);
#endif /*ENABLE_MODULE*/

    /*Create symbol registry.*/
    rjs_runtime_symbol_registry_init(rt);

#if ENABLE_CTYPE
    rjs_runtime_ctype_init(rt);
#endif /*ENABLE_CTYPE*/

    rt->rb.gc_enable = RJS_TRUE;

    /*Create the main realm.*/
    top = rjs_value_stack_save(rt);
    tmp = rjs_value_stack_push(rt);
    rt->main_realm = rjs_realm_new(rt, tmp);
    rjs_value_stack_restore(rt, top);

    rt->rb.bot_realm = rt->main_realm;

    return rt;
}

/**
 * Free an unused rt.
 * \param rt The rt to be freed.
 */
void
rjs_runtime_free (RJS_Runtime *rt)
{
    assert(rt);

#if ENABLE_GENERATOR || ENABLE_ASYNC
    /*Clear all the generator contexts.*/
    rjs_solve_generator_contexts(rt);
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

    /*Clear the main native stack.*/
    rjs_native_stack_clear(rt, &rt->native_stack);

    rjs_runtime_symbol_registry_deinit(rt);

#if ENABLE_MODULE
    rjs_runtime_module_deinit(rt);
#endif /*ENABLE_MODULE*/

#if ENABLE_WEAK_REF
    rjs_runtime_weak_ref_deinit(rt);
#endif /*ENABLE_WEAK_REF*/

#if ENABLE_FINALIZATION_REGISTRY
    rjs_runtime_finalization_registry_deinint(rt);
#endif /*ENABLE_FINALIZATION_REGISTRY*/

    rjs_runtime_context_deinit(rt);
    rjs_native_stack_deinit(rt, &rt->native_stack);
    rjs_runtime_gc_deinit(rt);
    rjs_runtime_job_deinit(rt);
    rjs_runtime_string_deinit(rt);

#if ENABLE_CTYPE
    rjs_runtime_ctype_deinit(rt);
#endif /*ENABLE_CTYPE*/

    rjs_native_data_free(rt, &rt->native_data);

    rjs_char_buffer_deinit(rt, &rt->tmp_cb);
    
    rjs_runtime_mem_deinit(rt);

    RJS_LOGD("free rt %p", rt);

    free(rt);
}

/**
 * Set the agent can be block flag.
 * \param rt The current runtime.
 * \param f The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_agent_can_block (RJS_Runtime *rt, RJS_Bool f)
{
    rt->agent_can_block = f;

    return RJS_OK;
}

/**
 * Set the user defined data of the runtime.
 * \param rt The runtime.
 * \param data The user defined data.
 * \param scan Scan function to scan referenced things in the data.
 * \param free Free function to release the data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_runtime_set_data (RJS_Runtime *rt, void *data,
        RJS_ScanFunc scan, RJS_FreeFunc free)
{
    rjs_native_data_free(rt, &rt->native_data);
    rjs_native_data_set(&rt->native_data, data, scan, free);

    return RJS_OK;
}

/**
 * Get the user defined data of the runtime.
 * \param rt The current runtime.
 * \return The user defined data of the runtime.
 */
void*
rjs_runtime_get_data (RJS_Runtime *rt)
{
    return rt->native_data.data;
}

/**
 * Set the module pathname function.
 * \param rt The current runtime.
 * \param fn The function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_module_path_func (RJS_Runtime *rt, RJS_ModulePathFunc fn)
{
    rt->mod_path_func = fn;

    return RJS_OK;
}

/**
 * Enable or disable stack dump function when throw an error.
 * \param rt The current runtime.
 * \param enable Enable/disable flag.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_throw_dump (RJS_Runtime *rt, RJS_Bool enable)
{
    rt->throw_dump = enable;

    return RJS_OK;
}
