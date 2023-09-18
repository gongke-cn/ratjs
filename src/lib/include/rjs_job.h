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
 * Job internal header.
 */

#ifndef _RJS_JOB_INTERNAL_H_
#define _RJS_JOB_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Job.*/
typedef struct {
    RJS_List        ln;    /**< List node data.*/
    void           *data;  /**< Parameter data.*/
    RJS_Realm      *realm; /**< The realm.*/
    RJS_JobFunc     func;  /**< Callback function.*/
    RJS_ScanFunc    scan;  /**< Data scan function.*/
    RJS_FreeFunc    free;  /**< Data free function.*/
} RJS_Job;

/**
 * Initialize the jobs queue in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_job_init (RJS_Runtime *rt);

/**
 * Release the jobs in the rt.
 * \param rt The current runtime.
 */
extern void
rjs_runtime_job_deinit (RJS_Runtime *rt);

/**
 * Scan the reference things in the jobs.
 * \param rt The current runtime.
 */
extern void
rjs_gc_scan_job (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

