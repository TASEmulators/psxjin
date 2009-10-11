##############################################################################
# MAKEFILE FOR THE PEOPS DSOUND SPU... run "make -f makefile.mgw"
##############################################################################

##############################################################################
# 1. SETS (CCFLAGS3 is used)
##############################################################################

CPUTYPE = i686
ifndef CPUTYPE
	CPUOPT = -mcpu=pentiumpro
else
	CPUOPT = -march=$(CPUTYPE)
endif
# http://gcc.gnu.org/onlinedocs/gcc-3.1.1/gcc/i386-and-x86-64-Options.html
#  The choices for cpu-type are i386, i486, i586, i686, pentium,
#  pentium-mmx, pentiumpro, pentium2, pentium3, pentium4, k6, k6-2,
#  k6-3, athlon, athlon-tbird, athlon-4, athlon-xp and athlon-mp.
#
#  While picking a specific cpu-type will schedule things
#  appropriately for that particular chip, the compiler will not
#  generate any code that does not run on the i386 without the
#  -march=cpu-type option being used. i586 is equivalent to
#  pentium and i686 is equivalent to pentiumpro.

TARGET = spuPeopsDSound
#DEBUG = 1

CC = gcc
RC = windres
LINK = dllwrap
STRIP = strip
CP = cp
RM = rm -f

CCFLAGS1 = -c -Wall -mcpu=486 -O3
CCFLAGS2 = -c -Wall -mcpu=486 -O2 -ffast-math
CCFLAGS3 = -c -Wall $(CPUOPT) -O3 -ffast-math -fomit-frame-pointer
CCFLAGS = $(CCFLAGS3) -D_WINDOWS -D__MINGW32__ -mwindows
RC1FLAGS = -D__MINGW32__
RC2FLAGS =  -J res -O coff
LINKFLAGS = -dllname $(TARGET).dll -def ../spuPeopsSound.def
LINKFLAGS += -Wl,--enable-stdcall-fixup -mwindows
LIB = -lwinmm -ldsound

ifndef DEBUG
	TARGETDIR = ./Release/
	CCFLAGS += -DNDEBUG
	RC1FLAGS += -DNDEBUG
else
	TARGETDIR = ./Debug/
	CCFLAGS += -D_DEBUG -ggdb
	RC1FLAGS += -D_DEBUG
endif

RESOBJ = res.o
OBJ = spu.o cfg.o dma.o freeze.o psemu.o registers.o
OBJ += dsound.o xa.o spuPeopsSound.o record.o $(RESOBJ)

vpath %.o $(TARGETDIR)

############################################################################## 
# 2. MAIN RULE 
############################################################################## 

$(TARGET).dll :	$(OBJ)
		cd $(TARGETDIR); \
		$(LINK) $(LINKFLAGS) $(OBJ) $(LIB)
ifndef DEBUG
		cd $(TARGETDIR); \
		$(STRIP) $(TARGET).dll
endif

ifndef DEBUG
release :	$(TARGET).dll
		$(CP) $(TARGETDIR)$(TARGET).dll ../
endif

############################################################################## 
# 3. GENERAL RULES 
############################################################################## 

%.o     : %.c 
	$(CC) $(CCFLAGS) $< -o $(TARGETDIR)$@

############################################################################## 
# 4. SPECIFIC RULES 
############################################################################## 

spu.o  : spu.c stdafx.h externals.h cfg.h dsoundoss.h regs.h xa.c reverb.c adsr.c
cfg.o  : cfg.c stdafx.h externals.h
dma.o : dma.c stdafx.h externals.h
freeze.o : freeze.c stdafx.h externals.h registers.h spu.h regs.h
psemu.o : psemu.c stdafx.h externals.h regs.h dma.h
registers.o : registers.c stdafx.h externals.h registers.h regs.h reverb.h

dsound.o : dsound.c stdafx.h externals.h
xa.o : xa.c stdafx.h xa.h
spuPeopsSound.o : spuPeopsSound.c stdafx.h
record.o : record.c record.h stdafx.h externals.h

$(RESOBJ) : spuPeopsSound.rc
	$(RC) $< $(TARGETDIR)$*.res $(RC1FLAGS)
	$(RC) $(RC2FLAGS) -o $(TARGETDIR)$@ -i $(TARGETDIR)$*.res 

clean:
	-$(RM) $(TARGETDIR)*.o
	-$(RM) $(TARGETDIR)*.res
	-$(RM) $(TARGETDIR)$(TARGET).dll

