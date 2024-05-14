# njsgen - Rat Javascript native module generation tool

## Native module

Rat Javascript supports standard ECMAScript modules. To improve the efficiency of JavaScript execution, Rat Javascript supports developing native modules using other compiled languages, extending and improving the efficiency of JavaScript execution.
The file format of the native module suffix ".njs" is a dynamic link library, and ratjs calls its functions through "dlopen/dlsym". In the native module, the following two function implementations need to be included:

```
RJS_Result
ratjs_module_init (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
}

RJS_Result
ratjs_module_exe (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
}

```

The function "ratjs_module_init" is called during the module initialization phase, and its main function is to register the imported and exported symbol tables with the module.
The function "ratjs_module_exec" is called during the module execution phase, and its main task is to create objects corresponding to exported symbols in the module.
You can refer to the [Native Module Reference Code](demo/native_module/native_module.c) to learn how to write native modules in C language.

## Compile and generate

Because native modules are dynamic link libraries, they can be compiled and generated using the method of dynamic link libraries.
The following is the command to generate native modules through GCC on the Linux platform.

```
$gcc -o my_module.njs my_module -shared -lratjs
```

## Introduction to njsgen

Njsgen is a program written in JavaScript that can help you quickly generate corresponding native JavaScript modules for existing C language development libraries.
For example, if you have a library developed in C language, you want to call functions in the library through JavaScript. Njsgen can quickly generate source files corresponding to native modules.
The specific operation steps are as follows:

1. Create an interface description JSON file
2. Run njsgen to scan the interface description file and generate the corresponding native module source file
3. Run the C compiler to compile the native module source files into native modules

## Njsgen command and parameters

The parameters of the njsgen command are as follows:

```
Usage: njsgen [Option] FILE
Options:
  -o STRING set the output file name
  -v validate the input JSON
  --help show this help message
```

Using njsgen to scan the description file "my_module.json" and generate the native module source file "my_module.c", the command is as follows:

```
$njsgen -o my_module.c my_module.json
```

If the description file is "XXX.json" and no output filename is specified by "-o" option, njsgen will use "XXX.c" as the output filename.
If the njsgen program takes the parameter "-v", njsgen will verify the format of the description file through JSON schema.
If there is an error in the file format, njsgen will report an error and exit.

## Njsgen description file

The njsgen description file is in JSON format, and its JSON schema description can be found in [njsgen JSON schema](module/njsgen/njs-schema.json).
The following will provide a detailed description of how to add common C language definitions to the description file.

### Number type macro

In C language, "#define" is often used to define macros of numerical types. If you want to use these definitions in JavaScript, you can add the "numberMacros" attribute description in the description file.
As defined in C language:

```
#define MY_INTEGER 19491001
#define MY_FLOAT 1.234567
```

Define in the corresponding description file "my_module":

```
"numberMacros":[
    "MY_INTEGER",
    "MY_FLOAT"
]
```

Njsgen generates a number type export item for each macro of this number type. After generating the native module, it can be called in the following ways:

```
import {MY_INTEGER, MY_FLOAT} from "my_module";

print (`${MY_INTEGER} ${MY_FLOAT}\n`);
```

### Enumeration

The enumeration in C language can be added to the native module by adding the "enumerations" attribute in the description file.
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
"enumerations":{
    "Color":[
        "COLOR_RED",
        "COLOR_GREEN",
        "COLOR_BLUE"
    ],
    "Gender":[
        "MALE",
        "FEMALE"
    ]
}
```

Njsgen generates a corresponding number type export item for each value in the enumeration. After generating the native module, it can be called in the following ways:

```
import {COLOR_RED, COLOR_GREEN, COLOR_BLUE, MALE, FEMALE} from "my_module";

let color=COLOR_RED;
let gender=MALE;

print(`color: ${color} gender: ${gender}\n`);
```

### Structure

The structure defined through "struct" in C language can add the "structures" attribute to the description file and add it to the native module.
As defined in C language:

```
typedef struct {
    char name [64];
    int age;
    Gender gender;
} User;
```

Define in the corresponding description file "my_module":

```
"structures": {
    "User": {
        "members":{
            "name": "char [64]",
            "age": "int",
            "gender": "Gender"
        }
    }
}
```

After generating the native module, it can be called in the following ways:

```
import {User} from "my_module";

let user=new User();
user.name="Monkey King";
user.age=500;
user.gender=MALE;

print (${JSON.stringify(user)}\n`);
```

Njsgen generates the corresponding prototype object and constructor for the structure "User". Assign a new structure object by calling the constructor, and read and write the properties of the object to the corresponding structure members based on the operation.
In the constructor, the members of the structure can be quickly set by passing in an Object:

```
let user=new User ({name: "Monkey King", age: 500, gender: MALE});
```

If the first parameter of the constructor passes in a number, create an array with a length equal to this number:

```
let users=new User (2);

users[0]={name: "Zhuge Liang", age: 50, gender: MALE};
users[1]={name: "Sima Yi", age: 50, gender: FEMALE};
```

The structure buffer created through the constructor will be released when the object is collected by GC. If you do not want the structure buffer to be managed by GC, you can pass a false at the end of the constructor to indicate that an external buffer has been created and will not be automatically released.

```
let externUser=new User(false)// Create an external user object buffer
```

Sometimes we want to create corresponding objects through specific C functions instead of using njsgen's automatically generated methods. We can add the "noConstructor" attribute in the structure definition of the description file.

```
"structures": {
    "User": {
        "noConstructor": true,
        "members":{
            "name": "char [64]",
            "age": "int",
            "gender": "Gender"
        }
    }
}
```

In this way, there will be no constructor "User" in the generated module.

Sometimes in C language, when defining a structure, we do not use "typedef" to define the corresponding type name. We need to reference this structure through "struct StructName", and in this case, we need to add the "noTypeDef" attribute in the description file structure definition.
For example, in C language, it is defined as:

```
struct MyStruct {
    int a;
    int b;
};
```

The description file needs to be defined as follows:

```
"structures": {
    "MyStruct": {
        "noTypeDef": true,
        "members":{
            "a": "int",
            "b": "int"
        }
    }
}
```

Njsgen defines corresponding getter and setter methods for each member of the structure, which are used to read and write member content, respectively. If some members we only want to read and cannot modify, we can declare the "readonly" attribute on them.

```
"structures": {
    "MyStruct": {
        "members": {
            "a": "int",
            "b":{
                "type": "int",
                "readonly": true
            }
        }
    }
}
```

The "b" attribute of "MyStruct" in the generated module only has a getter method. Attempting to modify the b attribute will throw an exception.
If all members in the structure are "readonly", the "readonly" attribute can be declared directly on the structure.

```
"structures": {
    "MyStruct": {
        "readonly": true,
        "members":{
            "a": "int",
            "b": "int"
        }
    }
}
```

The attributes "a" and "b" of "MyStruct" in the generated module are both read-only.

### Union

Unions defined through "union" in C language can add the "unions" attribute to the description file to add native modules.
As defined in C language:

```
typedef union{
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
} MyUnion;
```

Define in the corresponding description file "my_module":

```
"unions": {
    "MyUnion": {
        "members":{
            "u8": "uint8_t",
            "u16": "uint16_t",
            "u32": "uint32_t"
        }
    }
}
```

After generating the native module, it can be called in the following ways:

```
import {MyUnion} from "my_module";

let u=new MyUnion();
u.u32=0x12345678;

print (${JSON.stringify(u)}\n`);
```

Unions are similar to structures, and can be quickly initialized using objects as constructor parameters. Unions can also be created by passing in numerical parameters, or false can be passed in the constructor to indicate that allocated buffers are not automatically released when garbage collection objects occur.
Similarly, "union" also supports "noConstructor", "noTypeDef", and "readonly" attributes.

### function

The functions defined in C language can be added to the module by adding the "functions" attribute in the description file.
As defined in C language:

```
int my_func (int a)
{
}
```

Define in the corresponding description file "my_module":

```
{
    "functions":{
        "my_func":{
            "parameters":{
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

print (my_func (1949), "\n");
```

In the description file, each function is defined with two attributes: "parameters" and "return", which respectively describe the parameters and return value of the function. If the function parameter is empty, the "parameters" attribute can be omited. If the return value is "void", the "return" attribute can be omited.
Some functions have arrays of numerical types as their parameters, such as:

```
void fill_buf_256 (uint8_t buf[256])
{
}
```

Define in the corresponding description file "my_module":

```
{
    "functions":{
        "fill_buf_256":{
            "parameters":{
                "buf": "uint8_t[256]"
            }
        }
    }
}
```

In the generation module, we can create a TypedArray as a parameter passed in.

```
import {fill_buf_256} from "my_module";

let buf=new Uint8Array (256);

fill_buf_256 (buf);
```

Note that the type of TypedArray should match the data type in C language, and the length should be greater than or equal to the length declared in C language. Otherwise, an exception will be thrown during runtime.
Sometimes in C language, it is desired to pass in the length of an array as a parameter. For example:

```
void fill_buf (uint8_t *buf, int len)
{
}
```

At this point, we define the file as:

```
{
    "functions":{
        "fill_buf":{
            "parameters":{
                "buf":{
                    "type": "uint8_t*",
                    "length": "len"
                },
                "len": "int"
            }
        }
    }
}
```

Added attribute "length" definition in parameter "buf", whose value is an expression representing the length of the array, which can include other parameters. In the generation module, we call it in the following ways:

```
import {fill_buf} from "my_module";

let buf=new Uint8Array (256);
fill_buf (buf, 256);
```

Sometimes in C language, pointers are passed in as parameters, and numerical values are inputted into a function through pointers. We refer to these parameters as output parameters. If a parameter pointer is used to both pass in and return data, we refer to these parameters as input-output parameters. For example, C language function definition:

```
int my_func (int pin, int *point, int *pinout)
{
    int r = pin + *pinout;

    *pout = pin;
    *pinout = -pin;

    return r;
}
```

Among them, "pin" is the input parameter, "pout" is the output parameter, and "pinout" is the input-output parameter. The corresponding description file is defined as:

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

We have added the "direction" attribute in the parameter definition to represent the input-output characteristics of the parameter. If a parameter does not have a "direction" attribute defined, it indicates that it is an input parameter.
In JavaScript, we call it in the following way:

```
import {my_func} from "my_module";

print (JSON.stringify(my_func (123, 321), null, "  "));
```

The print result is:

```
{
    "return": 444,
    "pout": 123,
    "pinout": -123
}
```
As can be seen, in JavaScript function calls, only input and output parameters need to be passed in. The return value is an object, which includes the return value of the C function corresponding to the property "return", as well as the return values of all output parameters and input-output parameters.

Note that in C language, sometimes an array or buffer is used to store output data. At this time, njsgen will not place it in the properties of the input object and still needs to allocate the object before calling the function, passing it as a parameter to the function.
