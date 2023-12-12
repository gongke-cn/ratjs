#!/bin/bash

function clean
{
    make dist-clean O=$1
}

function build
{
    local O=$1

    shift

    echo build O=$O $@

    make dist-clean O=$O || exit 1
    make config O=$O $@ || exit 1

    make O=$O $@ -j64 || exit 1
}

if [ "x$1" = "xclean" ]; then
    clean out
    clean out_conv_iconv
    clean out_conv_internal
    clean out_no_module
    clean out_no_script
    clean out_big_int_gmp
    clean out_no_big_int
    clean out_no_priv_name
    clean out_no_generator
    clean out_no_async
    clean out_no_arrow_func
    clean out_no_bound_func
    clean out_no_proxy
    clean out_no_eval
    clean out_no_math
    clean out_no_date
    clean out_no_uri
    clean out_no_array_buffer
    clean out_no_shared_array_buffer
    clean out_no_int_indexed_object
    clean out_no_data_view
    clean out_no_atomics
    clean out_no_map
    clean out_no_set
    clean out_no_weak_map
    clean out_no_weak_set
    clean out_no_weak_ref
    clean out_no_finalization_registry
    clean out_no_json
    clean out_no_reflect
    clean out_no_func_source
    clean out_no_extension
    clean out_no_native_module
    clean out_no_ctype
    clean out_clang
    clean out_m32
    clean out_mingw_32
    clean out_mingw_64
    clean out_osize
    clean out_static_only
else
    build out
    build out_conv_iconv ENC_CONV=iconv
    build out_conv_internal ENC_CONV=internal
    build out_no_module ENABLE_MODULE=0
    build out_no_script ENABLE_SCRIPT=0
    build out_big_int_gmp ENABLE_BIG_INT=gmp
    build out_no_big_int ENABLE_BIG_INT=0
    build out_no_priv_name ENABLE_PRIV_NAME=0
    build out_no_generator ENABLE_GENERATOR=0
    build out_no_async ENABLE_ASYNC=0
    build out_no_arrow_func ENABLE_ARROW_FUNC=0
    build out_no_bound_func ENABLE_BOUND_FUNC=0
    build out_no_proxy ENABLE_PROXY=0
    build out_no_eval ENABLE_EVAL=0
    build out_no_math ENABLE_MATH=0
    build out_no_date ENABLE_DATE=0
    build out_no_uri ENABLE_URI=0
    build out_no_array_buffer ENABLE_ARRAY_BUFFER=0 ENABLE_SHARED_ARRAY_BUFFER=0 ENABLE_INT_INDEXED_OBJECT=0 ENABLE_DATA_VIEW=0 ENABLE_ATOMICS=0
    build out_no_shared_array_buffer ENABLE_SHARED_ARRAY_BUFFER=0 ENABLE_ATOMICS=0
    build out_no_int_indexed_object ENABLE_INT_INDEXED_OBJECT=0 ENABLE_ATOMICS=0
    build out_no_data_view ENABLE_DATA_VIEW=0
    build out_no_atomics ENABLE_ATOMICS=0
    build out_no_map ENABLE_MAP=0
    build out_no_set ENABLE_SET=0
    build out_no_weak_map ENABLE_WEAK_MAP=0
    build out_no_weak_set ENABLE_WEAK_SET=0
    build out_no_weak_ref ENABLE_WEAK_REF=0 ENABLE_WEAK_SET=0 ENABLE_WEAK_MAP=0
    build out_no_finalization_registry ENABLE_FINALIZATION_REGISTRY=0
    build out_no_json ENABLE_JSON=0
    build out_no_reflect ENABLE_REFLECT=0
    build out_no_func_source ENABLE_FUNC_SOURCE=0
    build out_no_extension ENABLE_EXTENSION=0
    build out_no_native_module ENABLE_NATIVE_MODULE=0
    build out_no_ctype ENABLE_CTYPE=0
    build out_clang CLANG=1
    build out_m32 M=32
    build out_mingw_32 ARCH=win CROSS_COMPILE=i686-w64-mingw32-
    build out_mingw_64 ARCH=win CROSS_COMPILE=x86_64-w64-mingw32-
    build out_osize OPTIMIZE_FOR_SIZE=1
    build out_static_only STATIC_LIBRARY_ONLY=1
fi

