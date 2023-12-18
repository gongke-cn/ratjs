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
 * JSON pointer.
 * @module oxdev/json-pointer
 */

import { Log } from "./log";
import { JSONManager } from "./json-manager";

let log = new Log("json-pointer");

//log.enable = false;

/**
 * JSON pointer object.
 */
export class JSONPointer {
    /**
     * Source of the JSON pointer.
     * @private
     * @type {string}
     */
    #source;

    /**
     * The pointers field.
     * @type {Array.<string>}
     */
    #pointers = [];

    /**
     * The pointed filename.
     * @type {string}
     */
    filename;

    /**
     * Throw JSON pointer error.
     * @param {string} msg The error message.
     */
    error (msg) {
        let str = `"${this.#source}: ${msg}"`;

        throw new TypeError(str);
    };

    /**
     * Create a new JSON pointer object.
     * @param {string} src The source of the JSON pointer.
     */
    constructor (src) {
        this.#source = src;

        let idx = src.indexOf("#");
        let dn, ptr;

        if (idx != -1) {
            dn  = src.slice(0, idx);
            ptr = src.slice(idx + 1);

            if (ptr.length == 0)
                ptr = "/";
        } else {
            dn  = src;
            ptr = "/";
        }

        if (dn.length > 0)
            this.filename = dn;

        let p = 0;
        if (ptr[p] == "/")
            ptr = ptr.slice(1);

        let seg = "";
        while (ptr.length > 0) {
            let match = ptr.match(/~0|~1|\//);

            if (match == null) {
                seg += ptr;
                this.#pointers.push(seg);
                break;
            } else {
                seg += ptr.slice(0, match.index);

                switch(match[0]) {
                case "~0":
                    seg += "~";
                    break;
                case "~1":
                    seg += "/";
                    break;
                case "/":
                    this.#pointers.push(seg);
                    seg = "";
                    break;
                }

                ptr = ptr.slice(match.index + match[0].length);
            }
        }
    };

    /**
     * Resolve the JSON pointer.
     * @param {*} curr The current JSON value.
     * @returns {*} The pointed value, or undefined when the pointer is invalid.
     */
    resolve (curr) {
        let res;

        if (this.filename == undefined) {
            if (curr == undefined)
                this.error("base value cannot be undefined");

            res = curr;
        } else {
            res = JSONManager.lookup(this.filename);
            if (res == null)
                this.error(`cannot find ${this.filename}`);
        }

        for (let ptr of this.#pointers) {
            if (Array.isArray(res)) {
                let idx = parseInt(ptr);

                if (isNaN(idx)) {
                    res = undefined;
                    break;
                }

                res = res[ptr];
            } else if (typeof res == "object") {
                res = res[ptr];
            } else {
                res = undefined;
                break;
            }
        }

        return res;
    };

    /**
     * Resolve the JSON pointer.
     * @param {*} curr The current JSON value.
     * @returns {*} The pointed value.
     * @throws {TypeError} The pointer is invalid.
     */
    resolveThrow (curr) {
        let res = this.resolve(curr);
        if (res == undefined)
            this.error(`invalid pointer`);

        return res;
    };

    /**
     * Resolve the JSON pointer.
     * @param {string} src The source of the JSON pointer.
     * @param {*} curr The current JSON value.
     * @returns {*} The pointed value, or undefined when the pointer is invalid.
     */
    static resolve (src, curr) {
        return new JSONPointer(src).resolve(curr);
    };

    /**
     * Resolve the JSON pointer.
     * @param {string} src The source of the JSON pointer.
     * @param {*} curr The current JSON value.
     * @returns {*} The pointed value.
     * @throws {TypeError} The pointer is invalid.
     */
    static resolveThrow (src, curr) {
        return new JSONPointer(src).resolveThrow(curr);
    }
};
