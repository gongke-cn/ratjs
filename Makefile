MAJOR_VERSION := 0
MINOR_VERSION := 3
MICRO_VERSION := 0

SO_MAJOR_VERSION := 1
SO_MINOR_VERSION := 0
SO_MICRO_VERSION := 0

# Quiet
Q ?= @
# Output directory
O ?= out

# Remove command
RM    := rm -f
# Remove the directory
RMDIR := rm -rf
# Make directory command
MKDIR := mkdir -p
# Install prefix
INSTALL_PREFIX ?= /usr

-include $(O)/config.mk

# C compiler
ifeq ($(CLANG),1)
	CC := $(CROSS_COMPILE)clang
else
	CC := $(CROSS_COMPILE)gcc
endif

# Archive program
AR := $(CROSS_COMPILE)ar
# ranlib program
RANLIB := $(CROSS_COMPILE)ranlib

# Compiling flags
CFLAGS += -Wall -Iinclude -I$(O)/include -I$(O)/src/lib
# Linked libraries and flags
LIBS += -lm

ifneq ($(OPTIMIZE_FOR_SIZE),1)
	CFLAGS += -O2
else
	CFLAGS += -Os
endif

ifneq ($(STATIC_LIBRARY_ONLY),1)
	CFLAGS += -fPIC
endif

ifeq ($(ARCH),win)
	# Suffix of static library
	SLIB_SUFFIX := .a
	# Suffix of dynamic library
	DLIB_SUFFIX := .dll
	# Suffix of executable program
	EXE_SUFFIX  := .exe
	ifndef ENABLE_COLOR_CONSOLE
		# Disable color console
		ENABLE_COLOR_CONSOLE := 0
	endif
	LIBRATJS_SRCS += src/lib/rjs_arch_win_opt.c
else
	# Default architecture is linux
	ARCH        := linux
	# Suffix of static library
	SLIB_SUFFIX := .a
	# Suffix of dynamic library
	DLIB_SUFFIX := .so
	# Suffix of executable program
	EXE_SUFFIX  :=
	# Link libdl
	LIBS += -lpthread -ldl
endif

ifeq ($(DEBUG),1)
	CFLAGS += -g
endif

ifeq ($(NOASSERT),1)
	CFLAGS += -DNDEBUG
endif

ifeq ($(M),32)
	CFLAGS += -m32
	LIBS += -m32
else
	ifeq ($(M),64)
		CFLAGS += -m64
		LIBS += -m64
	endif
endif

# Define a boolean configure entry
define bool_config =
ifndef $(1)
	$(1) := $(2)
endif
ifeq ($($(1)),1)
	LIBRATJS_SRCS += $(4)
else
	LIBRATJS_SRCS += $(5)
endif
$(1)_DEFAULT := $(2)
$(1)_DESC    := $(3)
$(1)_ENUM    := 0|1
CONFIG_ITEMS += $(1)
endef

# icu/iconv/internal encoding converter
ifndef ENC_CONV
	ENC_CONV := icu
endif
ENC_CONV_INTERNAL:= 0
ENC_CONV_ICONV   := 1
ENC_CONV_ICU     := 2
ifeq ($(ENC_CONV),iconv)
	ENC_CONV_C_VALUE := $(ENC_CONV_ICONV)
	LIBRATJS_SRCS += src/lib/rjs_conv_iconv_opt.c src/lib/rjs_uchar_internal_opt.c
	ifeq ($(ARCH),win)
		LIBS += -liconv
	endif
else
ifeq ($(ENC_CONV),internal)
	ENC_CONV_C_VALUE := $(ENC_CONV_INTERNAL)
	LIBRATJS_SRCS += src/lib/rjs_conv_internal_opt.c src/lib/rjs_uchar_internal_opt.c
else
	ENC_CONV_C_VALUE := $(ENC_CONV_ICU)
	LIBRATJS_SRCS += src/lib/rjs_conv_icu_opt.c src/lib/rjs_uchar_icu_opt.c
ifeq ($(ARCH),win)
	LIBS += -licuuc -licudt -lstdc++
else
	LIBS += -licuuc -licudata -lstdc++
endif
endif
endif

ENC_CONV_DEFAULT := icu
ENC_CONV_DESC    := character encoding converter. icu4c, libiconv or internal UTF8/UTF16/UCS converter. if select icu, can support full ECMA262 unicode features
ENC_CONV_ENUM    := icu|iconv|internal
CONFIG_H_ITEMS   += ENC_CONV_ICU ENC_CONV_ICONV ENC_CONV_INTERNAL
CONFIG_ITEMS     += ENC_CONV

# Big integer
ifndef ENABLE_BIG_INT
	ENABLE_BIG_INT := internal
endif
ENABLE_BIG_INT_GMP      := 1
ENABLE_BIG_INT_INTERNAL := 2
ifeq ($(ENABLE_BIG_INT),gmp)
	ENABLE_BIG_INT_C_VALUE := $(ENABLE_BIG_INT_GMP)
	LIBRATJS_SRCS += src/lib/rjs_big_int_gmp_opt.c
	LIBS += -lgmp
else
ifeq ($(ENABLE_BIG_INT),internal)
	ENABLE_BIG_INT_C_VALUE := $(ENABLE_BIG_INT_INTERNAL)
	LIBRATJS_SRCS += src/lib/rjs_big_int_internal_opt.c
else
	ENABLE_BIG_INT_C_VALUE := 0
endif
endif

ifeq ($(ENABLE_CTYPE),1)
	LIBS += -lffi
endif

ENABLE_BIG_INT_DEFAULT := internal
ENABLE_BIG_INT_DESC    := big integer. use internal big integer implement, link with libgmp or disable big integer feature
ENABLE_BIG_INT_ENUM    := internal|gmp|0
CONFIG_H_ITEMS   += ENABLE_BIG_INT_GMP ENABLE_BIG_INT_INTERNAL
CONFIG_ITEMS     += ENABLE_BIG_INT

# Configure options
$(eval $(call bool_config,ENABLE_MODULE,1,enable module,src/lib/rjs_module_opt.c src/lib/rjs_module_env_opt.c src/lib/rjs_module_ns_object_opt.c))
$(eval $(call bool_config,ENABLE_SCRIPT,1,enable script))
$(eval $(call bool_config,ENABLE_PRIV_NAME,1,enable private name,src/lib/rjs_private_name_opt.c))
$(eval $(call bool_config,ENABLE_GENERATOR,1,enable genrtator,src/lib/rjs_generator_opt.c))
$(eval $(call bool_config,ENABLE_ASYNC,1,enable async function,src/lib/rjs_async_function_opt.c))
$(eval $(call bool_config,ENABLE_ARROW_FUNC,1,enable arrow function))
$(eval $(call bool_config,ENABLE_BOUND_FUNC,1,enable bound function object,src/lib/rjs_bound_func_object_opt.c))
$(eval $(call bool_config,ENABLE_ARRAY_BUFFER,1,enable array buffer object,src/lib/rjs_array_buffer_opt.c src/lib/rjs_data_block_opt.c))
$(eval $(call bool_config,ENABLE_SHARED_ARRAY_BUFFER,1,enable shared array buffer object))
$(eval $(call bool_config,ENABLE_INT_INDEXED_OBJECT,1,enable integer indexed object,src/lib/rjs_int_indexed_object_opt.c src/lib/rjs_typed_array_opt.c))
$(eval $(call bool_config,ENABLE_PROXY,1,enable proxy object,src/lib/rjs_proxy_object_opt.c))
$(eval $(call bool_config,ENABLE_EVAL,1,enable function "eval",src/lib/rjs_eval_opt.c))
$(eval $(call bool_config,ENABLE_MATH,1,enable object "Math"))
$(eval $(call bool_config,ENABLE_DATE,1,enable object "Date"))
$(eval $(call bool_config,ENABLE_URI,1,enable URI encode/decode functions))
$(eval $(call bool_config,ENABLE_DATA_VIEW,1,enable data view object))
$(eval $(call bool_config,ENABLE_ATOMICS,1,enable atomics object))
$(eval $(call bool_config,ENABLE_SET,1,enable set object))
$(eval $(call bool_config,ENABLE_MAP,1,enable map object))
$(eval $(call bool_config,ENABLE_WEAK_SET,1,enable weak set object))
$(eval $(call bool_config,ENABLE_WEAK_MAP,1,enable weak map object))
$(eval $(call bool_config,ENABLE_WEAK_REF,1,enable weak reference object,src/lib/rjs_weak_ref_opt.c))
$(eval $(call bool_config,ENABLE_FINALIZATION_REGISTRY,1,enable finalization registry object,src/lib/rjs_finalization_registry_opt.c))
$(eval $(call bool_config,ENABLE_JSON,1,enable JSON object,src/lib/rjs_json_opt.c))
$(eval $(call bool_config,ENABLE_REFLECT,1,enable reflect object))
$(eval $(call bool_config,ENABLE_HASHBANG_COMMENT,1,enable hashbang comment))
$(eval $(call bool_config,ENABLE_LEGACY_OPTIONAL,1,enable legacy optional functions))
$(eval $(call bool_config,ENABLE_CALLER,1,enable Function.caller))
$(eval $(call bool_config,ENABLE_COLOR_CONSOLE,1,enable color terminal output))
$(eval $(call bool_config,ENABLE_FUNC_SOURCE,1,enable function source))
$(eval $(call bool_config,ENABLE_NATIVE_MODULE,1,enable native module))
$(eval $(call bool_config,ENABLE_EXTENSION,1,enable extension functions,src/lib/rjs_extension_opt.c))
$(eval $(call bool_config,ENABLE_CTYPE,1,enable C type,src/lib/rjs_ctype_opt.c))
$(eval $(call bool_config,STATIC_LIBRARY_ONLY,0,do not generate the dynamic library))
$(eval $(call bool_config,OPTIMIZE_FOR_SIZE,0,optimize to reduce size))
$(eval $(call bool_config,ENABLE_BINDING_CACHE,1,enable the binding cache))

# Host C
HOST_CC := cc

# Host suffix of executable program
HOST_EXE_SUFFIX :=

# Host C flags
HOST_CFLAGS += -g -O2 -Wall -I$(O)/include

# Host linked libraries and flags
HOST_LIBS +=

# Lexical table generator
LEXTAB := $(O)/gen/lextab-gen$(HOST_EXE_SUFFIX)
# Source files of lexical table generator
LEXTAB_SRCS := $(wildcard gen/lextab/*.c)
# Generated lexical table
LEXTAB_OUTPUT := $(O)/src/lib/rjs_lex_table_inc.c
# Generated token type header
TOKENTYPE_OUTPUT := $(O)/src/lib/rjs_token_types.h

# Internal string table generator
STRTAB := $(O)/gen/strtab-gen$(HOST_EXE_SUFFIX)
# Source files of internal string table generator
STRTAB_SRCS := $(wildcard gen/strtab/*.c)
# Generated string table C file
STRTAB_C_OUTPUT := $(O)/src/lib/rjs_string_table_inc.c
# Generated string table header file
STRTAB_H_OUTPUT := $(O)/src/lib/rjs_string_table.h
# Generated string table functions header file
STRTAB_F_OUTPUT := $(O)/src/lib/rjs_string_table_function.h

# Internal object table generator
OBJTAB := $(O)/gen/objtab-gen$(HOST_EXE_SUFFIX)
# Source files of internal object table generator
OBJTAB_SRCS := $(wildcard gen/objtab/*.c)
# Generated object table header file
OBJTAB_H_OUTPUT := $(O)/src/lib/rjs_object_table.h
# Generated object table functions header file
OBJTAB_F_OUTPUT := $(O)/src/lib/rjs_object_table_function.h
# Generated object table C file.
OBJTAB_C_OUTPUT := $(O)/src/lib/rjs_object_table_inc.c

# AST generator
AST := $(O)/gen/ast-gen$(HOST_EXE_SUFFIX)
# Source files of AST generator
AST_SRCS := $(wildcard gen/ast/*.c)
# Generated AST C file
AST_C_OUTPUT := $(O)/src/lib/rjs_ast_inc.c
# Generated AST header file
AST_H_OUTPUT := $(O)/src/lib/rjs_ast.h

# Byte code generator
BC := $(O)/gen/bc-gen$(HOST_EXE_SUFFIX)
# Source files of byte code generator
BC_SRCS := $(wildcard gen/bc/*.c)
# Generated byte code header file
BC_H_OUTPUT := $(O)/src/lib/rjs_bc.h
# Generated byte code C file
BC_C_OUTPUT := $(O)/src/lib/rjs_bc_inc.c
# Generated byte code running C file
BC_R_OUTPUT := $(O)/src/lib/rjs_bc_run_inc.c

# Static library target: libratjs
LIBRATJS_SLIB := $(O)/libratjs$(SLIB_SUFFIX)
# Dynamic library target: libratjs
LIBRATJS_DLIB := $(O)/libratjs$(DLIB_SUFFIX)
# Libraries targets: libratjs
LIBRATJS := $(LIBRATJS_SLIB)
ifneq ($(STATIC_LIBRARY_ONLY),1)
LIBRATJS += $(LIBRATJS_DLIB)
endif
# Source files of libratjs
LIBRATJS_SRCS += $(filter-out $(wildcard src/lib/*_inc.c src/lib/*_opt.c),$(wildcard src/lib/*.c))

# Executable program: ratjs
RATJS := $(O)/ratjs$(EXE_SUFFIX)
# Source files of ratjs
RATJS_SRCS := $(wildcard src/exe/*.c)

# Test 262
TEST262 := $(O)/test262$(EXE_SUFFIX)
TEST262_SRCS := $(wildcard test/test262/*.c)

# Native javascript module
NJS_JSON := $(wildcard njs/*.json)
NJS_SRCS := $(patsubst %.json,%.c,$(NJS_JSON))
NJS_MODULES := $(patsubst %.json,$(O)/%.njs,$(NJS_JSON))

SDL_LIBS := -lSDL

# Targets
TARGETS := $(LIBRATJS) $(RATJS)

ifeq ($(ENABLE_SCRIPT),1)
	TARGETS += $(TEST262)
endif

# config.mk
CONFIG_MK := $(O)/config.mk
# Items in config.mk
CONFIG_MK_ITEMS := \
	ARCH\
	DEBUG NOASSERT\
	CROSS_COMPILE\
	CLANG\
	M\
	$(CONFIG_ITEMS)

# config.h
CONFIG_H := $(O)/include/ratjs/rjs_config.h
# Items in config.h
CONFIG_H_ITEMS += \
	$(CONFIG_ITEMS)\
	MAJOR_VERSION\
	MINOR_VERSION\
	MICRO_VERSION

# Compile a C file to object file
define compile_o =
OBJS += $$(patsubst %.c,$(O)/%.o,$(1))
DEPS += $$(patsubst %.c,$(O)/%.d,$(1))
$$(patsubst %.c,$(O)/%.o,$(1)): $(1)
	$$(info CC $$< -> $$@)
	$(Q)$(MKDIR) $$(dir $$@)
	$(Q)$(CC) -c -o $$@ $$< $(CFLAGS) -MMD -MF $$(patsubst %.c,$(O)/%.d,$(1))
endef

# Compile C source files
define compile_srcs =
$$(foreach c,$(1),$$(eval $$(call compile_o,$$c)))
endef

# Create static library
define build_slib =
$(O)/$(1)$(SLIB_SUFFIX): $$(patsubst %.c,$(O)/%.o,$(2))
	$$(info AR $$^ -> $$@)
	$(Q)$(AR) rcs $$@ $$^
	$(Q)$(RANLIB) $$@
endef

# Link dynamic library
ifneq ($(STATIC_LIBRARY_ONLY),1)
ifeq ($(ARCH),win)
define build_dlib =
$(O)/$(1)$(DLIB_SUFFIX): $$(patsubst %.c,$(O)/%.o,$(2))
	$$(info CC $$^ -> $$@)
	$(Q)$(CC) -o $$@ $$^ -shared $(LIBS) -Wl,--out-implib,$(O)/$(1)$(DLIB_SUFFIX).a
	$(Q)$(RANLIB) $(O)/$(1)$(DLIB_SUFFIX).a
endef
else
define build_dlib =
$(O)/$(1)$(DLIB_SUFFIX): $$(patsubst %.c,$(O)/%.o,$(2))
	$$(info CC $$^ -> $$@)
	$(Q)$(CC) -o $$@ $$^ -shared $(LIBS) -Wl,-soname,$(1)$(DLIB_SUFFIX).$(SO_MAJOR_VERSION)
endef
endif
else
build_dlib =
endif

# Build library
ifneq ($(OPTIMIZE_FOR_SIZE),1)
define build_lib =
$(eval $(call compile_srcs,$(2)))
$(eval $(call build_slib,$(1),$(2)))
$(eval $(call build_dlib,$(1),$(2)))
endef
else
define build_lib =
src/lib/$(1)_opt.c: $(2)
	$(Q)cat $(2) > src/lib/$(1)_opt.c
.PHONY: src/lib/$(1)_opt.c
$(O)/$(1).o: src/lib/$(1)_opt.c
	$$(info CC $$^ -> $$@)
	$(Q)$(CC) -c -o $$@ src/lib/$(1)_opt.c $(CFLAGS) -MMD -MF $(O)/$(1).d
DEPS += $(O)/$(1).d
$(eval $(call build_slib,$(1),$(1).c))
$(eval $(call build_dlib,$(1),$(1).c))
endef
endif

# Build executable program
define build_exe =
$$(eval $$(call compile_srcs,$(2)))
$(O)/$(1)$(EXE_SUFFIX): $$(patsubst %.c,$(O)/%.o,$(2))
	$$(info CC $$^ -> $$@)
	$(Q)$(CC) -o $$@ $$(patsubst %.c,$(O)/%.o,$(2)) $(3) $(LIBS)
	$(Q)O=$(O) SO_MAJOR_VERSION=$(SO_MAJOR_VERSION) ./build/gen_exe_shell_$(ARCH).sh $(1)
endef

# Compile a C file to host object file
define compile_host_o =
DEPS += $$(patsubst %.c,$(O)/%.d,$(1))
$$(patsubst %.c,$(O)/%.ho,$(1)): $(1)
	$$(info HOST_CC $$< -> $$@)
	$(Q)$(MKDIR) $$(dir $$@)
	$(Q)$(HOST_CC) -c -o $$@ $$< $(HOST_CFLAGS) -MMD -MF $$(patsubst %.c,$(O)/%.d,$(1))
endef

# Compile C source files to host objects
define compile_host_srcs =
$$(foreach c,$(1),$$(eval $$(call compile_host_o,$$c)))
endef

# Build host executable program
define build_host_exe =
$$(eval $$(call compile_host_srcs,$(2)))
$(O)/gen/$(1)$(HOST_EXE_SUFFIX): $$(patsubst %.c,$(O)/%.ho,$(2))
	$$(info HOST_CC $$^ -> $$@)
	$(Q)$(HOST_CC) -o $$@ $$(patsubst %.c,$(O)/%.ho,$(2)) $(3) $(HOST_LIBS)
endef

# Save item to config.mk
define save_config_mk_item =
$(Q)echo $(2) ?= $($(2)) >> $(1);

endef

# Save config.mk
define save_config_mk =
$(Q)$(RM) $(1)
$(foreach i,$(CONFIG_MK_ITEMS),$(call save_config_mk_item,$(1),$i))
endef

# Save item to config.h
define save_config_h_item =
$(Q)echo \#define $(2) $(if $($(2)_C_VALUE),$($(2)_C_VALUE),$($(2))) >> $(1)

endef

# Save config.h
define save_config_h =
$(Q)$(RM) $(1)
$(foreach i,$(CONFIG_H_ITEMS),$(call save_config_h_item,$(1),$i))
endef

# Build njs module
define build_njs =
$(1): $$(patsubst %.njs,%.o,$(1))
	$$(info CC $$^ -> $$@)
	$(Q)$(CC) -o $$@ $$^ -shared $$($$(patsubst $(O)/njs/%.njs,%_LIBS,$(1))) $(LIBS)

$$(eval $$(call compile_o,$$(patsubst $(O)/%.njs,%.c,$(1))))

$$(patsubst $(O)/%.njs,%.c,$(1)): $$(patsubst $(O)/%.njs,%.json,$(1))
	$$(info GEN $$@)
	$(Q)mkdir -p $$(dir $$@)
	$(Q)ratjs -m module module/njsgen/njsgen.js -v -o $$@ $$^
endef

all: $(TARGETS)

# Generate config.mk and config.h
config $(CONFIG_H):
	$(info GEN config)
	$(Q)$(MKDIR) $(dir $(CONFIG_MK))
	$(call save_config_mk,$(CONFIG_MK))
	$(Q)$(MKDIR) $(dir $(CONFIG_H))
	$(call save_config_h,$(CONFIG_H))

# Generator: lextab
$(eval $(call build_host_exe,lextab-gen,$(LEXTAB_SRCS),))

# Generate lexical table
$(LEXTAB_OUTPUT): $(LEXTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(LEXTAB) > $@

$(LEXTAB_SRCS): $(CONFIG_H)

src/lib/rjs_lex.c: $(LEXTAB_OUTPUT)

# Token types
$(TOKENTYPE_OUTPUT): $(LEXTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(LEXTAB) -t > $@

# Generator: strtab
$(eval $(call build_host_exe,strtab-gen,$(STRTAB_SRCS),))

# Generate string table C file
$(STRTAB_C_OUTPUT): $(STRTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(STRTAB) -c > $@

src/lib/rjs_string.c: $(STRTAB_C_OUTPUT)

# Generate string table H file
$(STRTAB_H_OUTPUT): $(STRTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(STRTAB) -h > $@

# Generate string table function file
$(STRTAB_F_OUTPUT): $(STRTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(STRTAB) -f > $@

# Generator: objtab
$(eval $(call build_host_exe,objtab-gen,$(OBJTAB_SRCS),))

# Generate object table H file
$(OBJTAB_H_OUTPUT): $(OBJTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(OBJTAB) -h > $@

# Generate object table function file
$(OBJTAB_F_OUTPUT): $(OBJTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(OBJTAB) -f > $@

# Generate object table C file
$(OBJTAB_C_OUTPUT): $(OBJTAB)
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(OBJTAB) -c > $@

src/lib/rjs_builtin_func_object.c: $(OBJTAB_C_OUTPUT)

# Generator: ast
$(eval $(call build_host_exe,ast-gen,$(AST_SRCS),-ljson-c))

# Generate AST C file
$(AST_C_OUTPUT): $(AST) gen/ast/ast.json
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(AST) > $@

# Generate AST header file
$(AST_H_OUTPUT): $(AST) gen/ast/ast.json
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(AST) -h > $@

src/lib/rjs_parser.c: $(AST_C_OUTPUT)

# Generator: bc
$(eval $(call build_host_exe,bc-gen,$(BC_SRCS),-ljson-c))

# Generate byte code header file
$(BC_H_OUTPUT): $(BC) gen/bc/bc.json
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(BC) -h > $@

# Generate byte code C file
$(BC_C_OUTPUT): $(BC) gen/bc/bc.json
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(BC) > $@

# Generate byte code running C file
$(BC_R_OUTPUT): $(BC) gen/bc/bc.json
	$(info GEN $@)
	$(Q)$(MKDIR) $(dir $@)
	$(Q)$(BC) -r > $@

src/lib/rjs_bc_gen.c: $(BC_C_OUTPUT)

src/lib/rjs_bc_call.c: $(BC_R_OUTPUT)

# Library: libratjs
$(eval $(call build_lib,libratjs,$(LIBRATJS_SRCS)))

# Program: ratjs
$(RATJS): $(LIBRATJS)

$(eval $(call build_exe,ratjs,$(RATJS_SRCS),-L$(O) -lratjs))

# Program: test262
$(TEST262): $(LIBRATJS)
$(eval $(call build_exe,test262,$(TEST262_SRCS),-L$(O) -lratjs -lyaml -lpthread))

$(OBJS) $(HOST_OBJS): $(TOKENTYPE_OUTPUT) $(STRTAB_H_OUTPUT) $(STRTAB_F_OUTPUT) $(SYMTAB_H_OUTPUT) $(SYMTAB_F_OUTPUT) $(OBJTAB_H_OUTPUT) $(OBJTAB_F_OUTPUT) $(AST_H_OUTPUT) $(BC_H_OUTPUT) $(CONFIG_H)

-include $(DEPS)

demo:
	$(Q)for dir in $(wildcard demo/*); do\
		make -C $$dir;\
	done

demo-clean:
	$(Q)for dir in $(wildcard demo/*); do\
		make -C $$dir clean;\
	done

njs: $(NJS_MODULES)

$(foreach mod,$(NJS_MODULES),$(eval $(call build_njs,$(mod))))

install: uninstall
	$(info INSTALL)
	$(Q)install -m 644 $(LIBRATJS_SLIB) $(INSTALL_PREFIX)/lib
	$(Q)O=$(O) SO_MAJOR_VERSION=$(SO_MAJOR_VERSION) SO_MINOR_VERSION=$(SO_MINOR_VERSION) SO_MICRO_VERSION=$(SO_MICRO_VERSION) INSTALL_PREFIX=$(INSTALL_PREFIX) ./build/install_dlib_$(ARCH).sh
	$(Q)install -m 755 -s $(RATJS) $(INSTALL_PREFIX)/bin
	$(Q)install -m 644 include/ratjs.h $(INSTALL_PREFIX)/include
	$(Q)mkdir -m 755 $(INSTALL_PREFIX)/include/ratjs
	$(Q)install -m 644 include/ratjs/*.h $(INSTALL_PREFIX)/include/ratjs
	$(Q)install -m 644 $(O)/include/ratjs/rjs_config.h $(INSTALL_PREFIX)/include/ratjs

uninstall:
	$(info UNINSTALL)
	$(Q)$(RM) $(INSTALL_PREFIX)/lib/libratjs$(SLIB_SUFFIX)
	$(Q)SO_MAJOR_VERSION=$(SO_MAJOR_VERSION) SO_MINOR_VERSION=$(SO_MINOR_VERSION) SO_MICRO_VERSION=$(SO_MICRO_VERSION) INSTALL_PREFIX=$(INSTALL_PREFIX) ./build/install_dlib_$(ARCH).sh -u
	$(Q)$(RM) $(INSTALL_PREFIX)/bin/ratjs$(EXE_SUFFIX)
	$(Q)$(RMDIR) $(INSTALL_PREFIX)/include/ratjs
	$(Q)$(RM) $(INSTALL_PREFIX)/include/ratjs.h

clean:
	$(info CLEAN $(TARGETS) $(OBJS))
	$(Q)$(RM) $(TARGETS) $(OBJS)

dist-clean:
	$(info DIST CLEAN)
	$(Q)$(RMDIR) $(O)

help:
	$(info options:)
	$(info $(NULL)  O=DIR)
	$(info $(NULL)        set output directory)
	$(info $(NULL)  ARCH=linux|win)
	$(info $(NULL)        set the target architecture, default=linux)
	$(info $(NULL)  DEBUG=1|0)
	$(info $(NULL)        enable/disable debug, default=0)
	$(info $(NULL)  NOASSERT=1|0)
	$(info $(NULL)        disable/enable assert, default=0)
	$(info $(NULL)  CROSS_COMPILE=PREFIX)
	$(info $(NULL)        set the cross compile toolchain's prefix)
	$(info $(NULL)  CLANG=1|0)
	$(info $(NULL)        use clang as the C compiler, default=0)
	$(info $(NULL)  M=32|64)
	$(info $(NULL)        generate code for 32-bit or 64-bit ABI)
	$(foreach c,$(CONFIG_ITEMS),$(info $(NULL)  $(c)=$($(c)_ENUM)) $(info $(NULL)        $($(c)_DESC), default=$($(c)_DEFAULT)))

.PHONY: all clean dist-clean help config demo install uninstall
