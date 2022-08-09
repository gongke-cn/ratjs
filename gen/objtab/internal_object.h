static const OBJ_Entry
objects[] = {
    {"Object_prototype"},
    {"Array"},
    {"Array_prototype"},
    {"Boolean_prototype"},
    {"Number_prototype"},
    {"String_prototype"},
    {"Symbol_prototype"},
    {"RegExp"},
    {"RegExp_prototype"},
    {"Function"},
    {"Function_prototype"},
    {"Promise"},
    {"Promise_prototype"},
    {"Array_prototype_values"},
    {"Object_prototype_toString"},
    {"IteratorPrototype"},
    {"StringIteratorPrototype"},
    {"ArrayIteratorPrototype"},
    {"RegExpStringIteratorPrototype"},
    {"ForInIteratorPrototype"},
    {"Error"},
    {"RangeError"},
    {"ReferenceError"},
    {"SyntaxError"},
    {"TypeError"},
    {"AggregateError"},
    {"Error_prototype"},
    {"EvalError_prototype"},
    {"RangeError_prototype"},
    {"ReferenceError_prototype"},
    {"SyntaxError_prototype"},
    {"TypeError_prototype"},
    {"AggregateError_prototype"},
    {"ThrowTypeError"},
    {"Concat"},
    {"eval"},
    {"parseFloat"},
    {"parseInt"},
    {"Array_prototype_toString"},
#if ENABLE_DATE
    {"Date"},
    {"Date_prototype"},
#endif /*ENABLE_DATE*/
#if ENABLE_BIG_INT
    {"BigInt_prototype"},
#endif /*ENABLE_BIG_INT*/
#if ENABLE_GENERATOR
    {"GeneratorFunction"},
    {"GeneratorFunction_prototype"},
    {"Generator_prototype"},
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
    {"AsyncFunction"},
    {"AsyncFunction_prototype"},
    {"AsyncIteratorPrototype"},
    {"AsyncFromSyncIteratorPrototype"},
#endif /*ENABLE_ASYNC*/
#if ENABLE_GENERATOR && ENABLE_ASYNC
    {"AsyncGeneratorFunction"},
    {"AsyncGeneratorFunction_prototype"},
    {"AsyncGenerator_prototype"},
#endif /*ENABLE_GENERATOR && ENABLE_ASYNC*/
#if ENABLE_URI
    {"URIError"},
    {"URIError_prototype"},
#endif /*ENABLE_URI*/
#if ENABLE_ARRAY_BUFFER
    {"ArrayBuffer"},
    {"ArrayBuffer_prototype"},
#endif /*ENABLE_ARRAY_BUFFER*/
#if ENABLE_SHARED_ARRAY_BUFFER
    {"SharedArrayBuffer"},
    {"SharedArrayBuffer_prototype"},
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/
#if ENABLE_INT_INDEXED_OBJECT
    {"TypedArray"},
    {"Int8Array"},
    {"Uint8Array"},
    {"Uint8ClampedArray"},
    {"Int16Array"},
    {"Uint16Array"},
    {"Int32Array"},
    {"Uint32Array"},
    {"Float32Array"},
    {"Float64Array"},
    {"TypedArray_prototype"},
    {"Int8Array_prototype"},
    {"Uint8Array_prototype"},
    {"Uint8ClampedArray_prototype"},
    {"Int16Array_prototype"},
    {"Uint16Array_prototype"},
    {"Int32Array_prototype"},
    {"Uint32Array_prototype"},
    {"Float32Array_prototype"},
    {"Float64Array_prototype"},
    {"TypedArray_prototype_values"},
#if ENABLE_BIG_INT
    {"BigInt64Array"},
    {"BigUint64Array"},
    {"BigInt64Array_prototype"},
    {"BigUint64Array_prototype"},
#endif /*ENABLE_BIG_INT*/
#endif /*ENABLE_INT_INDEXED_OBJECT*/
#if ENABLE_DATA_VIEW
    {"DataView_prototype"},
#endif /*ENABLE_DATA_VIEW*/
#if ENABLE_MAP
    {"Map_prototype"},
    {"MapIteratorPrototype"},
    {"Map_prototype_entries"},
#endif /*ENABLE_MAP*/
#if ENABLE_SET
    {"Set_prototype"},
    {"SetIteratorPrototype"},
    {"Set_prototype_values"},
#endif /*ENABLE_SET*/
#if ENABLE_WEAK_MAP
    {"WeakMap_prototype"},
#endif /*ENABLE_WEAK_MAP*/
#if ENABLE_WEAK_SET
    {"WeakSet_prototype"},
#endif /*ENABLE_WEAK_SET*/
#if ENABLE_WEAK_REF
    {"WeakRef_prototype"},
#endif /*ENABLE_WEAK_REF*/
#if ENABLE_FINALIZATION_REGISTRY
    {"FinalizationRegistry_prototype"},
#endif /*ENABLE_FINALIZATION_REGISTRY*/
#if ENABLE_EXTENSION
    {"File_prototype"},
    {"Dir_prototype"},
#endif /*ENABLE_EXTENSION*/
    {NULL}
};
