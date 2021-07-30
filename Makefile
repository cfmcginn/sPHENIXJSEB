# Make sure that WD_BASEDIR points to the right place
ifeq ($(WD_BASEDIR),)
        WD_BASEDIR=/home/phnxrc/export/software/WinVer1260/WinDriver
endif

WD_BASEDIR:=$(wildcard $(WD_BASEDIR))

ifeq ($(wildcard $(WD_BASEDIR)/include/windrvr.h),)
        $(error Please edit the makefile and set the WD_BASEDIR variable \
        to point to the location of your WinDriver installation directory)
endif



# Comment/uncomment to enable/disable debugging code
DEBUG = 1

ifeq ($(DEBUG),1)
    DEBFLAGS = -g -O -DDEBUG
else
    DEBFLAGS = -O2
endif

ifndef TARGET_CPU
	TARGET_CPU=$(shell uname -m | sed 's/i.86/i386/' | sed 's/ppc/PPC/' | sed 's/ia64/IA64/')
endif
ifeq ("$(TARGET_CPU)", "PPC")
	CFLAGS += -DPOWERPC
endif

ifeq ("$(TARGET_CPU)", "IA64")
	CFLAGS += -DKERNEL_64BIT
endif

ifeq ("$(TARGET_CPU)", "PPC64")
	CFLAGS += -DKERNEL_64BIT
	ifndef USER_BITS
		USER_BITS = 64
	endif
	CFLAGS += -m$(USER_BITS)
	LFLAGS += -m$(USER_BITS)
endif

ifeq ("$(TARGET_CPU)", "x86_64")
	CFLAGS += -DKERNEL_64BIT
	ifndef USER_BITS
		USER_BITS = 64
	endif
	CFLAGS += -m$(USER_BITS)
	LFLAGS += -m$(USER_BITS)
endif

MKDIR_BIN=mkdir -p $(PWD)/bin
MKDIR_LIB=mkdir -p $(PWD)/lib

CFLAGS += -DLINUX $(DEBFLAGS) -Wall -I$(PWD) -I$(WD_BASEDIR)/include -I$(WD_BASEDIR) 
CFLAGS += -DWD_DRIVER_NAME_CHANGE
LFLAGS += -lwdapi1260 -L$(PWD)/lib
LFLAGS += -lpthread

TARGET = bin/sphenixADCTest
SRCS = src/sphenixADCTest.c src/jseb2_lib.c $(WD_BASEDIR)/samples/shared/diag_lib.c $(WD_BASEDIR)/samples/shared/wdc_diag_lib.c

LD = gcc

OD = lib
OBJS = $(addsuffix .o, $(addprefix $(OD)/, $(basename $(notdir $(SRCS)))))


all : mkdirBin mkdirLib $(TARGET)

mkdirBin:
	$(MKDIR_BIN)

mkdirLib:
	$(MKDIR_LIB)

$(TARGET) : $(OBJS)
	$(LD) -o $@ $(OBJS) $(LFLAGS) $(ADDITIONAL_LIBS)

lib/sphenixADCTest.o : src/sphenixADCTest.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

lib/jseb2_lib.o : src/jseb2_lib.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

lib/diag_lib.o : $(WD_BASEDIR)/samples/shared/diag_lib.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

lib/wdc_diag_lib.o : $(WD_BASEDIR)/samples/shared/wdc_diag_lib.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<


clean:
	rm -f *~ 
	rm -f *# 
	rm -f src/*~ 
	rm -f src/*# 
	rm -rf bin
	rm -rf lib
	rm -f input/*~
	rm -f *.o
