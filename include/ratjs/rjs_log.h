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
 * Log message output.
 */

#ifndef _RJS_LOG_H_
#define _RJS_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup log Log
 * Log message output functions
 * @{
 */

/**Log level.*/
typedef enum {
    RJS_LOG_ALL,     /**< All log message.*/
    RJS_LOG_DEBUG,   /**< Debug message.*/
    RJS_LOG_INFO,    /**< Information message.*/
    RJS_LOG_WARNING, /**< Warning message.*/
    RJS_LOG_ERROR,   /**< Error message.*/
    RJS_LOG_FATAL,   /**< Fatal error message.*/
    RJS_LOG_NONE     /**< Do not output.*/
} RJS_LogLevel;

/**
 * Set the log output level.
 * \param level Only the log level >= this value could be output.
 */
extern void
rjs_log_set_level (RJS_LogLevel level);

/**
 * Output log message.
 * \param level Log level.
 * \param tag The message's tag.
 * \param file The filename which generate the log message.
 * \param func The function which generate the log message.
 * \param line The line number where generate the log message.
 * \param fmt The output format pattern.
 * \param ... Arguments.
 */
extern void
rjs_log (RJS_LogLevel level, const char *tag, const char *file,
        const char *func, int line, const char *fmt, ...);

/**
 * Output log message with va_list as its argument.
 * \param level Log level.
 * \param tag The message's tag.
 * \param file The filename which generate the log message.
 * \param func The function which generate the log message.
 * \param line The line number where generate the log message.
 * \param fmt The output format pattern.
 * \param ap Arguments list.
 */
extern void
rjs_log_v (RJS_LogLevel level, const char *tag, const char *file,
        const char *func, int line, const char *fmt, va_list ap);

#ifndef RJS_LOG_TAG
    /**
     * Log message tag string.
     * User can redefine it to its private tag.
     */
    #define RJS_LOG_TAG "ratjs"
#endif

/**Output debug message log.*/
#define RJS_LOGD(a...) \
    rjs_log(RJS_LOG_DEBUG, RJS_LOG_TAG, __FILE__, __FUNCTION__, __LINE__, a)
/**Output information message log.*/
#define RJS_LOGI(a...) \
    rjs_log(RJS_LOG_INFO, RJS_LOG_TAG, __FILE__, __FUNCTION__, __LINE__, a)
/**Output warning message log.*/
#define RJS_LOGW(a...) \
    rjs_log(RJS_LOG_WARNING, RJS_LOG_TAG, __FILE__, __FUNCTION__, __LINE__, a)
/**Output error message log.*/
#define RJS_LOGE(a...) \
    rjs_log(RJS_LOG_ERROR, RJS_LOG_TAG, __FILE__, __FUNCTION__, __LINE__, a)
/**Output fatal error message log.*/
#define RJS_LOGF(a...) \
    rjs_log(RJS_LOG_FATAL, RJS_LOG_TAG, __FILE__, __FUNCTION__, __LINE__, a)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

