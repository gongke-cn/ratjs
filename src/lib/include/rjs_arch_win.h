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
 * Windows architecture header.
 */

#ifndef _RJS_ARCH_WIN_INTERNAL_H_
#define _RJS_ARCH_WIN_INTERNAL_H_

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**Declare an once control device.*/
#define RJS_ONCE_DECL(n) INIT_ONCE n

/**Mutex.*/
typedef HANDLE pthread_mutex_t;

/**Condition variable.*/
typedef HANDLE pthread_cond_t;

/**
 * Run the function once.
 * \param dev The once control device.
 * \param func The function.
 */
extern void
pthread_once (PINIT_ONCE dev, void (*func) (void));

/**
 * Initialize the mutex.
 * \param lock The mutex to be initialized.
 * \param attr Attributes.
 */
extern void
pthread_mutex_init (pthread_mutex_t *lock, void *attr);

/**
 * Destroy the mutex.
 * \param lock The mutex to be freed.
 */
extern void
pthread_mutex_destroy (pthread_mutex_t *lock);

/**
 * Lock the mutex.
 * \param lock The mutex to be locked.
 */
extern void
pthread_mutex_lock (pthread_mutex_t *lock);

/**
 * Unlock the mutex.
 * \param lock The mutex to be unlocked.
 */
extern void
pthread_mutex_unlock (pthread_mutex_t *lock);

/**
 * Initialize the condition variable.
 * \param cond The condition variable.
 * \param attr The attributes of the condition variable.
 */
extern void
pthread_cond_init (pthread_cond_t *cond, void *attr);

/**
 * Destroy the condition variable.
 * \param cond The condition variable to be freed.
 */
extern void
pthread_cond_destroy (pthread_cond_t *cond);

/**
 * Notify the condition variable.
 * \param cond The condition variable.
 */
extern void
pthread_cond_signal (pthread_cond_t *cond);

/**
 * Wait the condition variable.
 * \param cond The condition variable.
 * \param lock The mutex.
 * \retval 0 On success.
 * \retval -1 On error.
 */
extern int
pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *lock);

/**
 * Wait the condition variable with timeout.
 * \param cond The condition variable.
 * \param lock The mutex.
 * \param tv Wait time value.
 * \retval 0 On success.
 * \retval -1 On error.
 * \retval ETIMEDOUT Timeout.
 */
extern int
pthread_cond_timedwait (pthread_cond_t *cond, pthread_mutex_t *lock, struct timespec *tv);

/**
 * Get the current time value.
 * \param clk_id The clock's index.
 * \param[out] tp Return the time value.
 * \retval 0 On success.
 * \retval -1 On error.
 */
extern int
clock_gettime(int clk_id, struct timespec *tp);

/**Perform lazy binding.*/
#define RTLD_LAZY 0

/**
 * Open a dynamic linked library.
 * \param filename The dynamic linked library's filename.
 * \param flags Load flags.
 * \return The library's handle.
 * \retval NULL On error.
 */
extern void*
dlopen (const char *filename, int flags);

/**
 * Release a dynamic linked library.
 * \param ptr The library's handle.
 */
extern int
dlclose (void *ptr);

/**
 * Load a symbol from the dynamic linked library.
 * \param ptr The library's handle.
 * \param symbol The symbol's name.
 * \retval The symbol's pointer.
 * \retval NULL Cannot find the symbol.
 */
extern void*
dlsym (void *ptr, const char *symbol);

/**
 * Get the absolute path of the file.
 * \param path The input path.
 * \param resolved_path The output path's buffer.
 * \return The absolute path's pointer.
 */
extern char*
realpath(const char *path, char *resolved_path);

/**Make a new directory.*/
#define mkdir(path, mode) mkdir(path)

#ifdef __cplusplus
}
#endif

#endif

