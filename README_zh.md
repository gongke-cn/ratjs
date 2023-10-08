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

## 扩展

### 函数

### print (...)
+ 描述:
    将参数转化为字符串输出到标准输出。

### prerr (...)
+ 描述:
    将参数转化为字符串输出到标准错误输出。

### dirname (path)

+ 描述:
    获取路径对应的目录名
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |path|string|路径名|

+ 返回值:

    |类型|描述|
    |---|---|
    |string|path对应的目录名|

### basename (path)

+ 描述:
    获取路径去除目录后的基本名
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |path|string|路径名|

+ 返回值:

    |类型|描述|
    |---|---|
    |string|path去除目录后的基本名|

### realpath (path)

+ 描述:
    获取绝对路径名
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |path|string|路径名|

+ 返回值:

    |类型|描述|
    |---|---|
    |string|path对应的绝对路径名|
    |undefined|path对应的文件不存在|

### rename (old, new)

+ 描述:
    文件重命名
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |old|string|旧文件名|
    |new|string|新文件名|

### unlink (filename)

+ 描述:
    删除文件
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |filename|string|文件名|

### rmdir (dirname)

+ 描述:
    删除路径
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |dirname|string|路径名|

### mkdir (dirname[, mode])

+ 描述:
    创建路径
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |dirname|string|路径名|
    |mode|number|路径访问权限|

### chmod (filename, mode)

+ 描述:
    修改文件的访问权限
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |filename|string|文件名|
    |mode|number|新的访问权限|

### getenv (name)

+ 描述:
    获取环境变量
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |name|string|环境变量名|

+ 返回值:

    |类型|描述|
    |---|---|
    |string|环境变量值|
    |undefined|环境变量未定义|

### system (cmd)

+ 描述:
    运行系统shell命令
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |cmd|string|shell命令|

+ 返回值:

    |类型|描述|
    |---|---|
    |number|shell命令返回值|

### getcwd ()

+ 描述:
    获取当前工作目录名

+ 返回值:

    |类型|描述|
    |---|---|
    |string|当前工作目录名|

## 对象

### FileState对象

+ 描述:
    文件属性

+ 构造函数 FileState (filename)
    + 描述：
        创建一个文件对应的描述对象，对象包括以下属性:

        |名称|类型|描述|
        |---|---|---|
        |size|number|文件大小，单位为字节|
        |format|number|文件类型, 取值为：FileState.FORMAT_XXXX|
        |mode|number|文件访问权限|
        |atime|number|文件最后一次被访问的时间，单位为秒，数值为1970-1-1开始的秒数|
        |mtime|number|文件最后一次被修改的时间，单位为秒，数值为1970-1-1开始的秒数|
        |ctime|number|文件最后一次状态被修改的时间，单位为秒，数值为1970-1-1开始的秒数|

    + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|

    + 返回值:

        |类型|描述|
        |---|---|
        |object|文件状态描述对象|
        |undefined|文件不存在|

+ FileState 属性:

    |名称|类型|描述|
    |---|---|---|
    |FORMAT_REGULAR|number|普通文件类型|
    |FORMAT_DIR|number|路径文件类型|
    |FORMAT_CHAR|number|字符设备文件类型|
    |FORMAT_BLOCK|number|块设备文件类型|
    |FORMAT_SOCKET|number|套接字文件类型|
    |FORMAT_FIFO|number|FIFO文件类型|
    |FORMAT_LINK|number|链接文件类型|

### File对象

+ 描述:
    文件

+ 构造函数 File (filename, mode)
    + 描述：
        打开文件

    + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|
        |mode|string|文件打开模式|

        文件打开模式:

        |模式|描述|
        |---|---|
        |r  |读模式打开，当前位置定位在文件开头|
        |r+ |读写模式打开，当前位置定位在文件开头|
        |w  |文件存在则将文将长度清0，否则创建新文件，以写模式打开，当前位置定位在文件开头|
        |w+ |文件存在则将文将长度清0，否则创建新文件，以读写模式打开，当前位置定位在文件开头|
        |a  |文件不存在时创建新文件，以写模式打开，当前位置定位在文件结尾|
        |a+ |文件不存在时创建新文件，以读写模式打开，当前位置定位在文件结尾|
        |b  |二进制模式，b标志可以放在其他标志末尾|

    + 返回值:

        |类型|描述|
        |---|---|
        |object|新创建文件对象|

+ File 属性:
    + fields

        |名称|类型|描述|
        |---|---|---|
        |SEEK_SET|number|文件开始位置|
        |SEEK_END|number|文件结尾位置|
        |SEEK_CUR|number|文件当前位置|

    + loadString (filename[, enc])
        + 描述:
        加载文件内容并转化为字符串

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|
        |enc|string|文件的字符编码，缺省为"UTF-8"|

        + 返回值:

        |类型|描述|
        |---|---|
        |string|加载文件内容的字符串|

    + storeString (filename, str[, enc])
        + 描述:
        将字符串内容保存为文件

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|
        |str|string|要保存的字符串|
        |enc|string|文件的字符编码，缺省为"UTF-8"|

    + appendString (filename, str[, enc])
        + 描述:
        将字符串内容保存到文件末尾

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|
        |str|string|要保存的字符串|
        |enc|string|文件的字符编码，缺省为"UTF-8"|

    + loadData (filename)
        + 描述:
        加载文件内容并保存到一个缓冲区

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|

        + 返回值:

        |类型|描述|
        |---|---|
        |ArrayBuffer|加载文件内容的缓冲区|

    + storeData (filename, buf[, start, count])
        + 描述:
        将一个缓冲区内容保存为文件

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|
        |buf|ArrayBuffer|数据缓冲区|
        |start|number|开始保存数据的位置，缺省值为0|
        |count|number|要保存的数据长度，缺省值为缓冲区长度 - start|

    + appendData (filename, buf[, start, count])
        + 描述:
        将一个缓冲区内容保存到一个文件末尾

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |filename|string|文件名|
        |buf|ArrayBuffer|数据缓冲区|
        |start|number|开始保存数据的位置，缺省值为0|
        |count|number|要保存的数据长度，缺省值为缓冲区长度 - start|

+ File.prototype 属性:

    + read (buf[, start, count])
        + 描述:
        从文件读取数据到缓冲区

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |buf|ArrayBuffer|存储数据的缓冲区|
        |start|number|保存数据在存储数据缓冲区中的开始位置。缺省值为0|
        |count|number|要读取的数据长度，字节为单位。缺省值为缓冲区长度 - start|

        + 返回值:

        |类型|描述|
        |---|---|
        |number|实际读取的数据长度|

    + write (buf[, start, count])
        + 描述:
        将缓冲区数据写入文件

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |buf|ArrayBuffer|保存写入数据的缓冲区|
        |start|number|写入数据的开始位置。缺省值为0|
        |count|number|要写入的数据长度，字节为单位。缺省值为缓冲区长度 - start|

        + 返回值:

        |类型|描述|
        |---|---|
        |number|实际写入的数据长度|

    + seek (offset, whence)
        + 描述:
        修改文件读写位置

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |offset|number|以whence为参考的位置偏移，单位字节|
        |whence|number|文件基础位置，取值为File.SEEK_XXX|

        + 返回值:

        |类型|描述|
        |---|---|
        |object|this 文件对象|

    + tell ()
        + 描述:
        查询以文件开头为基准的当前位置偏移。

        + 返回值:

        |类型|描述|
        |---|---|
        |number|以文件开头为基准的位置偏移|

    + close ()
        + 描述
        关闭文件

    + getChar ()
        + 描述
        从文件中读取一字节

        + 返回值:

        |类型|描述|
        |---|---|
        |number|返回读取的字符,-1表示到达文件末尾|

    + putChar (c)
        + 描述
        向文件写入一字节

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |c|number|要写入的字符|

        + 返回值:

        |类型|描述|
        |---|---|
        |object|this文件对象|

    + getString ([enc])
        + 描述
        从文件读取数据直到文件末尾或'\n',返回读到的字符串。

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |enc|string|文件的字符编码，缺省为"UTF-8"|

        + 返回值:

        |类型|描述|
        |---|---|
        |string|返回读取到的字符串,-1表示到达文件末尾|

    + putString (str[, enc])
        + 描述
        向文件写入一个字符串

        + 参数:

        |名称|类型|描述|
        |---|---|---|
        |str|string|要写入的字符串|
        |enc|string|文件的字符编码，缺省为"UTF-8"|

        + 返回值:

        |类型|描述|
        |---|---|
        |object|this文件对象|

### Dir对象

+ 描述:
    路径。

+ 构造函数 Dir (dirname)
    + 描述：
        打开一个路径

    + 参数:

        |名称|类型|描述|
        |---|---|---|
        |dirname|string|路径名|

    + 返回值:

        |类型|描述|
        |---|---|
        |object|新创建路径对象|

+ Dir.prototype 属性:
    Dir.prototype的 [Prototype] 为 %IteratorPrototype%, 可以通过Iterator相关操作遍历路径文件中的所有文件名。

    + read ()
        + 描述
        从路径中读取一项

        + 返回值:

        |类型|描述|
        |---|---|
        |string|当前项目的文件名|
        |undefined|已到达路径文件末尾|

    + close ()
        + 描述
        关闭路径文件

    + next ()
        + 返回下一个IteratorRecord记录。

    + return ()
        + 描述
        关闭路径文件

## 许可证

RATJS采用许可证[MIT license](LICENSE)发布。
