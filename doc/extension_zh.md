# Rat Javascript 扩展

## 函数

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

### solveJobs ()

+ 描述:
    处理异步事务。可以调用此函数等待Promise对象的回调执行完成。
+ 示例：
```
/*同步import操作。*/
function syncImport(name)
{
    let ns, err;

    /*import 返回一个Promise。*/
    import(name).then(r => {
        ns = r;
    }, e => {
        err = e;
    });

    /*等待import执行完成。*/
    while ((ns == undefined) && (err == undefined))
        solveJobs();

    /*失败throw error。*/
    if (err != undefined)
        throw err;

    /*成功返回module的namespace对象。*/
    return ns;
}
```

### scriptPath

+ 描述:
    返回调用函数的script或模块文件的路径。
+ 返回值:

    |类型|描述|
    |---|---|
    |string|script或模块文件的路径|

### modules

+ 描述:
    返回所有加载的模块文件的路径。
+ 返回值:

    |类型|描述|
    |---|---|
    |string[]|所有加载的模块文件的路径|

### encodeText

+ 描述:
    将字符串通过编码输出到ArrayBuffer。
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |str|string|要编码的字符串|
    |enc|string|字符编码，缺省为"UTF-8"|
    |buf|ArrayBuffer|存放输出编码字符的缓冲区，如果没有设定buf,函数创建一个新缓冲区|
    |start|number|开始存放位置偏移，缺省为0|

+ 返回值:

    |类型|描述|
    |---|---|
    |ArrayBuffer|如果输入参数没有指定buf,函数创建一个新缓存区存放编码字符|
    |number|如果输入参数指定buf,返回写入的字符长度|

### decodeText

+ 描述:
    解码ArrayBuffer中的字符。
+ 参数:

    |名称|类型|描述|
    |---|---|---|
    |enc|string|字符编码，缺省为"UTF-8"|
    |buf|ArrayBuffer|存放输入编码字符的缓冲区|
    |start|number|字符存放的开始存放，缺省为0|
    |end|number|字符存放的结束位置，缺省为缓冲区长度|

+ 返回值:

    |类型|描述|
    |---|---|
    |string|返回解码后的字符串|

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