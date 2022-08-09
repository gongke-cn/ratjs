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

/*Initialize callback function.*/
static BOOL CALLBACK
init_func (PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    void (*func) (void) = Parameter;

    func();

    return TRUE;
}

/**
 * Run the function once.
 * \param dev The once control device.
 * \param func The function.
 */
void
pthread_once (PINIT_ONCE dev, void (*func) (void))
{
    InitOnceExecuteOnce(dev, init_func, func, NULL);
}

/**
 * Initialize the mutex.
 * \param lock The mutex to be initialized.
 * \param attr Attributes.
 */
void
pthread_mutex_init (pthread_mutex_t *lock, void *attr)
{
    HANDLE handle;

    handle = CreateMutex(NULL, FALSE, NULL);

    *lock = handle;
}

/**
 * Destroy the mutex.
 * \param lock The mutex to be freed.
 */
void
pthread_mutex_destroy (pthread_mutex_t *lock)
{
    HANDLE handle = *lock;

    CloseHandle(handle);
}

/**
 * Lock the mutex.
 * \param lock The mutex to be locked.
 */
void
pthread_mutex_lock (pthread_mutex_t *lock)
{
    HANDLE handle = *lock;

    WaitForSingleObject(handle, INFINITE);
}

/**
 * Unlock the mutex.
 * \param lock The mutex to be unlocked.
 */
void
pthread_mutex_unlock (pthread_mutex_t *lock)
{
    HANDLE handle = *lock;

    ReleaseMutex(handle);
}

/**
 * Initialize the condition variable.
 * \param cond The condition variable.
 * \param attr The attributes of the condition variable.
 */
void
pthread_cond_init (pthread_cond_t *cond, void *attr)
{
    HANDLE handle;

    handle = CreateEvent(NULL, TRUE, FALSE, NULL);

    *cond = handle;
}

/**
 * Destroy the condition variable.
 * \param cond The condition variable to be freed.
 */
void
pthread_cond_destroy (pthread_cond_t *cond)
{
    HANDLE handle = *cond;

    CloseHandle(handle);
}

/**
 * Notify the condition variable.
 * \param cond The condition variable.
 */
void
pthread_cond_signal (pthread_cond_t *cond)
{
    HANDLE handle = *cond;

    SetEvent(handle);
}

/**
 * Wait the condition variable.
 * \param cond The condition variable.
 * \param lock The mutex.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *lock)
{
    HANDLE hcond = *cond;
    HANDLE hlock = *lock;

    ReleaseMutex(hlock);

    WaitForSingleObject(hcond, INFINITE);

    WaitForSingleObject(hlock, INFINITE);

    return 0;
}

/**
 * Wait the condition variable with timeout.
 * \param cond The condition variable.
 * \param lock The mutex.
 * \param tv Wait time value.
 * \retval 0 On success.
 * \retval -1 On error.
 * \retval ETIMEDOUT Timeout.
 */
int
pthread_cond_timedwait (pthread_cond_t *cond, pthread_mutex_t *lock, struct timespec *tv)
{
    HANDLE hcond = *cond;
    HANDLE hlock = *lock;
    DWORD  ms;
    DWORD  r;
    struct timeval now;

    ReleaseMutex(hlock);

    gettimeofday(&now, NULL);

    if (now.tv_sec > tv->tv_sec) {
        ms = 0;
    } else if (now.tv_sec == tv->tv_sec) {
        if (now.tv_usec * 1000 > tv->tv_nsec)
            ms = 0;
        else
            ms = (tv->tv_nsec - now.tv_usec * 1000) / 1000000;
    } else {
        ms = (tv->tv_sec - now.tv_sec) * 1000
                + (tv->tv_nsec - now.tv_usec * 1000) / 1000000;
    }

    r = WaitForSingleObject(hcond, ms);

    WaitForSingleObject(hlock, INFINITE);

    return (r == WAIT_TIMEOUT) ? ETIMEDOUT : 0;
}

/**
 * Get the current time value.
 * \param clk_id The clock's index.
 * \param[out] tp Return the time value.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
clock_gettime(int clk_id, struct timespec *tp)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    tp->tv_sec  = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

/**
 * Open a dynamic linked library.
 * \param filename The dynamic linked library's filename.
 * \param flags Load flags.
 * \return The library's handle.
 * \retval NULL On error.
 */
void*
dlopen (const char *filename, int flags)
{
    HMODULE handle;
    
    handle = LoadLibraryA(filename);

    return handle;
}

/**
 * Release a dynamic linked library.
 * \param ptr The library's handle.
 */
int
dlclose (void *ptr)
{
    HMODULE handle;

    handle = ptr;

    FreeLibrary(handle);

    return 0;
}

/**
 * Load a symbol from the dynamic linked library.
 * \param ptr The library's handle.
 * \param symbol The symbol's name.
 * \retval The symbol's pointer.
 * \retval NULL Cannot find the symbol.
 */
void*
dlsym (void *ptr, const char *symbol)
{
    HMODULE handle;

    handle = ptr;

    return GetProcAddress(handle, symbol);
}

/**
 * Get the absolute path of the file.
 * \param path The input path.
 * \param resolved_path The output path's buffer.
 * \return The absolute path's pointer.
 */
char*
realpath(const char *path, char *resolved_path)
{
    char       *rstr;
    struct stat sb;

    rstr = _fullpath(resolved_path, path, PATH_MAX);

    if (rstr && (stat(rstr, &sb) == -1))
        rstr = NULL;

    return rstr;
}
