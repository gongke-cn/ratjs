#!/usr/bin/ratjs

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

import { Log, LogLevel } from "../log";
import { OptionParser } from "../option";
import { JSONManager } from "../json-manager";
import { JSONSchema } from "../json-schema";
import { CContext } from "./c-context";
import { NjsGenerator } from "./njs-generator";
import { merge } from "./merge";

let log = new Log("njsgen");

//log.enable = false;

/**
 * Option parser.
 * @type {OptionParser}
 */
const optionParser = new OptionParser([
    {
        abbr: "o",
        argument: "string",
        description: "set the output filename"
    },
    {
        abbr: "v",
        description: "validate the input JSON"
    },
    {
        name: "help",
        description: "show this help message"
    }
]);

/**
 * Show help message.
 */
function showHelp ()
{
    print(`\
Usage: njsgen [OPTION]... FILE
Options:
${optionParser.description}`
    );
}

/**
 * Validate the input object.
 * @param {object} input The input object.
 */
function validateInput (input)
{
    let path = `${dirname(scriptPath())}/njs-schema.json`;
    let schema = JSONManager.load(path);

    new JSONSchema(schema).validateThrow(input);
}

/**
 * Generate the output njs source file.
 * @param {object} input The input object.
 * @param {string} output The output filename.
 */
function generateOutput (input, output)
{
    let ctxt = new CContext(input);
    let gen = new NjsGenerator(ctxt);
    let code = gen.generate();

    merge(output, code);
}

/**
 * Main function of "njsgen"
 */
function main ()
{
    let input, output, validate, help;

    Log.level = LogLevel.ALL;

    /*Parse the options.*/
    optionParser.parse(arguments, (name, arg) => {
        switch (name) {
        case "o":
            output = arg;
            break;
        case "v":
            validate = true;
            break;
        case "help":
            help = true;
            break;
        case undefined:
            if (input !== undefined)
                throw new TypeError(`input filename is already defined`);
            input = arg;
            break;
        }

        return true;
    });

    if (help) {
        showHelp();
        return 0;
    }

    if (input === undefined)
        throw new TypeError(`input filename is not specified`);

    if (output === undefined) {
        output = input.replace(/\.json$/, ".c");
    }

    /*Load the input JSON.*/
    let inputJson = JSONManager.load(input);

    /*Validate the input JSON.*/
    if (validate)
        validateInput(inputJson);

    /*Generate the output file.*/
    generateOutput(inputJson, output);

    return 0;
}
