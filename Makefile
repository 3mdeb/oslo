#
# Makefile for the OSLO package.
#

ifeq ($(DEBUG),)
CCFLAGS += -Os -DNDEBUG
else
CCFLAGS += -g
endif

CCFLAGS   += -std=gnu99 -mregparm=3 -Iinclude/ -W -Wall -ffunction-sections -fstrict-aliasing -fomit-frame-pointer -minline-all-stringops -Winline
OBJ = asm.o util.o tis.o tpm.o sha.o elf.o mp.o


CC=gcc
VERBOSE = @


.PHONY: all
all: oslo beirut munich


oslo: osl.ld $(OBJ) osl.o
	$(LD) --section-start .slheader=0x00400000 -gc-sections -N -o $@ -T $^

beirut: beirut.ld $(OBJ) beirut.o
	$(LD) -Ttext=0x00410000 -gc-sections -N -o $@ -T $^

munich: beirut.ld $(OBJ) boot_linux.o munich.o
	$(LD) -Ttext=0x0040a000 -gc-sections -N -o $@ -T $^


util.o:  include/asm.h include/util.h 
sha.o:   include/asm.h include/util.h include/sha.h
tis.o:   include/asm.h include/util.h include/tis.h
tpm.o:   include/asm.h include/util.h include/tis.h include/tpm.h
mp.o:    include/asm.h include/util.h include/mp.h
osl.o:   include/asm.h include/util.h include/sha.h \
	 include/elf.h include/tis.h  include/tpm.h \
	 include/mbi.h include/version.h include/osl.h
beirut.o: include/asm.h include/util.h include/sha.h \
          include/elf.h include/tis.h   include/tpm.h \
          include/mbi.h
munich.o: include/asm.h include/util.h include/version.h \
          include/boot_linux.h include/mbi.h include/munich.h


.PHONY: clean
clean:
	$(VERBOSE) rm -f oslo beirut munich $(OBJ) osl.o beirut.o munich.o

%.o: %.c
	$(VERBOSE) $(CC) $(CCFLAGS) -c $<
%.o: %.S
	$(VERBOSE) $(CC) $(CCFLAGS) -c $<
