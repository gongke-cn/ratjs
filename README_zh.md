# Rat Javascript - 小型javascript/ecmascript解释器

[English](README.md)

RATJS是一个用C语言实现的小型javascript/ecmascript解释器。你可以用它来运行自己的javascript程序，也可以将它作为脚本引擎嵌入自己开发的程序中。

## 特性

* 兼容ECMA262标准第14版
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
* 扩展
	+ Hash bang 注释
	+ Native module
	+ JSON module
	+ File system functions
	+ FileState object
	+ File object
	+ Directory object
* 完全用C语言实现
* 低占用空间
* 丰富的配置选项

## 依赖

* [icu4c](http://icu-project.org/download/latest_milestone.html): 当配置选项设置 ENC_CONV=icu 后需要此库
* [gmp](http://gmplib.org): 当配置选项设置 ENABLE_BIG_INT=gmp 后需要此库

## 构建

RATJS 使用 GNU make 作为代码构建工具。
在构建时需要安装以下的库和对应的头文件：

* [c-json](https://github.com/json-c/json-c/wiki)
* [libyaml](http://pyyaml.org/wiki/LibYAML): 如果需要生成 test262 测试程序需要此库

### 配置选项

显示全部配置选项：
```
$ make help
```

### 在Linux系统中构建

项目配置。
```
$ make config
```

生成RATJS链接库和可执行程序。
```
$ make
```

安装RATJS连接库和可执行程序。
```
$ make install
```

清除构建中间文件。
```
$ make clean
```

清除构建目录。
```
$ make dist-clean
```

### 在Windows系统中构建
在Windows系统中构建需要安装[MinGW](https://www.mingw-w64.org)开发环境。

项目配置。
```
$ make ARCH=win config
```

生成RATJS链接库和可执行程序。
```
$ make
```

安装RATJS连接库和可执行程序。
```
$ make install
```

清除构建中间文件。
```
$ make clean
```

清除构建目录。
```
$ make dist-clean
```

## 使用

### 执行javascript

运行可执行程序"ratjs"解释你的javascript脚本。

运行 “js”脚本：
```
$ ratjs -s your_script.js arguments...
```
"ratjs"会加载并运行脚本"your_script.js"。如果脚本中定义了函数"main"，"main"函数将被调用且"arguments"将作为函数的参数被传入。

将"js"脚本作为ECMA262模块运行：
```
$ ratjs your_module.js arguments
```
"ratjs"将加载、链接并执行模块"your_module.js"。如果模块中定义了函数"main"，"main"函数将被调用且"arguments"将作为函数的参数被传入。

将参数字符串作为脚本源码调用"eval()"运行:
```
$ ratjs -e "script_string"
```

显示可执行程序"ratjs"的全部选项:
```
$ ratjs --help
```

### 嵌入程序

首先，在你的程序中包含头文件"ratjs.h"。然后通过调用 RATJS API加载并执行javascript脚本。

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

将你的程序连接库"libratjs"。
```
$ gcc -o your_program -lratjs -lm your_program_source.c
```

运行命令"doxygen"生成RATJS的API文档。

你可以参考"demo"目录下的示例程序学习如何将RATJS嵌入你的程序中。

### 原生模块

你可以用其他编译语言开发原生模块扩展javascript的功能，可参考[njsgen原生模块生成工具](doc/njsgen_zh.md)快速创建原生模块。

## 扩展

[扩展函数和对象](doc/extension_zh.md)

## 许可证

RATJS采用许可证[MIT license](LICENSE)发布。
