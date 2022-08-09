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

/*Free the job.*/
static void
job_free (RJS_Runtime *rt, RJS_Job *job)
{
    if (job->free)
        job->free(rt, job->data);

    RJS_DEL(rt, job);
}

/**
 * Enqueue a new job.
 * \param rt The current runtime.
 * \param func The callback function.
 * \param realm The realm.
 * \param scan The data scan function.
 * \param free The data free function.
 * \param data The parameter data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_job_enqueue (RJS_Runtime *rt, RJS_JobFunc func, RJS_Realm *realm,
        RJS_ScanFunc scan, RJS_FreeFunc free, void *data)
{
    RJS_Job *job;

    assert(func);

    RJS_NEW(rt, job);

    job->func  = func;
    job->realm = realm;
    job->scan  = scan;
    job->free  = free;
    job->data  = data;

    rjs_list_append(&rt->job_list, &job->ln);

    return RJS_OK;
}

/**
 * Resolve all the jobs in queue.
 * \param rt The current runtime.
 */
void
rjs_solve_jobs (RJS_Runtime *rt)
{
    RJS_Job   *j, *nj;
    RJS_Realm *old_realm = rt->rb.bot_realm;

    while (!rjs_list_is_empty(&rt->job_list)) {
        rjs_list_foreach_safe_c(&rt->job_list, j, nj, RJS_Job, ln) {

            rt->rb.bot_realm = j->realm;
            
            j->func(rt, j->data);

            rjs_list_remove(&j->ln);

            job_free(rt, j);
        }
    }

    rt->rb.bot_realm = old_realm;
}

/**
 * Initialize the jobs queue in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_job_init (RJS_Runtime *rt)
{
    rjs_list_init(&rt->job_list);
}

/**
 * Release the jobs in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_job_deinit (RJS_Runtime *rt)
{
    RJS_Job *j, *nj;

    rjs_list_foreach_safe_c(&rt->job_list, j, nj, RJS_Job, ln) {
        job_free(rt, j);
    }
}

/**
 * Scan the reference things in the jobs.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_job (RJS_Runtime *rt)
{
    RJS_Job *j;

    rjs_list_foreach_c(&rt->job_list, j, RJS_Job, ln) {
        if (j->realm)
            rjs_gc_mark(rt, j->realm);

        if (j->scan)
            j->scan(rt, j->data);
    }
}
