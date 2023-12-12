# njsgen - Rat Javascript 原生模块生成工具

## 原生模块

Rat Javascript支持标准ECMAScript模块。为提升javascript运行效率，Rat Javascript支持用其他编译语言开发原生模块，扩展并提升javascript的运行效率。
原生模块后缀为".njs"，其文件格式为动态连接库，ratjs通过"dlopen/dlsym"加载模块并调用其中的函数。在原生模块中，需要包含以下两个函数实现：

```
RJS_Result
ratjs_module_init (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
...
}

RJS_Result
ratjs_module_exec (RJS_Runtime *njs_rt, RJS_Value *njs_mod)
{
...
}
```
函数"ratjs_module_init"在模块初始化阶段被调用，此函数中主要工作是向模块注册导入和导出的符号表。函数"ratjs_module_exec"在模块执行阶段被调用，此函数中主要工作是创建并初始化模块中导出符号对应的对象。
可参考[原生模块参考代码](../demo/native_module/native_module.c)了解如何通过C语言编写原生模块。

## 编译生成

因为原生模块就是动态连接库，可以按照动态连接库的方法编译生成原生模块。下面就是在linux平台上通过gcc编译生成原生模块的命令。

```
$gcc -o my_module.njs my_module -shared -lratjs
```

## njsgen简介

njsgen是一个用javascript编写的程序，可以帮助你快速针对已有的C语言开发库生成对应的javascript原生模块。

比如你有一个C语言开发的库,你希望通过javascript调用库中的函数。通过njsgen可以快速生成原生模块的C语言源文件。

具体操作步骤如下：

1. 创建接口描述JSON文件
2. 运行njsgen扫描接口描述文件生成对应的原生模块源文件
3. 运行C编译器将原生模块源文件编译成原生模块

## njsgen命令及参数

njsgen命令其参数如下：

```
Usage: njsgen [OPTION]... FILE
Options:
  -o STRING         set the output filename
  -v                validate the input JSON
  --help            show this help message

```

如使用njsgen扫描描述文件"my_module.json",生成原生模块源文件"my_module.c",命令如下：

```
$njsgen -o my_module.c my_module.json
```
没有通过"-o"参数指定输出源文件名称，如描述文件为"XXX.json"，njsgen将使用"XXX.c"作为输出文件名。
如果运行时带参数"-v"，njsgen将通过json schema校验描述文件的格式，如文件格式有错，njsgen将报错退出。

## njsgen描述文件

njsgen描述文件为JSON格式，其JSON schema描述见[njsgen json schema](../module/njsgen/njs-schema.json)。
以下将详细描述如何在描述文件中添加常见C语言定义。

### 数据类形

描述文件中很多属性定义C语言数据类型。目前njsgen识别以下C语言基本数据类型：

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

#### 数组类型

在一个数据类型后加"[...]"表示一个数组类型。"[]"中间可以加入一个表达式，表示数组的长度。

比如：

```
int[]
char[256]
double[MAX_ARRAY_LENGTH]
```

#### 指针类型

在一个数据类型后加入"*"表示一个指针类型。

比如：

```
char*
int**
double*
```

#### 枚举，数据结构和联合体

在描述文件中定义的枚举，数据结构和联合体，也可以作为数据类型名称使用。

比如：

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

### 数字类型宏

C语言中定义经常会用"#define"定义一些数字类型的宏，如果想在javascript中使用这些定义，可以在描述文件中添加"numberMacros"属性描述。
如C语言中定义：

```
#define MY_INTEGER 19491001
#define MY_FLOAT   1.234567
```

在对应描述文件"my_module"中定义：

```
{
    "numberMacros": [
        "MY_INTEGER",
        "MY_FLOAT"
    ]
}
```
njsgen为每个数字类型的宏生成一个number类型的export项。

生成原生模块后，可通过如下方式调用：

```
import {MY_INTEGER, MY_FLOAT} from "my_module";

print(`${MY_INTEGER} ${MY_FLOAT}\n`);
```

### 枚举

C语言中的枚举可通过在描述文件中添加"enumerations"属性加入原生模块。
如C语言中定义：

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
在对应描述文件"my_module"中定义：

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
njsgen为枚举的每个取值都生成一个对应的number类型export项。

生成原生模块后，可通过如下方式调用：

```
import {COLOR_RED, COLOR_GREEN, COLOR_BLUE, MALE, FEMALE} from "my_module";

let color = COLOR_RED;
let gender = MALE;

print(`color:${color} gender:${gender}\n`);

```

### 数据结构

C语言中通过"struct"定义的数据结构可在描述文件中添加"structures"属性加入原生模块。
如C语言中定义：

```
typedef struct {
    char name[64];
    int age;
    Gender gender;
} User;
```

在对应描述文件"my_module"中定义：

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
生成原生模块后，可通过如下方式调用：

```
import {User} from "my_module";

let user = new User();

user.name = "Monkey King";
user.age = 500;
user.gender = MALE;

print(`${JSON.stringify(user)}\n`);

```

njsgen为结构"User"生成对应的Prototype对象和构造函数。通过调用构造函数分配一个新的数据结构对象，读写对象的属性对因操作相应的数据结构成员。

在构造函数中，可以通过传入一个Object快速设置结构的成员：

```
let user = new User({name: "Monkey King", age: 500, gender: MALE});
```

如果构造函数的第一个参数传入一个数字，则创建一个长度等于此数字的数组：

```
let users = new User(2);

users[0] = {name: "Zhuge Liang", age: 50, gender: MALE};
users[1] = {name: "Sima Yi", age: 50, gender: FEMALE};
```

通过构造函数创建的数据结构缓冲区将在对象被GC回收时释放。如果不希望数据结构缓冲区由GC管理，可以在构造函数最后传一个false表示创建的是一个外部缓冲区，缓冲区不会自动释放。

```
let externUser = new User(false); //Create an external user object buffer.
```

有时候我们希望通过特定的C函数创建对应，而不是通过njsgen自动生成的方法创建对象，我们可以在描述文件数据结构定义中增加"noConstructor"属性。

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
这样在生成的模块中就没有构造函数"User"了。

有时在C语言中，我们定义数据结构不会使用"typedef"定义对应的类型名，我们需要通过"struct STRUCTNAME"引用这个结构，此时需要在描述文件数据结构定义中增加"noTypeDef"属性。

比如C语言中定义：

```
struct MyStruct {
    int a;
    int b;
};
```

描述文件需要定义如下：

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

njsgen会为数据结构的每个成员定义对应的getter和setter方法，分别用于读取和写入成员内容。如果一些成员我们只希望读取，不能修改，我们可以对成员声明"readonly"属性。

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
这样生成的模块中"MyStruct"的"b"属性只有getter方法，尝试修改b属性将抛出异常。

如果数据结构中所有成员都是"readonly"的，可以直接对数据结构声明"readonly"属性。

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
这样生成的模块中"MyStruct"的属性"a"和属性"b"都是只读的。

### 联合

C语言中通过“union”定义的联合体可在描述文件中添加"unions"属性加入原生模块。
如C语言中定义：

```
typedef union {
    uint8_t  u8;
    uint16_t u16;
    uint32_t u32;
} MyUnion;
```

在对应描述文件"my_module"中定义：

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

生成原生模块后，可通过如下方式调用：

```
import {MyUnion} from "my_module";

let u = new MyUnion();

u.u32 = 0x12345678;

print(`${JSON.stringify(u)}\n`);

```

联合体和数据结构类似，也可以用对象作为构造函数参数快速初始化，也可以通过传入数值参数创建联合体数组，也可以在构造函数中传入false表示在垃圾收集对象时不自动释放分配的缓冲区。
同样，"union"也支持"noConstructor","noTypeDef","readonly"属性。

### 变量

C语言中的变量可以通过描述文件中添加"variables"属性加入模块。

如C语言中定义如下变量：

```
int my_int_var;
User my_user_info;
```
在描述文件中定义：

```
{
    "variables": {
        "my_int_var": "int",
        "my_user_info": "User"
    }
}
```
在javascript中可以通过以下方法调用：

```
import {my_int_var, my_user_info} from "my_module";

$.my_int_var = 1;
$.my_user_info.name = "Liu Bei";

print(`${$.my_int_var} ${JSON.stringify($.my_user_info)}`);
```
可以看到njsgen生成了一个名为"$"的对象，njsgen为每一个变量生成一个"$"对象的accessor属性，accessor的getter和setter方法对应变量的读取和写入操作。

如果一个变量是常量，可以定义其"readonly"属性为true，njsgen将只生成这个变量对应accessor的getter方法。

如C语言中定义常量：

```
const int my_const = 19491009;
```
在描述文件中定义：

```
{
    "variables": {
        "my_const": {
            "type": "int",
            "readonly" : true
        }
    }
}
```

### 函数

C语言中定义的函数可以通过描述文件中添加"functions"属性加入模块。
如C语言中定义函数：

```
int my_func (int a)
{
    ...
}
```
在对应描述文件"my_module"中定义：

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

生成原生模块后，可通过如下方式调用：

```
import {my_func} from "my_module";

print(my_func(1949),"\n");

```

在描述文件中每个函数定义了"parameters"和"return"两个属性，分别描述了函数的参数和返回值。如果函数参数为空，可以不定义"parameters"属性。如果返回值为"void",可以不定义"return"属性。

有些函数的参数为数值类型的数组，比如：

```
void fill_buf_256 (uint8_t buf[256])
{
    ...
}
```
在对应描述文件"my_module"中定义：

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
在生成模块中，我们可以创建一个TypedArray作为参数传入。

```
import {fill_buf_256} from "my_module";

let buf = new Uint8Array(256);

fill_buf_256(buf);
```

注意TypedArray的类型要和C语言中数据类型匹配，并且长度要大于等于C语言声明中的长度。否则运行时会抛出异常。

有时C语言中希望传入数组的长度作为参数传入。比如：

```
void fill_buf (uint8_t *buf, int len)
{
    ...
}
```
此时我们描述文件定义为：

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
在参数"buf"中增加了属性"length"定义，其值为一个表示数组长度的表达式，其中可以包含其他参数。在生成模块中，我们通过以下方式调用：

```
import {fill_buf} from "my_module";

let buf = new Uint8Array(256);

fill_buf(buf, 256);
```

有时C语言中会通过参数传入指针，在函数中通过指针输出一些数值，我们称这些参数为输出参数。如一个参数指针既传入数据又用来返回数据，我们称这些参数为输入输出参数。比如C语言函数定义：

```
int my_func (int pin, int *pout, int *pinout)
{
    int r = pin + *pinout;

    *pout = pin;
    *pinout = -pin;

    return r;
}

```
其中"pin"是输入参数，"pout"是输出参数，"pinout"是输入输出参数。对应描述文件定义为：

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
我们在参数定义中增加了"direction"属性表示参数的输入输出特性。如果一个参数没有定义"direction"属性，则表示其为输入参数。
在javascript中，我们用以下方式调用：

```
import {my_func} from "my_module";

print(JSON.stringify(my_func(123, 321), null, "  "));
```
运行打印结果为：

```
{
  "return": 444,
  "pout": 123,
  "pinout": -123
}
```
可以看到，在javascript的函数调用中，只需要传入输入参数和输入输出参数。而返回值是一个对象，其中包括属性"return"对应C函数返回值，还有所有输出参数和输入输出参数的返回值。

注意，在C语言中有时会用一个数组或缓冲区作为参数存放输出数据，此时njsgen不会将其放到输入对象的属性中，仍然需要在调用函数前分配对象，将其作为参数传给函数。比如下面的函数：

```
void get_data (uint8_t *buf, int len)
{
    buf[0] = 0x47;
    buf[1] = 0x00;
    ...
}

void get_user_info (User *u)
{
    snprintf(u->name, sizeof(u->name), "%s", "Guan Yu");
    u->age = 1000;
    u->gender = MALE;
}
```
对应描述文件定义为：

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
在javascript中调用方法如下：

```
import {get_path, get_user_info, User} from "my_module";

let buf = new Uint8Array(256);
get_data(buf, 256);

let user = new User();
get_user_info(user);

```

### C语言与javascript函数互相调用

为了让C语言与javascript函数互相调用，需要在描述文件定义"functionTypes"属性。

比如,C语言定义函数类型如下：

```
typedef int (*MyFunc) (int a);
```
在描述文件中添加定义：

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
如果C语言中有以下函数使用"MyFunc"作为参数：

```
void func_with_func_parm(MyFunc cb)
{
    int i;
    for (i = 0; i < 100; i++)
        printf("%d ", cb(i));
}
```
在描述文件中定义此函数为：

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
在javascript中，我们可以直接传入一个javascript函数，其会被转化为C函数被调用执行：

```
import {func_with_func_parm} from  "my_module";

func_with_func_parm(n => n * n);
```

如果C语言定义了一个函数指针类型的变量：

```
MyFunc my_func_var;
```
在描述文件中定义此变量为:

```
{
    ...
    "variables": {
        "my_func_var": "MyFunc"
    }
}
```
在javascript中也可以直接调用此变量：

```
import {my_func_var} from  "my_module";

print(my_func_var(100));
```

### 用户代码槽

njsgen生成原生模块的源文件。有时我们希望在源文件中添加自己的代码。njsgen在源文件中会加入若干用户代码槽，用户可以在其中添加自己的代码。这样当描述文件更新，需要再次运行njsgen更新原生模块源文件时，njsgen会自动将之前用户添加的自定义代码合并到新生成的原生模块源文件中。

用户代码槽在源文件中格式如下：

```
/*NJSGEN XXXXXXXX BEGIN*/
/*NJSGEN TODO*/
...
/*NJSGEN XXXXXXXX END*/
```
可以看到，用户代码槽是由开始行标志"/\*NJSGEN XXXXXXX BEGIN\*/"和结束行标志"/\*NJSGEN XXXXXXX END\*/"包围起来的多行代码组成的。用户可以在其中添加自己的代码。其中"XXXXXXX"为代码块标志tag，tag在源文件中是唯一的。目前njsgen生成的slot tag包括以下几个：

+ INCLUDE_HEAD: 在"#include"头文件包含代码块开头
+ INCLUDE_TAIL: 在"#include"头文件包含代码块末尾
+ DECL: 变量和类型声明代码块
+ EXPORT_TABLE_DECL: 导入导出符号表声明
+ MODULE_DATA_DECL: 模块私有数据声明
+ MODULE_DATA_SCAN: 模块私有数据GC scan函数
+ MODULE_DATA_FREE: 模块私有数据GC free函数
+ FUNC_HEAD: 函数定义代码块开头
+ FUNC_TAIL: 函数定义代码块末尾
+ MODULE_INIT: 模块初始化函数
+ MODULE_EXEC: 模块执行函数

比如，可以在"INCLUDE_TAIL"块中添加需要包含的头文件。

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