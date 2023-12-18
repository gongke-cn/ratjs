# Extension of Rat Javascript

## Functions

### print (...)
+ description:
    Output arguments to the standard output.

### prerr (...)
+ description:
    Output arguments to the standard error output.

### dirname (path)

+ description:
    Get the directory name from the pathname.
+ arguments:

    |name|type|description|
    |---|---|---|
    |path|string|Pathname.|

+ return value:

    |type|description|
    |---|---|
    |string|The directory name of the path|

### basename (path)

+ description:
    Get the base name without the directory.
+ arguments:

    |name|type|description|
    |---|---|---|
    |path|string|Pathname.|

+ return value:

    |type|description|
    |---|---|
    |string|The base name without the directory.|

### realpath (path)

+ description:
    Get the absolute pathname.
+ arguments:

    |name|type|description|
    |---|---|---|
    |path|string|Pathname.|

+ return value:

    |type|description|
    |---|---|
    |string|The absolute pathname.|
    |undefined|The file does not exist.|

### rename (old, new)

+ description:
    Rename the file.
+ arguments:

    |name|type|description|
    |---|---|---|
    |old|string|The old filename.|
    |new|string|The new filename.|

### unlink (filename)

+ description:
    Remove the file.
+ arguments:

    |name|type|description|
    |---|---|---|
    |filename|string|The file to be removed.|

### rmdir (dirname)

+ description:
    Remove the directory.
+ arguments:

    |name|type|description|
    |---|---|---|
    |dirname|string|The directory to be removed.|

### mkdir (dirname[, mode])

+ description:
    Create a new directory.
+ arguments:

    |name|type|description|
    |---|---|---|
    |dirname|string|The directory's name.|
    |mode|number|The directory's access mode.|

### chmod (filename, mode)

+ description:
    Change the file's access mode.
+ arguments:

    |name|type|description|
    |---|---|---|
    |filename|string|The filename.|
    |mode|number|New access mode.|

### getenv (name)

+ description:
    Get the value of the environment variable.
+ arguments:

    |name|type|description|
    |---|---|---|
    |name|string|The environment variable's name.|

+ return value:

    |type|description|
    |---|---|
    |string|The value of the environment variable.|
    |undefined|The environment variable is not defined.|

### system (cmd)

+ description:
    Execute the shell command.
+ arguments:

    |name|type|description|
    |---|---|---|
    |cmd|string|The shell command.|

+ return value:

    |type|description|
    |---|---|
    |number|The return value of the shell command.|

### getcwd ()

+ description:
    Get the current work directory.

+ return value:

    |type|description|
    |---|---|
    |string|The name of current work directory.|

### solveJobs ()

+ description:
    Solve the asynchronous jobs. Use it to wait the callback functions of Promise objects.
+ sample：
```
/*Synchronous import.*/
function syncImport(name)
{
    let ns, err;

    /*import return a Promise object.*/
    import(name).then(r => {
        ns = r;
    }, e => {
        err = e;
    });

    /*Wait the import promise.*/
    while ((ns == undefined) && (err == undefined))
        solveJobs();

    /*Throw the error.*/
    if (err != undefined)
        throw err;

    /*Return the module's namespace.*/
    return ns;
}
```

### scriptPath

+ description:
    Return the calling function's script or module's pathname.
+ return value:

    |type|description|
    |---|---|
    |string|The calling script/module's pathname.|

### modules

+ description:
    Return all loaded modules' pathnames.
+ return value:

    |type|description|
    |---|---|
    |string[]|The string array stored all modules' pathnames.|

### encodeText

+ description:
    Encode the string and output the text to an ArrayBuffer。
+ arguments:

    |name|type|description|
    |---|---|---|
    |str|string|The input string.|
    |enc|string|The encoding type, default is "UTF-8".|
    |buf|ArrayBuffer|The ArrayBuffer store the output data. If it is not set, allocate a new buffer to store the data.|
    |start|number|The start position in the ArrayBuffer to store the output data, default is 0.|

+ return value:

    |type|description|
    |---|---|
    |ArrayBuffer|When the parameter "buf" is not set, return a new ArrayBuffer stored the encoded data.|
    |number|When the parameter "buf" is set, return the written data length.|

### decodeText

+ description:
    Decode the text data in an ArrayBuffer。
+ arguments:

    |name|type|description|
    |---|---|---|
    |enc|string|The encoding type, default is "UTF-8".|
    |buf|ArrayBuffer|The ArrayBuffer store the text data.|
    |start|number|The start position in the ArrayBuffer to store the text data, default is 0.|
    |end|number|The end position in the ArrayBuffer to store the text data, default is the length of the ArrayBuffer.|

+ return value:

    |type|description|
    |---|---|
    |string|Return the string with decoded text.|

## Objects

### FileState Object

+ description:
    File state.

+ Constructor: FileState (filename)
    + description:
        Create a new file state object. The object contains the following fields:

        |name|type|description|
        |---|---|---|
        |size|number|Size of the file in bytes.|
        |format|number|The file format, the value is FileState.FORMAT_XXXX|
        |mode|number|The file's access mode.|
        |atime|number|The last access time in seconds, from 1970-1-1.|
        |mtime|number|The last modify time in seconds, from 1970-1-1.|
        |ctime|number|The last status change time in seconds, from 1970-1-1.|

    + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|

    + return value:

        |type|description|
        |---|---|
        |object|The new file state object.|
        |undefined|The file does not exist.|

+ FileState's properties:

    |name|type|description|
    |---|---|---|
    |FORMAT_REGULAR|number|The regular file type.|
    |FORMAT_DIR|number|The directory.|
    |FORMAT_CHAR|number|The character device.|
    |FORMAT_BLOCK|number|The block device.|
    |FORMAT_SOCKET|number|The socket device.|
    |FORMAT_FIFO|number|The FIFO device.|
    |FORMAT_LINK|number|The linkage file.|

### File Object

+ description:
    The file.

+ Constructor: File (filename, mode)
    + description:
        Open the file.

    + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|
        |mode|string|The file operation mode description string.|

        Operation mode description string:

        |mode|description|
        |---|---|
        |r  |Open file for reading. The stream is positioned at the beginning of the file.|
        |r+ |Open for reading and writing. The stream is positioned at the beginning of the file.|
        |w  |Truncate file to zero length or create text file for writing. The stream is positioned at the beginning of the file.|
        |w+ |Open for reading and writing. The file is created if it does not exist, otherwise it is truncated. The stream is positioned at the beginning of the file.|
        |a  |Open for appending (writing at end of file). The file is created if it does not exist. The stream is positioned at the end of the file.|
        |a+ |Open for reading and appending (writing at end of file). The file is created if it does not exist. The stream is positioned at the end of the file.|
        |b  |In binary mode. "b" can be at the end of other mode.|

    + return value:

        |type|description|
        |---|---|
        |object|The new file object.|

+ File's properties:
    + fields

        |name|type|description|
        |---|---|---|
        |SEEK_SET|number|The beginning of the file.|
        |SEEK_END|number|The end of the file.|
        |SEEK_CUR|number|The current position.|

    + loadString (filename[, enc])
        + description:
        Load the file and convert it into string.

        + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|
        |enc|string|The file's character encoding, default value is "UTF-8".|

        + return value:

        |type|description|
        |---|---|
        |string|The string with file's data in it.|

    + storeString (filename, str[, enc])
        + description:
        Store the string to a file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|
        |str|string|Teh string to be stored.|
        |enc|string|The file's character encoding, default value is "UTF-8".|

    + appendString (filename, str[, enc])
        + description:
        Append the string's data to the end of a file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|
        |str|string|The string to be stored.|
        |enc|string|The file's character encoding, default value is "UTF-8".|

    + loadData (filename)
        + description:
        Load the file data to an array buffer.

        + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|

        + return value:

        |type|description|
        |---|---|
        |ArrayBuffer|The array buffer stored the file's data.|

    + storeData (filename, buf[, start, count])
        + description:
        Store the data in the array buffer to a file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|
        |buf|ArrayBuffer|The array buffer to be stored.|
        |start|number|Start position to be stored, default value is 0.|
        |count|number|The stored data length, default value is length - start|

    + appendData (filename, buf[, start, count])
        + description:
        Append the data in the array buffer to the end of a file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |filename|string|The filename.|
        |buf|ArrayBuffer|The array buffer to be stored.|
        |start|number|Start position to be stored, default value is 0.|
        |count|number|The stored data length, default value is buffer's length - start|

+ File.prototype's properties:

    + read (buf[, start, count])
        + description:
        Read data from a file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |buf|ArrayBuffer|The array buffer to store the read data.|
        |start|number|Start position to store the read data, default value is 0.|
        |count|number|Data length to be read, default value is buffer's length - start.|

        + return value:

        |type|description|
        |---|---|
        |number|Actual read count.|

    + write (buf[, start, count])
        + description:
        Write data to a file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |buf|ArrayBuffer|The array buffer contains data.|
        |start|number|Start position of data to be stored, default value is 0.|
        |count|number|Data length to be stored, default value is buffer's length - start.|

        + return value:

        |type|description|
        |---|---|
        |number|Actual write data count.|

    + seek (offset, whence)
        + description:
        Change the current file operation position.

        + arguments:

        |name|type|description|
        |---|---|---|
        |offset|number|Offset in bytes.|
        |whence|number|The relative postion, value is File.SEEK_XXX|

        + return value:

        |type|description|
        |---|---|
        |object|this file object.|

    + tell ()
        + description:
        Get the current operation offset from the beginning of the file.

        + return value:

        |type|description|
        |---|---|
        |number|The offset from the beginning of the file.|

    + close ()
        + description:
        Close the file.

    + getChar ()
        + description:
        Read 1 byte from the file.

        + return value:

        |type|description|
        |---|---|
        |number|Return the byte value, -1 means reach the file end.|

    + putChar (c)
        + description:
        Write 1 byte to the file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |c|number|The character value to be written.|

        + return value:

        |type|description|
        |---|---|
        |object|this file object.|

    + getString ([enc])
        + description:
        Read data from a file until the file end or '\n'. And convert the read data to string.

        + arguments:

        |name|type|description|
        |---|---|---|
        |enc|string|The file's character encoding, default value is "UTF-8".|

        + return value:

        |type|description|
        |---|---|
        |string|Return the read string, -1 means reach the file end.|

    + putString (str[, enc])
        + description:
        Write a string to the file.

        + arguments:

        |name|type|description|
        |---|---|---|
        |str|string|The string to be written.|
        |enc|string|The file's character encoding, default value is "UTF-8".|

        + return value:

        |type|description|
        |---|---|
        |object|this file object.|

### Dir Object

+ description:
    Directory.

+ Constructor: Dir (dirname)
    + description:
        Open a directory.

    + arguments:

        |name|type|description|
        |---|---|---|
        |dirname|string|The directory's name.|

    + return value:

        |type|description|
        |---|---|
        |object|The new directory object.|

+ Dir.prototype's properties:
    Dir.prototype's [Prototype] value is %IteratorPrototype%. You can use normal iterator operation to traverse the filenames in the directory.

    + read ()
        + description:
        Read a filename entry from the directory.

        + return value:

        |type|description|
        |---|---|
        |string|The current entry's filename.|
        |undefined|Reach the end of the directory.|

    + close ()
        + description:
        Close the directory.

    + next ()
        + Get the next IteratorRecord。

    + return ()
        + description:
        Close the directory.