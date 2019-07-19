ifeq ($(DEBUG),)
CCFLAGS += -Os -DNDEBUG
else
CCFLAGS += -g
endif

CCFLAGS += -std=gnu99 -mregparm=3 -Iinclude/ -W -Wall -ffunction-sections -fstrict-aliasing -fomit-frame-pointer -minline-all-stringops -Winline
OBJS    =  asm.o util.o osl.o tis.o tpm.o sha.o

CC=gcc
VERBOSE = @


oslo: osl.ld $(OBJS)
	$(LD) -gc-sections -N -o $@ -T $^

util.o:  include/asm.h include/util.h 
sha.o:   include/asm.h include/util.h include/sha.h
tis.o:   include/asm.h include/util.h include/tis.h
tpm.o:   include/asm.h include/util.h include/tis.h include/tpm.h
osl.o:   include/asm.h include/util.h include/sha.h \
	 include/elf.h include/tis.h  include/tpm.h \
	 include/mbi.h include/osl.h


.PHONY: clean
clean:
	$(VERBOSE) rm -f oslo $(OBJS)

%.o: %.c
	$(VERBOSE) $(CC) $(CCFLAGS) -c $<
%.o: %.S
	$(VERBOSE) $(CC) $(CCFLAGS) -c $<
