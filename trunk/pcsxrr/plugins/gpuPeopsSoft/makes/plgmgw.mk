#
# plugin specific makefile
#

CPUTYPE = i686
PLUGIN = gpuPeopsSoft
#PLUGINTYPE = libgpu.so
ifndef CPUTYPE
	CPUOPT = -mcpu=pentiumpro
else
	CPUOPT = -march=$(CPUTYPE)
endif
CFLAGS = -g -Wall -O4 $(CPUOPT) -fomit-frame-pointer -ffast-math 
#CFLAGS = -g -Wall -O3 -mpentium -fomit-frame-pointer -ffast-math
#INCLUDE = -I/usr/local/include
# brcc32.exe uses INCLUDE enviroment variable.
OBJECTS = gpu.o cfg.o draw.o fps.o key.o menu.o prim.o soft.o zn.o hq3x32.o hq2x32.o hq3x16.o hq2x16.o
LIBS =
