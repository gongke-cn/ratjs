# Rat Javascript - a small javascript/ecmascript interpreter

[中文](README_zh.md)

RATJS is a small javascript/ecmascript interpreter written in C. You can use it to run javascript programs, or embed it into your program as a script engine

## Feature

* Compatible with ECMA262 14th edition
	+ Symbol
	+ Generator
	+ Promise
	+ Async function
	+ Arrow function
	+ Async module
	+ Big integer
	+ Typed array
	+ Array buffer/Shared array buffer
	+ DataView
	+ Atomics
	+ Map/Set/WeakSet/WeakMap
	+ WeakRef/FinalizationRegistry
	+ Private identifier
	+ Multiply realm
	+ Module/Async module
* Extension
	+ Hash bang comment
	+ Native module
	+ JSON module
	+ File system functions
	+ FileState object
	+ File object
	+ Directory object
* Fully implemented in C language
* Low footprint
* Rich configuration options

## Dependence

* [icu4c](http://icu-project.org/download/latest_milestone.html): Required if configured with ENC_CONV=icu
* [gmp](http://gmplib.org): Required if configured with ENABLE_BIG_INT=gmp

## Build

RATJS use GNU make to build the source code.
The following libraries and headers are required for building:

* [c-json](https://github.com/json-c/json-c/wiki)
* [libyaml](http://pyyaml.org/wiki/LibYAML): Required if you want to build test262 program

### Options

Show all the configuration options:
```
$ make help
```

### Build on Linux

Configure the project.
```
$ make config
```

Build the libraries and executable programs.
```
$ make
```

Install the libraries and executable programs.
```
$ make install
```

Clear intermediate files.
```
$ make clean
```

Clear the build directory.
```
$ make dist-clean
```

### Build on Windows

To build RATJS on Windows, you need setup [MinGW](https://www.mingw-w64.org) environment.

Configure the project.
```
$ make ARCH=win config
```

Build the libraries and executable programs.
```
$ make
```

Install the libraries and executable programs.
```
$ make install
```

Clear intermediate files.
```
$ make clean
```

Clear the build directory.
```
$ make dist-clean
```

## Usage

### Execute javascript

Run program "ratjs" to execute your javascript program.

To run the "js" script file:
```
$ ratjs -s your_script.js arguments...
```
"ratjs" will load and execute the script "your_script.js". If the script has defined "main" function, the function will be invoked and "arguments" will be passed to it as the arguments.


To run the "js" as ECMA262 module:
```
$ ratjs your_module.js arguments
```
"ratjs" will load, link and execute the module "your_module.js". If the module has defined "main" function, the function will be invoked and "arguments" will be passed to it as the arguments.

To read script string from argument and invoke "eval()" function to execute it:
```
$ ratjs -e "script_string"
```

To show options of the program "ratjs":
```
$ ratjs --help
```


### Embed

In your source code, include header file "ratjs.h". Then invoke RATJS APIs to load and execute the script.

```
#include <ratjs.h>

...

RJS_Runtime *rt;
RJS_Value   *source, *script;

rt = rjs_runtime_new();
rjs_realm_load_extension(rt, NULL);

source = rjs_value_stack_push(rt);
script = rjs_value_stack_push(rt);

rjs_string_from_enc_chars(rt, source, "print(\"hello, world!\")", -1, NULL);
rjs_script_from_string(rt, script, source, NULL, RJS_FALSE);
rjs_script_evaluation(rt, script, NULL);

rjs_runtime_free(rt);
```

Link your program with library "libratjs".
```
$ gcc -o your_program -lratjs -lm your_program_source.c
```
Run command "doxygen" to generate API document of RATJS.

You can refer to the relevant programs in the "demo" directory to study how to embed RATJS to your program.

### Native module

You can use other compiler language to develop the native module to extend the javascript's function. Please refer to [njsgen-native module generator](doc/njsgen.md) to quickly build native modules.

## Extension

[Extension functions and objects](doc/extension.md)

## License

This project is licensed under the terms of the [MIT license](LICENSE).
