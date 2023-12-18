import {
    NUM_MACRO_0, NUM_MACRO_PI,
    MY_ENUM0_V0, MY_ENUM0_V1, MY_ENUM0_V2,
    MY_ENUM1_V2, MY_ENUM1_V3, MY_ENUM1_V4,
    MyStruct1, MyStruct2, MyUnion,
    MyFunc0, MyFunc1, MyFunc2, MyFunc3, MyFunc4, MyFunc5, MyFunc6, MyFunc7, MyFunc8, MyFunc9,
    MyFuncType, MyFuncType1,
    $
} from "njsgen_test";

let testFailCount = 0;
let testPassCount = 0;

function test (src)
{
    let r = eval(src);

    if (!r) {
        prerr(`${src} failed!\n`);
        testFailCount ++;
    } else {
        testPassCount ++;
    }
}

function main ()
{
    test("NUM_MACRO_0 == 0x19781009");
    test("NUM_MACRO_PI == 3.1415926");
    test("MY_ENUM0_V0 == 0");
    test("MY_ENUM0_V1 == 1");
    test("MY_ENUM0_V2 == 2");
    test("MY_ENUM1_V2 == 2");
    test("MY_ENUM1_V3 == 3");
    test("MY_ENUM1_V4 == 4");

    test("let s = new MyStruct1(); s.u8 == 0");
    test("let s = new MyStruct1(); s.u8 = 255; s.u8 == 255");
    test("let s = new MyStruct1(); s.u8 = 256; s.u8 == 0");
    test("let s = new MyStruct1(); s.u8 = -1; s.u8 == 255");
    test("let s = new MyStruct1(); s.u16 == 0");
    test("let s = new MyStruct1(); s.u16 = 0xffff; s.u16 == 0xffff");
    test("let s = new MyStruct1(); s.u16 = 0x10000; s.u16 == 0");
    test("let s = new MyStruct1(); s.u16 = -1; s.u16 == 0xffff");
    test("let s = new MyStruct1(); s.u32 == 0");
    test("let s = new MyStruct1(); s.u32 = 0xffffffff; s.u32 == 0xffffffff");
    test("let s = new MyStruct1(); s.u32 = 0x100000000; s.u32 == 0");
    test("let s = new MyStruct1(); s.u32 = -1; s.u32 == 0xffffffff");
    test("let s = new MyStruct1(); s.u64 == 0");
    test("let s = new MyStruct1(); s.u64 = 0x100000000; s.u64 == 0x100000000");
    test("let s = new MyStruct1(); s.u64 = -1; s.u64 == 0xffffffffffffffff");
    test("let s = new MyStruct1({u8:1,u16:2,u32:3,u64:4}); s.u8==1 && s.u16==2 && s.u32==3 && s.u64==4");

    test("let s = new MyStruct2(); s.s == ''");
    test("let s = new MyStruct2(); s.s = 'abcdefg'; s.s == 'abcdefg'");
    test("let s = new MyStruct2({s:'xxxx', s1:{u32:19781009}}); s.s=='xxxx' && s.s1.u32 == 19781009");
    test("let s = new MyStruct2(); s.u8a.length == 32");
    test("let s = new MyStruct2(); s.u8a.fill(47); s.u8a[31] == 47");
    test("let s = new MyStruct2(); s.s1a.length == 4");
    test("let s = new MyStruct2(); s.s1a[0].u32 = 19781009; s.s1a[0].u32 == 19781009");
    test("let s = new MyStruct2(); s.s1a[0] = {u32:19490517}; s.s1a[0].u32 == 19490517");
    test("let a = new MyStruct2(); let b = new MyStruct1({u32:19490609}); a.s1a[1]=b; a.s1a[1].u32 == 19490609");
    test("let a = new MyStruct1(2); let b = new MyStruct1(3); b[0]={u32:19820810};b[1]={u32:19840721};b[3]={u32:19921812};a=b;a[0].u32==19820810 && a[1].u32==19840721");

    test("let s = new MyStruct2(); let m1 = s.s1; let m2 = s.s1; m1 == m2;");

    test("let u = new MyUnion({u64:0xab12345678}); u.u8 == 0x78 && u.u16 == 0x5678 && u.u32 == 0x12345678 && u.u64 == 0xab12345678");

    test("MyFunc0(19491009) == -19491009")
    test("MyFunc1(null) == 0");
    test("MyFunc1('abcdefg') == 7");
    test("MyFunc2() == 'test!'");
    test("let r = MyFunc3(); r.a == 1900");
    test("let r = MyFunc4(1234); r.return == 1234 && r.a == -1234");
    test("let s = new MyStruct1({u32:0xf0f0f0f0}); MyFunc5(s); s.u32 == 0x0f0f0f0f");
    test("let a = new MyStruct1(2); MyFunc6(a); a[0].u32 == 1234 && a[1].u32 == 5678");
    test("let a = new Uint16Array(3); MyFunc7(a, 3); a[0] == 0 && a[1] == 1 && a[2] == 2");

    test("$.iv == 1314250");
    test("$.iv = 762473; $.iv == 762473");
    test("$.sv.u32 == 0");
    test("$.sv.u32 = 4637981; $.sv.u32 == 4637981");
    test("$.csv.u32 == 1234567");

    test("MyFunc8(a=>a*10,10) == -100");
    test("MyFunc8(a=>a*10,1) == -10");
    test("$.fnv(10) == 110");
    test("$.fnv(0) == 100");
    test("MyFunc8($.fnv, 0) == -100");
    test("MyFunc8($.fnv, 100) == -200");
    test("let f = new MyFuncType(a => a + a); MyFunc8(f, 100) == -200");

    test(`MyFunc9((a,c,d) => {
        for (let i = 0; i < 256; i ++) {
            if (d[i] != i)
                return 0;
        }
        if (c.u32 != 987654321)
            return 0;
        return {
            b: 1234567,
            return: 7654321
        }
    }) == 8888888`);

    print(`passed: ${testPassCount} failed: ${testFailCount}\n`);
}