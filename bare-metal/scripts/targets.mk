################################################################################
# targets.mk: Parse and generate targets from each 'Kbuild/Makefile'
#
# Description: This file is from 'scripts/Makefile.lib'.
# 
################################################################################
# In user-defined 'Kbuild' or 'Makefile' under each subdirectories, only the
# following variables should be used:
#
# targets:
# * obj-y, obj-m, obj-: for target objects and subdirectories
# * lib-y, lib-m, lib-: for target libraries
# * always, targets: for target objects alwlays built
# * subdir-y, subdir-m, subdir-: for target subdirectories
#
# flags:
# * asflags-y, ccflags-y, cppflags-y, ldflags-y: flags only for all the targets
#   under this directory
# * subdir-asflags-y, subdir-ccflags-y: flags for all the targets and be
#	be inherited by subdirectories
#


# Figure out what we need to build from the various variables
################################################################################
# We clarify the targets here:
#
# * subdir-ym: the subdirectories listed in 'obj-y' or 'obj-m'
# * obj-y: 
#

# When an object is listed to be built compiled-in and modular,
# only build the compiled-in version

obj-m := $(filter-out $(obj-y),$(obj-m))

# Libraries are always collected in one lib file.
# Filter out objects already built-in

lib-y := $(filter-out $(obj-y), $(sort $(lib-y) $(lib-m)))


# Handle objects in subdirs
# ---------------------------------------------------------------------------
# o if we encounter foo/ in $(obj-y), replace it by foo/built-in.o
#   and add the directory to the list of dirs to descend into: $(subdir-y)
# o if we encounter foo/ in $(obj-m), remove it from $(obj-m)
#   and add the directory to the list of dirs to descend into: $(subdir-m)

# Determine modorder.
# Unfortunately, we don't have information about ordering between -y
# and -m subdirs.  Just put -y's first.
modorder	:= $(patsubst %/,%/modules.order, $(filter %/, $(obj-y)) $(obj-m:.o=.ko))

__subdir-y	:= $(patsubst %/,%,$(filter %/, $(obj-y)))
subdir-y	+= $(__subdir-y)
__subdir-m	:= $(patsubst %/,%,$(filter %/, $(obj-m)))
subdir-m	+= $(__subdir-m)
obj-y		:= $(patsubst %/, %/built-in.o, $(obj-y))
obj-m		:= $(filter-out %/, $(obj-m))

# Subdirectories we need to descend into

subdir-ym	:= $(sort $(subdir-y) $(subdir-m))

# if $(foo-objs) exists, foo.o is a composite object
multi-used-y := $(sort $(foreach m,$(obj-y), $(if $(strip $($(m:.o=-objs)) $($(m:.o=-y))), $(m))))
multi-used-m := $(sort $(foreach m,$(obj-m), $(if $(strip $($(m:.o=-objs)) $($(m:.o=-y)) $($(m:.o=-m))), $(m))))
multi-used   := $(multi-used-y) $(multi-used-m)
single-used-m := $(sort $(filter-out $(multi-used-m),$(obj-m)))

# Build list of the parts of our composite objects, our composite
# objects depend on those (obviously)
multi-objs-y := $(foreach m, $(multi-used-y), $($(m:.o=-objs)) $($(m:.o=-y)))
multi-objs-m := $(foreach m, $(multi-used-m), $($(m:.o=-objs)) $($(m:.o=-y)))
multi-objs   := $(multi-objs-y) $(multi-objs-m)

# $(subdir-obj-y) is the list of objects in $(obj-y) which uses dir/ to
# tell kbuild to descend
subdir-obj-y := $(filter %/built-in.o, $(obj-y))

# $(obj-dirs) is a list of directories that contain object files
obj-dirs := $(dir $(multi-objs) $(obj-y))

# Replace multi-part objects by their individual parts, look at local dir only
real-objs-y := $(foreach m, $(filter-out $(subdir-obj-y), $(obj-y)), $(if $(strip $($(m:.o=-objs)) $($(m:.o=-y))),$($(m:.o=-objs)) $($(m:.o=-y)),$(m))) $(extra-y)
real-objs-m := $(foreach m, $(obj-m), $(if $(strip $($(m:.o=-objs)) $($(m:.o=-y)) $($(m:.o=-m))),$($(m:.o=-objs)) $($(m:.o=-y)) $($(m:.o=-m)),$(m)))

# Add subdir path

extra-y		:= $(addprefix $(obj)/,$(extra-y))
always		:= $(addprefix $(obj)/,$(always))
targets		:= $(addprefix $(obj)/,$(targets))
modorder	:= $(addprefix $(obj)/,$(modorder))
obj-y		:= $(addprefix $(obj)/,$(obj-y))
obj-m		:= $(addprefix $(obj)/,$(obj-m))
lib-y		:= $(addprefix $(obj)/,$(lib-y))
subdir-obj-y	:= $(addprefix $(obj)/,$(subdir-obj-y))
real-objs-y	:= $(addprefix $(obj)/,$(real-objs-y))
real-objs-m	:= $(addprefix $(obj)/,$(real-objs-m))
single-used-m	:= $(addprefix $(obj)/,$(single-used-m))
multi-used-y	:= $(addprefix $(obj)/,$(multi-used-y))
multi-used-m	:= $(addprefix $(obj)/,$(multi-used-m))
multi-objs-y	:= $(addprefix $(obj)/,$(multi-objs-y))
multi-objs-m	:= $(addprefix $(obj)/,$(multi-objs-m))
subdir-ym	:= $(addprefix $(obj)/,$(subdir-ym))
obj-dirs	:= $(addprefix $(obj)/,$(obj-dirs))


# Prepare Build Flags
################################################################################
# flags that take effect in sub directories
export KBUILD_SUBDIR_ASFLAGS := $(KBUILD_SUBDIR_ASFLAGS) $(subdir-asflags-y)
export KBUILD_SUBDIR_CCFLAGS := $(KBUILD_SUBDIR_CCFLAGS) $(subdir-ccflags-y)

# These flags are needed for modversions and compiling, so we define them here
# already
# $(modname_flags) #defines KBUILD_MODNAME as the name of the module it will
# end up in (or would, if it gets compiled in)
# Note: Files that end up in two or more modules are compiled without the
#       KBUILD_MODNAME definition. The reason is that any made-up name would
#       differ in different configs.
name-fix = $(squote)$(quote)$(subst $(comma),_,$(subst -,_,$1))$(quote)$(squote)
basename_flags = -DKBUILD_BASENAME=$(call name-fix,$(basetarget))
modname_flags  = $(if $(filter 1,$(words $(modname))),\
                 -DKBUILD_MODNAME=$(call name-fix,$(modname)))

orig_c_flags   = $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS) $(KBUILD_SUBDIR_CCFLAGS) \
                 $(ccflags-y) $(CFLAGS_$(basetarget).o)
_c_flags       = $(filter-out $(CFLAGS_REMOVE_$(basetarget).o), $(orig_c_flags))
orig_a_flags   = $(KBUILD_CPPFLAGS) $(KBUILD_AFLAGS) $(KBUILD_SUBDIR_ASFLAGS) \
                 $(asflags-y) $(AFLAGS_$(basetarget).o)
_a_flags       = $(filter-out $(AFLAGS_REMOVE_$(basetarget).o), $(orig_a_flags))
_cpp_flags     = $(KBUILD_CPPFLAGS) $(cppflags-y) $(CPPFLAGS_$(@F))

# If building the kernel in a separate objtree expand all occurrences
# of -Idir to -I$(srctree)/dir except for absolute paths (starting with '/').

ifeq ($(KBUILD_SRC),)
__c_flags	= $(_c_flags)
__a_flags	= $(_a_flags)
__cpp_flags     = $(_cpp_flags)
else

# -I$(obj) locates generated .h files
# $(call addtree,-I$(obj)) locates .h files in srctree, from generated .c files
#   and locates generated .h files
# FIXME: Replace both with specific CFLAGS* statements in the makefiles
__c_flags	= $(if $(obj),-I$(srctree)/$(src) -I$(obj)) \
		  $(call flags,_c_flags)
__a_flags	= $(call flags,_a_flags)
__cpp_flags     = $(call flags,_cpp_flags)
endif

c_flags        = -Wp,-MD,$(depfile) $(NOSTDINC_FLAGS) $(LINUXINCLUDE)     \
		 $(__c_flags) $(modkern_cflags)                           \
		 $(basename_flags) $(modname_flags)

a_flags        = -Wp,-MD,$(depfile) $(NOSTDINC_FLAGS) $(LINUXINCLUDE)     \
		 $(__a_flags) $(modkern_aflags)

cpp_flags      = -Wp,-MD,$(depfile) $(NOSTDINC_FLAGS) $(LINUXINCLUDE)     \
		 $(__cpp_flags)

ld_flags       = $(LDFLAGS) $(ldflags-y)

dtc_cpp_flags  = -Wp,-MD,$(depfile).pre.tmp -nostdinc                    \
		 -I$(srctree)/arch/$(SRCARCH)/boot/dts                   \
		 -I$(srctree)/arch/$(SRCARCH)/boot/dts/include           \
		 -I$(srctree)/drivers/of/testcase-data                   \
		 -undef -D__DTS__

# Finds the multi-part object the current object will be linked into
modname-multi = $(sort $(foreach m,$(multi-used),\
		$(if $(filter $(subst $(obj)/,,$*.o), $($(m:.o=-objs)) $($(m:.o=-y))),$(m:.o=))))

# Useful for describing the dependency of composite objects
# Usage:
#   $(call multi_depend, multi_used_targets, suffix_to_remove, suffix_to_add)
define multi_depend
$(foreach m, $(notdir $1), \
	$(eval $(obj)/$m: \
	$(addprefix $(obj)/, $(foreach s, $3, $($(m:%$(strip $2)=%$(s)))))))
endef

# Commands useful for building a boot image
# ===========================================================================
#
#	Use as following:
#
#	target: source(s) FORCE
#		$(if_changed,ld/objcopy/gzip)
#
#	and add target to extra-y so that we know we have to
#	read in the saved command line

# Linking
# ---------------------------------------------------------------------------

quiet_cmd_ld = LD      $@
cmd_ld = $(LD) $(LDFLAGS) $(ldflags-y) $(LDFLAGS_$(@F)) \
	       $(filter-out FORCE,$^) -o $@

# Objcopy
# ---------------------------------------------------------------------------

quiet_cmd_objcopy = OBJCOPY $@
cmd_objcopy = $(OBJCOPY) $(OBJCOPYFLAGS) $(OBJCOPYFLAGS_$(@F)) $< $@

# Gzip
# ---------------------------------------------------------------------------

quiet_cmd_gzip = GZIP    $@
cmd_gzip = (cat $(filter-out FORCE,$^) | gzip -n -f -9 > $@) || \
	(rm -f $@ ; false)

# DTC
# ---------------------------------------------------------------------------
DTC ?= $(objtree)/scripts/dtc/dtc

# Disable noisy checks by default
ifeq ($(KBUILD_ENABLE_EXTRA_GCC_CHECKS),)
DTC_FLAGS += -Wno-unit_address_vs_reg
endif

# Generate an assembly file to wrap the output of the device tree compiler
quiet_cmd_dt_S_dtb= DTB     $@
cmd_dt_S_dtb=						\
(							\
	echo '\#include <asm-generic/vmlinux.lds.h>'; 	\
	echo '.section .dtb.init.rodata,"a"';		\
	echo '.balign STRUCT_ALIGNMENT';		\
	echo '.global __dtb_$(subst -,_,$(*F))_begin';	\
	echo '__dtb_$(subst -,_,$(*F))_begin:';		\
	echo '.incbin "$<" ';				\
	echo '__dtb_$(subst -,_,$(*F))_end:';		\
	echo '.global __dtb_$(subst -,_,$(*F))_end';	\
	echo '.balign STRUCT_ALIGNMENT'; 		\
) > $@

$(obj)/%.dtb.S: $(obj)/%.dtb
	$(call cmd,dt_S_dtb)

quiet_cmd_dtc = DTC     $@
cmd_dtc = mkdir -p $(dir ${dtc-tmp}) ; \
	$(CPP) $(dtc_cpp_flags) -x assembler-with-cpp -o $(dtc-tmp) $< ; \
	$(DTC) -O dtb -o $@ -b 0 \
		-i $(dir $<) $(DTC_FLAGS) \
		-d $(depfile).dtc.tmp $(dtc-tmp) ; \
	cat $(depfile).pre.tmp $(depfile).dtc.tmp > $(depfile)

$(obj)/%.dtb: $(src)/%.dts FORCE
	$(call if_changed_dep,dtc)

dtc-tmp = $(subst $(comma),_,$(dot-target).dts.tmp)

# U-Boot mkimage
# ---------------------------------------------------------------------------

MKIMAGE := $(srctree)/scripts/mkuboot.sh

# SRCARCH just happens to match slightly more than ARCH (on sparc), so reduces
# the number of overrides in arch makefiles
UIMAGE_ARCH ?= $(SRCARCH)
UIMAGE_COMPRESSION ?= $(if $(2),$(2),none)
UIMAGE_OPTS-y ?=
UIMAGE_TYPE ?= kernel
UIMAGE_LOADADDR ?= arch_must_set_this
UIMAGE_ENTRYADDR ?= $(UIMAGE_LOADADDR)
UIMAGE_NAME ?= 'Linux-$(KERNELRELEASE)'
UIMAGE_IN ?= $<
UIMAGE_OUT ?= $@

quiet_cmd_uimage = UIMAGE  $(UIMAGE_OUT)
      cmd_uimage = $(CONFIG_SHELL) $(MKIMAGE) -A $(UIMAGE_ARCH) -O linux \
			-C $(UIMAGE_COMPRESSION) $(UIMAGE_OPTS-y) \
			-T $(UIMAGE_TYPE) \
			-a $(UIMAGE_LOADADDR) -e $(UIMAGE_ENTRYADDR) \
			-n $(UIMAGE_NAME) -d $(UIMAGE_IN) $(UIMAGE_OUT)


# ASM offsets
################################################################################

# Default sed regexp - multiline due to syntax constraints
#
# Use [:space:] because LLVM's integrated assembler inserts <tab> around
# the .ascii directive whereas GCC keeps the <space> as-is.
define sed-offsets
	's:^[[:space:]]*\.ascii[[:space:]]*"\(.*\)".*:\1:; \
	/^->/{s:->#\(.*\):/* \1 */:; \
	s:^->\([^ ]*\) [\$$#]*\([^ ]*\) \(.*\):#define \1 \2 /* \3 */:; \
	s:->::; p;}'
endef

# Use filechk to avoid rebuilds when a header changes, but the resulting file
# does not
define filechk_offsets
	(set -e; \
	 echo "#ifndef $2"; \
	 echo "#define $2"; \
	 echo "/*"; \
	 echo " * DO NOT MODIFY."; \
	 echo " *"; \
	 echo " * This file was generated by Kbuild"; \
	 echo " */"; \
	 echo ""; \
	 sed -ne $(sed-offsets); \
	 echo ""; \
	 echo "#endif" )
endef
