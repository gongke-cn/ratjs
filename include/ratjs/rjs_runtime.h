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
 * Runtime.
 */

#ifndef _rjs_runtime_H_
#define _rjs_runtime_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup runtime Runtime
 * Javascript runtime
 * @{
 */

/**
 * Create a new rt.
 * \return The new rt.
 */
extern RJS_Runtime*
rjs_runtime_new (void);

/**
 * Free an unused rt.
 * \param rt The rt to be freed.
 */
extern void
rjs_runtime_free (RJS_Runtime *rt);

/**
 * Set the user defined data of the runtime.
 * \param rt The runtime.
 * \param data The user defined data.
 * \param scan Scan function to scan referenced things in the data.
 * \param free Free function to release the data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_runtime_set_data (RJS_Runtime *rt, void *data,
        RJS_ScanFunc scan, RJS_FreeFunc free);

/**
 * Get the user defined data of the runtime.
 * \param rt The current runtime.
 * \return The user defined data of the runtime.
 */
extern void*
rjs_runtime_get_data (RJS_Runtime *rt);

/**
 * Set the agent can be block flag.
 * \param rt The current runtime.
 * \param f The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_agent_can_block (RJS_Runtime *rt, RJS_Bool f);

/**
 * Set the module lookup function.
 * \param rt The current runtime.
 * \param fn The function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_module_lookup_func (RJS_Runtime *rt, RJS_ModuleLookupFunc fn);

/**
 * Set the module load function.
 * \param rt The current runtime.
 * \param fn The function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_module_load_func (RJS_Runtime *rt, RJS_ModuleLoadFunc fn);

/**
 * Enable or disable stack dump function when throw an error.
 * \param rt The current runtime.
 * \param enable Enable/disable flag.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_throw_dump (RJS_Runtime *rt, RJS_Bool enable);

/**
 * Get the value undefined.
 * \param rt The current runtime.
 * \return The value undefined.
 */
static inline RJS_Value*
rjs_v_undefined (RJS_Runtime *rt)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    return &rb->v_undefined;
}

/**
 * Get the value null.
 * \param rt The current runtime.
 * \return The value null.
 */
static inline RJS_Value*
rjs_v_null (RJS_Runtime *rt)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    return &rb->v_null;
}

/**
 * Get the argument pointer.
 * \param rt The current runtime.
 * \param args The argument array.
 * \param argc The arguments' count.
 * \param id The argument's index.
 * \return The argument value's pointer.
 */
static inline RJS_Value*
rjs_argument_get (RJS_Runtime *rt, RJS_Value *args, size_t argc, size_t id)
{
    if (id < argc) {
        return rjs_value_buffer_item(rt, args, id);
    } else {
        return rjs_v_undefined(rt);
    }
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

