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
 * JSON manager.
 * @module json-manager
 */

import { Log } from "./log";
import { JSONPointer } from "./json-pointer";

let log = new Log("json-manager");

//log.enable = false;

/**
 * JSON manager.
 */
export class JSONManager {
    /**
     * JSON directories.
     * @type {Set<string>}
     */
    static dirs = new Set();

    /**
     * Add a JSON directory.
     * @param {string} dir The JSON directory to be added.
     */
    static addDirectory (dir) {
        this.dirs.add(dir);
    };

    /**
     * Lookup a JSON file.
     * @param {string} name The JSON filename.
     * @returns {string|null} The JSON file's path. Or null if cannot find the file. 
     */
    static lookup (name) {
        for (let dir of this.dirs.values()) {
            let path = `${dir}/name`;

            if (FileState(path) != undefined)
                return path;

            return null;
        }
    };

    /**
     * Resolve the pointers in the JSON object.
     * @param {*} json The JSON object.
     * @param {*} curr The current value.
     */
    static resolvePointers (json, curr = json) {
        if ((typeof curr == "object") && (curr !== null)) {
            if (Array.isArray(curr)) {
                for (let e of curr) {
                    this.resolvePointers(json, e);
                }
            } else {
                for (let [k, v] of Object.entries(curr)) {
                    if (k == "$ref") {
                        let rv = JSONPointer.resolveThrow(v, json);

                        curr[k] = rv;

                        for (let [ek, ev] of Object.entries(rv)) {
                            if (curr[ek] === undefined)
                                curr[ek] = ev;
                        }
                    } else {
                        this.resolvePointers(json, v);
                    }
                }
            }
        }
    };

    /**
     * Load a JSON file.
     * @param {string} path 
     */
    static load (path) {
        let str = File.loadString(path);
        let json = JSON.parse(str);

        this.resolvePointers(json);

        return json;
    };
};