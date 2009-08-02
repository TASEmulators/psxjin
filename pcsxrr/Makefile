#
# PCSX Makefile for Win32-mingw
#

#DEBUG = 1

MAJ = 1
MIN = 5
VERSION = ${MAJ}.${MIN}

all: pcsx

CPU = ix86
CPUTYPE = i686
# CPUTYPE = k6-2

CC = gcc
NASM = nasm
RM = rm -f
ifndef DEBUG
	STRIP = strip pcsx.exe
endif
RC = windres

ifndef CPUTYPE
	CPUOPT = -mcpu=pentiumpro
else
	CPUOPT = -march=${CPUTYPE}
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

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math
ifndef DEBUG
	FLAGS = -D__WIN32__ -D__MINGW32__ -DPCSX_VERSION=\"${VERSION}\"
else
	FLAGS = -D__WIN32__ -D__MINGW32__ -DPCSX_VERSION=\"${VERSION}\" -ggdb
endif

#FLAGS+= -fprofile-generate
#FLAGS+= -fprofile-use

RC1FLAGS = -d__MINGW32__
LIBS = -lz -lcomctl32 -llua51
RESOBJ = Win32/pcsxres.o
OBJS = PsxBios.o Gte.o CdRom.o PsxCounters.o PsxDma.o \
       DisR3000A.o Spu.o Sio.o PsxHw.o Mdec.o PsxMem.o Misc.o \
       plugins.o Decode_XA.o R3000A.o PsxInterpreter.o \
       PsxHLE.o Movie.o Cheat.o LuaEngine.o
OBJS+= Win32/WndMain.o Win32/Plugin.o Win32/ConfigurePlugins.o \
       Win32/AboutDlg.o Win32/memwatch.o Win32/memsearch.o \
       Win32/memcheat.o Win32/maphkeys.o Win32/movie.o ${RESOBJ}

ifeq (${CPU}, ix86)
	CC = gcc
ifndef DEBUG
	OPTIMIZE = -O4 -fomit-frame-pointer -finline-functions -ffast-math -fno-exceptions
else
	OPTIMIZE = -O1 -fomit-frame-pointer -finline-functions -ffast-math -fno-exceptions
endif
	OPTIMIZE += ${CPUOPT}
	OBJS+= ix86/iR3000A.o ix86/ix86.o
	FLAGS+= -D__i386__
endif
CFLAGS = -Wall ${OPTIMIZE} -IWin32 -I. ${FLAGS} -mwindows -mno-cygwin

ASMFLAGS = -f elf ${FLAGS} -iWin32 -i.

pcsx: ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o pcsx.exe ${LIBS}
	${STRIP}

.PHONY: clean pcsx

clean:
	-${RM} Win32/*.o
	-${RM} Win32/*.res
	-${RM} *.o
	-${RM} ${CPU}/*.o
	-${RM} pcsx.exe

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

${CPU}/%.o: ${CPU}/%.asm
	${NASM} ${ASMFLAGS} -o $@ $<

Win32/%.o: Win32/%.c
	${CC} ${CFLAGS} -c -o $@ $<

Cpu/ix86/%.o: Cpu/ix86/%.c
	${CC} ${CFLAGS} -c -o $@ $<

${RESOBJ}: Win32/pcsx.rc
	${RC} $< $*.res
	${RC} $*.res $@
