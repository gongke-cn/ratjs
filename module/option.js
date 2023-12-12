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
 * Option parser.
 * @module oxdev/option
 */

import { Log } from "./log";
import { assert } from "./assert";

let log = new Log("option");

//log.enable = false;

/**
 * @typedef {object} OptionDescription
 * @type OptionDescription
 * @property {string} name The name of the option.
 * @property {string} abbr The abbreviation of the option.
 * @property {(string|Array|Object)} argument The argument of the option.
 * "boolean": The option has a boolean type argument.
 * "string": The option has a string type argument.
 * "number": The option has a number type argument.
 * "integer": The option has an integer type argument.
 * Array: The items of array are the enumeration values of the argument.
 * Object: The property keys are the enumeration values of the argument,
 * and the property values are related argument value.
 * @property {string} description The description of the option. 
 */

/**
 * @callback OptionCallback
 * @param {string} name The option's name.
 * @param {*} [arg] The argument of the option.
 * @returns {boolean} false means stop the option parse process.
 */

/**
 * Option parser.
 */
export class OptionParser {
    /**
     * string -> OptionDescription map.
     * @type {Map<string, OptionDescription>}
     */
    optMap = new Map();

    /**
     * Options array.
     * @type {Array.<OptionDescription>}
     */
    options = [];

    /**
     * Create a new option parser.
     * @param {OptionDescription[]} descs The option description array.
     */
    constructor (descs) {
        /*Analyze the option descriptions.*/
        for (let desc of descs) {
            if (desc.name != undefined) {
                assert(!this.optMap.get(desc.name), `option ${desc.name} is not defined`);

                this.optMap.set(desc.name, desc);
            }

            if (desc.abbr != undefined) {
                assert(desc.abbr.length == 1, `option abbreviation "${desc.abbr}" must be a character`);
                assert(!this.optMap.get(desc.abbr), `option ${desc.abbr} is not defined`);

                this.optMap.set(desc.abbr, desc);
            }

            this.options.push(desc);
        }
    };

    /**
     * Check if the value is a valid argument.
     * @private
     * @param {OptionDescription} desc The option's description.
     * @param {string} arg The argument string.
     * @returns {*} The valid argument value.
     * undefined means the value is not a valid argument value.
     */
    checkArgument (desc, arg) {
        let res = undefined;

        switch (desc.argument) {
        case "boolean":
            arg = arg.toLowerCase();

            if ((arg == "true")
                    || (arg == "yes")
                    || (arg == "1"))
                res = true;
            else if ((arg == "false")
                    || (arg == "no")
                    || (arg == "0"))
                res = false;
            break;
        case "number":
            let n = parseFloat(arg);
            if (!isNaN(n))
                res = n;
            break;
        case "integer":
            let i = parseInt(arg);
            if (!isNaN(i))
                res = i;
            break;
        case "string":
            res = arg;
            break;
        default:
            if (Array.isArray(desc.argument)) {
                if (desc.argument.indexOf(arg) != -1)
                    res = arg;
            } else if (typeof desc.argument == "object") {
                res = desc.argument[arg.toUpperCase()];
            }
            break;
        }

        return res;
    }

    /**
     * Parse the arguments.
     * @param {string[]} args The command line arguments.
     * @param {OptionCallback} cb The option callback function.
     * @returns {boolean} true when parse stopped successfully, false when callback return false.
     * @throws {TypeError} When the input options have error.
     */
    parse (args, cb) {
        for (let i = 1; i < args.length; i ++) {
            let arg = args[i];
            let optName, optArg;
            let match;

            match = arg.match(/^--([^=]+)(=(.+))?$/);

            if (match != null) {
                optName = match[1];
                optArg  = match[3];
            } else {
                match = arg.match(/^-(.)(.+)?$/);

                if (match == null) {
                    if (!cb(undefined, arg))
                        return false;
                    continue;
                }

                optName = match[1];
                optArg  = match[2];
            }

            let desc;

            /*Lookup the option.*/
            desc = this.optMap.get(optName);
            if (desc == undefined) {
                for (let opt of this.options) {
                    if (opt.name != undefined) {
                        if (opt.name.startsWith(optName)) {
                            if (desc != undefined) {
                                desc = undefined;
                                break;
                            }
    
                            desc = opt;
                        }
                    }
                }
            }

            if (desc == undefined)
                throw new TypeError(`illegal option "${optName}"`);

            if (desc.name != undefined)
                optName = desc.name;
            else if (desc.abbr != undefined)
                optName = desc.abbr;

            /*Check the argument.*/
            if (desc.argument != undefined) {
                let actArg;

                if (optArg == undefined) {
                    if (i == args.length - 1) {
                        if (desc.default != undefined)
                            actArg = desc.default;
                        else if (desc.argument == "boolean")
                            actArg = true;
                        else
                            throw new TypeError(`option "${optName}" should has an argument`);
                    } else {
                        actArg = this.checkArgument(desc, args[i + 1]);

                        if (actArg != undefined) {
                            i ++;
                        } else if (desc.default != undefined) {
                            actArg = desc.default;
                        } else if (desc.argument == "boolean") {
                            actArg = true;
                        } else {
                            throw new TypeError(`"${args[i + 1]}" is not a valid argument of option "${optName}"`);
                        }
                    }
                } else {
                    let actArg = this.checkArgument(desc, optArg);

                    if (actArg == undefined)
                        throw new TypeError(`"${optArg}" is not a valid argument of option "${optName}"`);
                }

                optArg = actArg;
            } else {
                if (optArg != undefined)
                    throw new TypeError(`option "${optName}" has not argument`);
            }

            /*Invoke the callback.*/
            if (!cb(optName, optArg))
                return false;
        }

        return true;
    };

    /**
     * The description text of the options.
     * @type {string}
     */
    get description () {
        let str = "";

        for (let desc of this.options) {
            let line = "  ";

            if (desc.abbr != undefined) {
                line += `-${desc.abbr}`;

                if (desc.name != undefined)
                    line += ","
            }

            if (desc.name != undefined) {
                line += `--${desc.name}`;
            }

            if (desc.argument != undefined) {
                line += " ";

                switch (desc.argument) {
                case "boolean":
                    line += "BOOLEAN";
                    break;
                case "number":
                    line += "NUMBER";
                    break;
                case "integer":
                    line += "INTEGER";
                    break;
                case "string":
                    line += "STRING";
                    break;
                default:
                    if (Array.isArray(desc.argument))
                        line += desc.argument.join("|");
                    else
                        line += Object.keys(desc.argument).join("|");
                    break;
                }
            }

            if (desc.default) {
                let def;

                if ((typeof desc.argument == "object")
                        && !Array.isArray(desc.argument)) {
                    for (let key of Object.keys(desc.argument)) {
                        if (desc[key] == desc.default) {
                            def = key;
                            break;
                        }
                    }

                    assert(def != undefined, `${desc.name}.default match the argument`);
                } else {
                    def = desc.default;
                }

                line += ` default=${def}`;
            }

            if (desc.description) {
                if (line.length < 20) {
                    line += " ".repeat(20 - line.length);
                } else {
                    line += "\n" + " ".repeat(20);
                }

                line += desc.description;
            }

            str += line;
            str += "\n";
        }

        return str;
    };
};
