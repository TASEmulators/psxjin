

CC = gcc

VERSION = 1
BUILD = 4
PLUGIN = libcdriso-${VERSION}.${BUILD}.so
CFG = cfgCdrIso
CFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -I.. -I. -D__LINUX__
OBJECTS = ../cdriso.o Config.o Linux.o
CFGOBJS = conf.o interface.o support.o Config.o
LIBS = -lz -lbz2
CFGLIBS = $(shell gtk-config --libs) ${LIBS}
CFLAGS += $(shell gtk-config --cflags) -DVERSION=${VERSION} -DBUILD=${BUILD}

all: plugin cfg

plugin: ${OBJECTS}
	rm -f ${PLUGIN}
	${CC} -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJECTS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

cfg: ${CFGOBJS}
	rm -f ${CFG}
	${CC} ${CFLAGS} ${CFGOBJS} -o ${CFG} ${CFGLIBS}
	strip ${CFG}

clean: 
	rm -f ../*.o *.o *.so ${CFG}


# Dependencies

callbacks.o: callbacks.c callbacks.h interface.h support.h
interface.o: interface.c callbacks.h interface.h support.h
support.o: support.c support.h
