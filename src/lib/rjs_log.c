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

/*Log output level.*/
static RJS_LogLevel log_level = RJS_LOG_NONE;

/**
 * Set the log output level.
 * \param level Only the log level >= this value could be output.
 */
void
rjs_log_set_level (RJS_LogLevel level)
{
    log_level = level;
}

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
void
rjs_log (RJS_LogLevel level, const char *tag, const char *file,
        const char *func, int line, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    rjs_log_v(level, tag, file, func, line, fmt, ap);

    va_end(ap);
}

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
void
rjs_log_v (RJS_LogLevel level, const char *tag, const char *file,
        const char *func, int line, const char *fmt, va_list ap)
{
    const char *lstr;
    const char *col;

    if (level < log_level)
        return;

    switch (level) {
    case RJS_LOG_DEBUG:
        lstr = "D";
        col  = "\033[36;1m";
        break;
    case RJS_LOG_INFO:
    default:
        lstr = "I";
        col  = NULL;
        break;
    case RJS_LOG_WARNING:
        lstr = "W";
        col  = "\033[35;1m";
        break;
    case RJS_LOG_ERROR:
        lstr = "E";
        col  = "\033[31;1m";
        break;
    case RJS_LOG_FATAL:
        lstr = "F";
        col  = "\033[33;1m";
        break;
    }

#if ENABLE_COLOR_CONSOLE
    if (!isatty(2))
#endif /*ENABLE_COLOR_CONSOLE*/
        col = NULL;

    fprintf(stderr, "%s%s%s|%s|\"%s\" %s %d: ",
            col ? col : "",
            lstr,
            col ? "\033[0m" : "",
            tag, file, func, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}
