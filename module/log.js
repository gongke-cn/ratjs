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
 * Log message object.
 * @module log
 */

/**
 * @typedef {number} LogLevel
 * @enum LogLevel
 * @property {number} ALL Output all message.
 * @property {number} DEBUG Debug message level.
 * @property {number} MESSAGE Normal message level.
 * @property {number} WARNING Warning message level.
 * @property {number} ERROR Error message level.
 * @property {number} FATAL Fatal error message level.
 * @property {number} NONE Do not output any message.
 */
export const LogLevel = {
    ALL:     0,
    DEBUG:   1,
    MESSAGE: 2,
    WARNING: 3,
    ERROR:   4,
    FATAL:   5,
    NONE:    6
};

/**
 * Log message object.
 */
export class Log {
    /**
     * Current log output level.
     * Only the message >= this level can be outputed.
     * @type {LogLevel}
     */
    static level = LogLevel.NONE;

    /**
     * Verbosely output the process message.
     * @type {boolean}
     */
    static verbose = true;

    /**
     * The log output is enabled.
     * @type {boolean}
     */
    enable;

    /**
     * Create a new log object.
     * @param {string} tag The log message tag.
     * The tag will be prefixed text of the message.
     */
    constructor (tag) {
        this.tag    = tag;
        this.enable = true;
    };

    /**
     * Output the log message.
     * @param {LogLevel} level The message's level.
     * @param {string} msg The message text.
     */
    output (level, msg) {
        if (level < Log.level)
            return;

        let type;

        switch (level) {
        case LogLevel.DEBUG:
            type = "D";
            break;
        case LogLevel.MESSAGE:
            type = "M";
            break;
        case LogLevel.WARNING:
            type = "W";
            break;
        case LogLevel.ERROR:
            type = "E";
            break;
        case LogLevel.FATAL:
            type = "F";
            break;
        }

        print(`${type}: ${this.tag}: ${msg}\n`);
    };

    /**
     * Output debug message.
     * @param {string} msg The message text.
     */
    debug (msg) {
        this.output(LogLevel.DEBUG, msg);
    };

    /**
     * Output normal message.
     * @param {string} msg The message text.
     */
    message (msg) {
        if (Log.verbose)
            print(`${msg}\n`);

        this.output(LogLevel.MESSAGE, msg);
    };

    /**
     * Output warning message.
     * @param {string} msg The message text.
     */
    warning (msg) {
        this.output(LogLevel.WARNING, msg);
    };

    /**
     * Output error message.
     * @param {string} msg The message text.
     */
    error (msg) {
        prerr(`error: ${msg}\n`);
        this.output(LogLevel.ERROR, msg);
    };

    /**
     * Output fatal error message.
     * @param {string} msg The message text.
     */
    fatal (msg) {
        prerr(`fatal: ${msg}\n`);
        this.output(LogLevel.FATAL, msg);
    };
};
