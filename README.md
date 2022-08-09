# RatJs - an embedded JavaScript engine

RatJs is a small JavaScript engine written in C. You can use it to run JavaScript programs, or embed it into your program as a script engine

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

* icu4c (http://icu-project.org/download/latest_milestone.html): Required if configured with ENC_CONV=icu
* gmp (http://gmplib.org): Required if configured with ENABLE_BIG_INT=1

## Build

RatJs use GNU make to build the source code.
The following libraries and headers are required for building:

* c-json (https://github.com/json-c/json-c/wiki)
* libyaml (http://pyyaml.org/wiki/LibYAML): Required if you want to build test262 program

### Options

Show all the configuration options:
```
make help
```

### Build on Linux

Configure the project.
```
make config
```

Build the libraries and executable programs.
```
make
```

Install the libraries and executable programs.
```
make install
```

### Build on Windows

To build RatJs on windows, you need setup MinGW environment.
(https://www.mingw-w64.org)

Configure the project.
```
make ARCH=win config
```

Build the libraries and executable programs.
```
make
```

## Usage

## 

## License

This project is licensed under the terms of the MIT license.