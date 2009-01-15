##############################################################################
# MAKEFILE FOR THE PEOPS OSS SPU... just run "make"
##############################################################################

##############################################################################
# 1. SETS (CCFLAGS3 is used)
##############################################################################

ifndef CPUTYPE
	CPUOPT = -mcpu=pentiumpro
else
	CPUOPT = -march=$(CPUTYPE)
endif

TARGET = cdrPeops
# DEBUG = 1

CC = gcc
RC = windres
LINK = dllwrap
STRIP = strip
CP = cp
RM = rm -f

CCFLAGS1 = -c -Wall -m486 -O3
CCFLAGS2 = -c -Wall -m486 -O2 -ffast-math
CCFLAGS3 = -c -Wall $(CPUOPT) -O3 -ffast-math -fomit-frame-pointer
CCFLAGS = $(CCFLAGS3) -DWIN32 -D_WINDOWS -D_MINGW -mwindows
RC1FLAGS = -D_MINGW
RC2FLAGS =  -J res -O coff
LINKFLAGS = -dllname $(TARGET).dll -def ../$(TARGET).def
LINKFLAGS += -Wl,--enable-stdcall-fixup -mwindows
LIB = -lwinmm -ldsound

ifndef DEBUG
	TARGETDIR = ./Release/
	CCFLAGS += -DNDEBUG
	RC1FLAGS += -DNDEBUG
else
	TARGETDIR = ./Debug/
	CCFLAGS += -D_DEBUG
	RC1FLAGS += -D_DEBUG
endif

RESOBJ = res.o
OBJ = stdafx.o cdrpeops.o ppf.o cfg.o generic.o scsi.o cdda.o toc.o sub.o ioctrl.o read.o cdr.o $(RESOBJ)

vpath %.o $(TARGETDIR)

############################################################################## 
# 2. MAIN RULE 
############################################################################## 

$(TARGET).dll :	$(OBJ)
	cd $(TARGETDIR) ; \
	$(LINK) $(LINKFLAGS) $(OBJ) $(LIB)
ifndef DEBUG
	cd $(TARGETDIR) ; \
	$(STRIP) $(TARGET).dll
endif

ifndef DEBUG
release : $(TARGET).dll
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

$(RESOBJ) : $(TARGET).rc
	$(RC) $< $(TARGETDIR)$*.res $(RC1FLAGS)
	$(RC) $(RC2FLAGS) -o $(TARGETDIR)$@ -i $(TARGETDIR)$*.res 

clean :
	-$(RM) $(TARGETDIR)*.o ; $(RM) $(TARGETDIR)*.res ; $(RM) $(TARGETDIR)$(TARGET).dll

stdafx.o : stdafx.c stdafx.h
cdrpeops.o : cdrpeops.c stdafx.h cdrPeops.h externals.h
ppf.o : ppf.c stdafx.h externals.h
cfg.o : cfg.c stdafx.h resource.h externals.h
generic.o : generic.c stdafx.h externals.h
scsi.o : scsi.c stdafx.h externals.h
cdda.o : cdda.c stdafx.h externals.h
toc.o : toc.c stdafx.h externals.h
sub.o : sub.c stdafx.h externals.h
ioctrl.o : ioctrl.c stdafx.h externals.h
read.o : read.c stdafx.h externals.h
cdr.o : cdr.c stdafx.h resource.h externals.h
