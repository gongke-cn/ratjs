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
 * Async function object internal header.
 */

#ifndef _RJS_ASYNC_FUNCTION_INTERNAL_H_
#define _RJS_ASYNC_FUNCTION_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Async from sync iterator object.*/
typedef struct {
    RJS_Object   object;       /**< Base object data.*/
    RJS_Value    sync_object;  /**< Sync iterator object.*/
    RJS_Value    sync_method;  /**< Sync method object.*/
    RJS_Iterator sync_iter;    /**< Sync iterator record.*/
} RJS_AsyncFromSyncIterObject;

/**
 * Create a new async function object.
 * \param rt The current runtime.
 * \param[out] f Return the new function.
 * \param proto The prototype.
 * \param script The script.
 * \param sf The script function.
 * \param env The environment.
 * \param priv_env The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_async_function_new (RJS_Runtime *rt, RJS_Value *f, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Environment *env,
        RJS_PrivateEnv *priv_env);

/**
 * Close the async iterator.
 * \param rt The current runtime.
 * \param iter The iterator record.
 * \param op The await operation function.
 * \param ip The integer parameter of the function.
 * \param vp The value parameter of the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_SUSPEND Async wait a promise.
 */
extern RJS_Result
rjs_async_iterator_close (RJS_Runtime *rt, RJS_Iterator *iter, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp);

/**
 * Await the async iterator close operation.
 * \param rt The current runtime.
 * \param type The calling type.
 * \param iv The input value.
 * \param rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_await_async_iterator_close (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv);

#ifdef __cplusplus
}
#endif

#endif

