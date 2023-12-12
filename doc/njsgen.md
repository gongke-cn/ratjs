# njsgen - Rat Javascript Native Module Generation Tool

## Native modules

Rat Javascript supports standard ECMAScript modules. To improve the efficiency of javascript, Rat Javascript supports the development of native modules in other compiled languages to extend and enhance the efficiency of javascript.

The native module suffix is ".njs", and its file format is dynamic link library. Ratjs loads the module and calls the functions through "dlopen/dlsym". In the native module, the following two function implementations are required:

```
RJS_Result ratjs_module_init (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
    ...
}

RJS_Result ratjs_module_exec (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
    ...
}
```
The function "ratjs_module_init" is called during the module initialization phase, and the main task of this function is to register import and export symbol tables with the module. The function "ratjs_module_exec" is called during the module execution phase, and the main task of this function is to create and initialize objects corresponding to exported symbols in the module. You can refer to the [Native Module Reference Code](../demo/native_module/native_module.c) to learn how to write native modules in C.

## Compiling and Generating

Because native modules are dynamic link libraries, they can be compiled and generated using the dynamic link library method. The following is the command to compile and generate native modules using gcc on the Linux platform.

```
$gcc -o my_module.njs my_module -shared -lratjs
```

## Introduction to njsgen

njsgen is a program written in javascript that helps you quickly generate javascript native modules for existing C language development libraries.

For example, if you have a library developed in C and you want to call functions in the library through javascript, njsgen can quickly generate C source files for the native module.

The specific steps are as follows:

1. Create an interface description JSON file
2. Run njsgen to scan the interface description file to generate the corresponding native module source file
3. Run the C compiler to compile the native module source file into a native module

## njsgen command and parameters

The njsgen command has the following parameters:

```
Usage: njsgen [OPTION]...FILE
Options:
  -o STRING         set the output filename
  -v                validate the input JSON
  --help            show this help message
```

If you use njsgen to scan the description file "my_module.json" to generate the native module source file "my_module.c", the command is as follows:

```
$njsgen -o my_module.c my_module.json
```
If the output source file name is not specified through the '-o' parameter, such as the description file being 'XXX.json', njsgen will use 'XXX.c' as the output file name.

If the runtime is passed the parameter "-v", njsgen will validate the format of the description file through JSON schema. If the file format is incorrect, njsgen will report an error and exit.

## njsgen description file

The njsgen description file is in JSON format, and its JSON schema description is shown in [njsgen json schema](../module/njsgen/njs-schema.json).

The following will detail how to add common C language definitions to the description file.

### Data Types

Many properties in the description file define C language data types.
Currently, njsgen recognizes the following basic C language data types:

+ char
+ short
+ int
+ long
+ long long
+ unsigned char
+ unsigned short
+ unsigned int
+ unsigned long
+ unisnged long long
+ int8_t
+ int16_t
+ int32_t
+ int64_t
+ uint8_t
+ uint16_t
+ uint32_t
+ uint64_t
+ size_t
+ ssize_t
+ float
+ double

#### Array Types

Adding "[...]" after a data type indicates an array type. An expression can be added between the brackets to indicate the length of the array.

For example:
```
int[]
char[256]
double[MAX_ARRAY_LENGTH]
```
#### Pointer Type

Adding "*" after a data type indicates a pointer type.

For example:
```
char*
int**
double*
```

#### Enumeration, Data Structure, and Union

The enumerations, data structures, and unions defined in the description file can also be used as data type names.

For example:

```
{
    "enumerations": {
        "MyEnum": [
            "MY_ENUM_VALUE_1",
            "MY_ENUM_VALUE_2",
            "MY_ENUM_VALUE_3"
        ]
    },
    "structures": {
        "MyStruct": {
            "members": {
                "a": "MyEnum",
                "b": "int"
            }
        }
    },
    "unions": {
        "MyUnion": {
            "members": {
                "s": "MyStruct",
                "i": "int"
            }
        }
    }
}
```

### Numeric type macros

In C language, we often use "#define" to define some numeric type macros. If you want to use these definitions in javascript, you can add the "numberMacros" property description in the description file.

As defined in C language:

```
#define MY_INTEGER 19491001
#define MY_FLOAT   1.234567
```
Define in the corresponding description file "my_module":

```
{
    "numberMacros": [
        "MY_INTEGER",
        "MY_FLOAT"
    ]
}
```
njsgen generates an export item of type number for each macro of type number.

After generating the native module, it can be called in the following ways:

```
import {MY_INTEGER, MY_FLOAT} from "my_module";

print(`${MY_INTEGER} ${MY_FLOAT}\n`);
```

### Enumeration

Enumerations in C can be added to the native module by adding the "enumerations" property in the description file.

As defined in C language:

```
typedef enum {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
} Color;

typedef enum {
    MALE,
    FEMALE
} Gender;
```
Define in the corresponding description file "my_module":

```
{
    "enumerations": {
        "Color": [
            "COLOR_RED",
            "COLOR_GREEN",
            "COLOR_BLUE"
        ],
        "Gender": [
            "MALE",
            "FEMALE"
        ]
    }
}
```
njsgen generates a corresponding number type export item for each value of the enumeration.

After generating the native module, it can be called in the following ways:

```
import {COLOR_RED, COLOR_GREEN, COLOR_BLUE, MALE, FEMALE} from "my_module";

let color = COLOR_RED;
let gender = MALE;
print(`color:${color} gender:${gender}\n`);
```

### Structure

The data structures defined in C using "struct" can be added to the native module by adding the "structures" property to the description file.

As defined in C language:

```
typedef struct {
    char name[64];
    int age;
    Gender gender;
} User;
```
Define in the corresponding description file "my_module":

```
{
    "structures": {
        "User": {
            "members": {
                "name": "char[64]",
                "age": "int",
                "gender": "Gender"
            }
        }
    }
}
```
After generating the native module, it can be called in the following ways:

```
import {User} from "my_module";

let user = new User();
user.name = "Monkey King";
user.age = 500;
user.gender = MALE;
print(`${JSON.stringify(user)}\n`);
```

njsgen generates the corresponding prototype object and constructor function for the structure "User". By calling the constructor function, a new data structure object is allocated, and reading and writing the object's properties correspond to operating on the corresponding data structure members.

In the constructor, you can quickly set the members of the structure by passing in an Object parameter:

```
let user = new User({name: "Monkey King", age: 500, gender: MALE});
```
If the first parameter of the constructor is passed a number, an array with a length equal to this number will be created:

```
let users = new User(2);
users[0] = {name: "Zhuge Liang", age: 50, gender: MALE};
users[1] = {name: "Sima Yi", age: 50, gender: FEMALE};
```
The data structure buffer created by the constructor will be released when the object is garbage collected. If you do not want the data structure buffer to be managed by the GC, you can pass a false value at the end of the constructor to indicate that an external buffer is being created, and the buffer will not be automatically released.

```
let externUser = new User(false); //Create an external user object buffer.
```
Sometimes we want to create a corresponding object through a specific C function instead of using the automatically generated method of njsgen. We can add the "noConstructor" property to the data structure definition in the description file.

```
{
    "structures": {
        "User": {
            "noConstructor": true,
            "members": {
                "name": "char[64]",
                "age": "int",
                "gender": "Gender"
            }
        }
    }
}
```
In this way, there is no constructor "User" in the generated module.

Sometimes in C, we define data structures without using a "typedef" to define the corresponding type name.  Instead, we need to reference the structure using "struct STRUCTNAME".  In this case, we need to add the "noTypeDef" property to the data structure definition in the description file.

For example, in C language, the definition is:

```
struct MyStruct {
    int a;
    int b;
};
```
The description file needs to be defined as follows:

```
{
    "structures": {
        "MyStruct": {
            "noTypeDef": true,
            "members": {
                "a": "int",
                "b": "int"
            }
        }
    }
}
```
njsgen will define getter and setter methods for each member of the data structure, which are used to read and write member content respectively. If we only want to read some members and cannot modify them, we can declare the "readonly" property for the members.

```
{
    "structures": {
        "MyStruct": {
            "members": {
                "a": "int",
                "b": {
                    "type": "int",
                    "readonly": true
                }
            }
        }
    }
}
```
The "b" property of "MyStruct" in the generated module only has getter methods. Attempting to modify the "b" property will throw an exception.

If all members in the data structure are "readonly", you can directly declare the "readonly" property on the structure.

```
{
    "structures": {
        "MyStruct": {
            "readonly": true,
            "members": {
                "a": "int",
                "b": "int"
            }
        }
    }
}
```
The attributes "a" and "b" of the "MyStruct" in the generated module are both readonly.

### Union

The union defined in C language can be added to the native module by adding the "unions" property in the description file.

As defined in C language:

```
typedef union {
    uint8_t  u8;
    uint16_t u16;
    uint32_t u32;
} MyUnion;
```
Define in the corresponding description file "my_module":

```
{
    "unions": {
        "MyUnion": {
            "members": {
                "u8": "uint8_t",
                "u16": "uint16_t",
                "u32": "uint32_t"
            }
        }
    }
}
```
After generating the native module, it can be called in the following ways:

```
import {MyUnion} from "my_module";

let u = new MyUnion();
u.u32 = 0x12345678;
print(`${JSON.stringify(u)}\n`);
```
Unions are similar to structures, and can be initialized quickly using an object as constructor parameter. They can also be created by passing numerical parameter, or by passing false in the constructor to indicate that the allocated buffer will not be automatically released during garbage collection of the object. Similarly, "union" also supports the "noConstructor", "noTypeDef", and "readonly" properties.

### Variables

Variables in C can be added to the module by adding the "variables" property in the description file.

Define the following variables in C language:

```
int my_int_var;
User my_user_info;
```
Define in the description file:
```
{
    "variables": {
        "my_int_var": "int",
        "my_user_info": "User"
    }
}
```
In javascript, it can be called through the following methods:
```
import {my_int_var, my_user_info} from "my_module";

$.my_int_var = 1;
$.my_user_info.name = "Liu Bei";
print(`${$.my_int_var} ${JSON.stringify(@my_user_info)}`);
```
You can see that njsgen generates an object named "$". njsgen generates accessor properties for each variable, and the accessor getter and setter methods correspond to the variable's read and write operations.

If a variable is constant, you can define its "readonly" property as true, and njsgen will only generate getter method for this variable's accessor.

As defined in C language:

```
const int my_const = 19491009;
```
Define in the description file:

```
{
    "variables": {
        "my_const": {
            "type": "int",
            "disabled" : true
        }
    }
}
```

### Function

Functions defined in C can be added to modules by adding the "functions" property in the description file.

As defined in C language:

```
int my_func (int a)
{
    ...
}
```
Define in the corresponding description file "my_module":

```
{
    "functions": {
        "my_func": {
            "parameters": {
                "a": "int"
            },
            "return": "int"
        }
    }
}
```
After generating the native module, it can be called in the following ways:

```
import {my_func} from "my_module";

print(my_func(1949),"\n");
```
Each function in the description file defines two properties: "parameters" and "return", which describe the function's parameters and return value, respectively. If the function parameters are empty, the "parameters" property can be omitted. If the return value is "void", the "return" property can be omitted.

Some functions take arrays of numeric types as parameters, such as:

```
void fill_buf_256 (uint8_t buf[256])
{
    ...
}
```
Define in the corresponding description file "my_module":

```
{
    "functions": {
        "fill_buf_256": {
            "parameters": {
                "buf": "uint8_t[256]"
            }
        }
    }
}
```
In the generation module, we can create a TypedArray as an argument.

```
import {fill_buf_256} from "my_module";

let buf = new Uint8Array(256);
fill_buf_256(buf);
```

Note that the type of TypedArray should match the data type in C, and the length should be greater than or equal to the length declared in C. Otherwise, an exception will be thrown during runtime.

Sometimes in C, it is desirable to pass the length of an array as a parameter.

For example:

```
void fill_buf (uint8_t *buf, int len)
{
    ...
}
```
At this time, we describe the file definition as follows:

```
{
    "functions": {
        "fill_buf": {
            "parameters": {
                "buf": {
                    "type": "uint8_t*",
                    "length": "len"
                },
                "len": "int"
            }
        }
    }
}
```
The property "length" definition is added to the parameter "buf", and its value is an expression representing the length of the array, which can contain other parameters.

In the generation module, we call it in the following way:
```
import {fill_buf} from "my_module";

let buf = new Uint8Array(256);
fill_buf(buf, 256);
```
Sometimes in C language, pointers are passed through parameters, and some values are output through these pointers in functions.  We call these parameters output parameters. If a parameter pointer is used to pass data and return data, we call these parameters input and output parameters.

For example, the C language function definition:

```
int my_func (int pin, int *pout, int *pinout)
{
    int r = pin + *pinout;

    *pout = pin;
    *pinout = -pin;
    return r;
}
```
Where "pin" is the input parameter, "pout" is the output parameter, and "pinout" is the input and output parameter.

The corresponding description file definition is:

```
{
    "functions": {
        "my_func": {
            "parameters": {
                "pin": {
                    "type": "int",
                    "direction": "in"
                },
                "pout": {
                    "type": "int*",
                    "direction": "out"
                },
                "pinout": {
                    "type": "int*",
                    "direction": "inout"
                },
            },
            "return": "int"
        }
    }
}
```
We have added the "direction" property to parameter definitions to indicate the input and output characteristics of the parameter. If a parameter does not have a defined "direction" property, it indicates that it is an input parameter.

In JavaScript, we call it in the following way:

```
import {my_func} from "my_module";

print(JSON.stringify(my_func(123, 321), null, "  "));
```
The print result is:

```
{
  "return": 444,
  "pout": 123,
  "pinout": -123
}
```
As you can see, in JavaScript function calls, only input parameters and input/output parameters need to be passed in. The return value is an object that includes the property "return" corresponding to the C function return value, as well as the return values of all output parameters and input/output parameters.

Note that in C, sometimes an array or buffer is used as a parameter to store output data. In this case, njsgen will not place it in the properties of the return object, and you still need to allocate the object before calling the function and pass it as a parameter to the function.

For example, the following function:

```
void get_data (uint8_t *buf, int len)
{
    buf[0] = 0x47;
    buf[1] = 0x00;
    ...
}

void get_user_info (User *u)
{
    printf(u->name, sizeof(u->name), "%s", "Guan Yu");
    u->age = 1000;
    u->gender = MALE;
}
```
The corresponding description file is defined as:

```
{
    "functions": {
        "get_data": {
            "parameters": {
                "buf": {
                    "type": "uint8_t*",
                    "direction": "out",
                    "length": "len"
                },
                "len": "int"
            }
        },
        "get_user_info": {
            "parameters": {
                "buf": {
                    "type": "User*",
                    "direction": "out"
                }
            }
        }
    }
}
```
Call methods in javascript as follows:

```
import {get_path, get_user_info, User} from "my_module";

let buf = new Uint8Array(256);
get_data(buf, 256);

let user = new User();
get_user_info(user);
```

### Calling C and javascript functions

In order to allow C and javascript functions to call each other, the "functionTypes" property needs to be defined in the description file.

For example, the C language defines function types as follows:

```
typedef int (*MyFunc) (int a);
```
Add definitions to the description file:

```
{
    "functionTypes": {
        "MyFunc": {
            "parameters": "int"
        },
        "return": "int"
    }
}
```
If the following functions in C use "MyFunc" as an argument:
```
void func_with_func_parm(MyFunc cb)
{
    int i;
    for (i = 0; i < 100; i++)
        printf("%d ", cb(i));
}
```
Define this function in the description file as:

```
{
    ...
    "functions": {
        "func_with_func_parm": {
            "parameters": {
                "cb": "MyFunc"
            }
        }
    }
}
```
In JavaScript, we can directly pass in a JavaScript function, which will be converted into a C function and called for execution:

```
import {func_with_func_parm} from  "my_module";

func_with_func_parm(n => n * n);
```
If C defines a variable of function pointer type:

```
MyFunc my_func_var;
```
Define this variable in the description file as:

```
{
    ...
    "variables": {
        "my_func_var": "MyFunc"
    }
}
```
In javascript, you can also directly call this variable:

```
import {my_func_var} from  "my_module";

print(my_func_var(100));
```
### User Code Slot

njsgen generates the source file of the native module. Sometimes we want to add our own code to the source file. njsgen adds several user code slots to the source file, where users can add their own code. This way, when the description file is updated and njsgen needs to be run again to update the native module source file, njsgen will automatically merge the custom code previously added by the user into the newly generated native module source file.

The user code slot is formatted as follows in the source file:

```
/*NJSGEN XXXXXXXX BEGIN*/
/*NJSGEN TODO*/
...
/*NJSGEN XXXXXXXX END*/
```
As you can see, the user code slot is composed of multiple lines of code surrounded by a start line marker of "/\*NJSGEN XXXXXXX BEGIN\*/" and an end line marker of "/\*NJSGEN XXXXXXX END\*/". Users can add their own code to it. Where "XXXXXXX" is the code block tag tag, which is unique in the source file.

Currently, the slot tags generated by njsgen include the following:

+ INCLUDE_HEAD: At the beginning of the "#include" block
+ INCLUDE_TAIL: At the end of the "#include" block
+ DECL: variable and type declaration code block
+ EXPORT_TABLE_DECL: import and export symbol table declaration
+ MODULE_DATA_DECL: Module private data declaration
+ MODULE_DATA_SCAN: Module private data GC scan function
+ MODULE_DATA_FREE: Module private data GC free function
+ FUNC_HEAD: the beginning of the function definition code block
+ FUNC_TAIL: The end of the function definition code block
+ MODULE_INIT: Module initialization function
+ MODULE_EXEC: Module execution function

For example, you can add the required header files in the "INCLUDE_TAIL" block.

```
/*NJSGEN INCLUDE_HEAD BEGIN*/
/*NJSGEN TODO*/
/*NJSGEN INCLUDE_HEAD END*/
#include <string.h>
#include <stdlib.h>
#include <ratjs.h>
/*NJSGEN INCLUDE_TAIL BEGIN*/
#include <SDL.h>
/*NJSGEN INCLUDE_TAIL END*/
```
