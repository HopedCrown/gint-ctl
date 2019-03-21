#! /usr/bin/make -f
#  Makefile for the gint control add-in
#---

#
#  Configuration
#

# Compiler flags
cf        := -mb -ffreestanding -nostdlib -Wall -Wextra -std=c11 -Os \
             -fstrict-volatile-bitfields
cf-fx     := $(cf) -m3 -DFX9860G
cf-cg     := $(cf) -m4-nofpu -DFXCG50

# Linker flags
lf-fx     := -Tfx9860g.ld -lgint-fx -lgcc -Wl,-Map=build.fx/map
lf-cg     := -Tfxcg50.ld  -lgint-cg -lgcc -Wl,-Map=build.cg/map -L. -lfxcg

dflags     = -MMD -MT $@ -MF $(@:.o=.d) -MP
cpflags   := -R .bss -R .gint_bss

g1af      := -i icon-fx.png -n gintctl --internal=@GINTCTL
g3af      := -n basic:" " -i uns:icon-cg-uns.png -i sel:icon-cg-sel.png

#
#  File listings
#

elf        = $(dir $<)gintctl.elf
bin        = $(dir $<)gintctl.bin
target-fx := gintctl.g1a
target-cg := gintctl.g3a

# Source and object files
src       := $(wildcard *.c)
res       := $(wildcard resources/*)
obj-fx    := $(src:%.c=build.fx/%.o) $(res:resources/%=build.fx/%.o)
obj-cg    := $(src:%.c=build.cg/%.o) $(res:resources/%=build.cg/%.o)

# Additional dependencies
deps-fx   := icon-fx.png
deps-cg   := icon-cg-uns.png icon-cg-sel.png

#
#  Build rules
#

all: all-fx all-cg

all-fx: $(target-fx)
all-cg: $(target-cg)

$(target-fx): $(obj-fx) $(deps-fx)

	sh3eb-elf-gcc -o $(elf) $(obj-fx) $(cf-fx) $(lf-fx)
	sh3eb-elf-objcopy -O binary $(cpflags) $(elf) $(bin)
	fxg1a $(bin) -o $@ $(g1af)

$(target-cg): $(obj-cg) $(deps-cg)

	sh4eb-elf-gcc -o $(elf) $(obj-cg) $(cf-cg) $(lf-cg)
	sh4eb-elf-objcopy -O binary $(cpflags) $(elf) $(bin)
	mkg3a $(g3af) $(bin) $@

# C sources
build.fx/%.o: %.c | build.fx/
	sh3eb-elf-gcc -c $< -o $@ $(cf-fx) $(dflags)
build.cg/%.o: %.c | build.cg/
	sh4eb-elf-gcc -c $< -o $@ $(cf-cg) $(dflags)

# Images
build.fx/%.png.o: resources/%.png | build.fx/
	fxconv -i $< -o $@ name:$*
build.cg/%.png.o: resources/%.png | build.cg/
	@ echo -e "\e[31;1mWARNING: conversion for fxcg50 not supported yet\e[0m"
	fxconv -i $< -o $@ name:$*

%/:
	@ mkdir -p $@

#
#  Cleaning and utilities
#

# Dependency information
-include $(shell find build* -name *.d 2> /dev/null)
build.fx/%.d: ;
build.cg/%.d: ;
.PRECIOUS: build.fx build.cg build.fx/%.d build.cg/%.d %/

clean:
	@ rm -rf build*
distclean: clean
	@ rm -f $(target-fx) $(target-cg)

install-fx: $(target-fx)
	p7 send -f $<
install-cg: $(target-cg)
	@ while [[ ! -h /dev/Prizm1 ]]; do sleep 1; done
	@ mount /dev/Prizm1
	@ rm -f /mnt/prizm/$<
	@ cp $< /mnt/prizm
	@ umount /dev/Prizm1
	@- eject /dev/Prizm1

.PHONY: all all-fx all-cg clean distclean install-fx install-cg
