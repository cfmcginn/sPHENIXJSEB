# Make sure that WD_BASEDIR points to the right place
ifeq ($(WD_BASEDIR),)
        WD_BASEDIR=/home/phnxrc/export/software/WinVer1260/WinDriver
endif

WD_BASEDIR:=$(wildcard $(WD_BASEDIR))

#ifeq ($(wildcard $(WD_BASEDIR)/include/windrvr.h),)
#        $(error Please edit the makefile and set the WD_BASEDIR variable \
#        to point to the location of your WinDriver installation directory)
#endif


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
MKDIR_OBJ=mkdir -p $(PWD)/obj

INCLUDE=-I$(PWD)
LIB=-L$(PWD)/lib

CFLAGS += -DLINUX $(DEBFLAGS) -Wall -I$(PWD) -I$(WD_BASEDIR)/include -I$(WD_BASEDIR) 
CFLAGS += -DWD_DRIVER_NAME_CHANGE
LFLAGS += -lwdapi1260
LFLAGS += -lpthread

TARGET = bin/sphenixADCTest
SRCS = src/sphenixADCTest.c src/jseb2_lib.c $(WD_BASEDIR)/samples/shared/diag_lib.c $(WD_BASEDIR)/samples/shared/wdc_diag_lib.c

LD = gcc

OD = ./
OBJS = $(addsuffix .o, $(addprefix $(OD)/, $(basename $(notdir $(SRCS)))))

ROOT=`root-config --cflags --glibs`

all : mkdirBin mkdirObj mkdirLib obj/checkMakeDir.o obj/globalDebugHandler.o lib/libSPHENIXADCHelper.so bin/sphenixTokenPassing.exe $(TARGET) 

mkdirBin:
	$(MKDIR_BIN)

mkdirObj:
	$(MKDIR_OBJ)

mkdirLib:
	$(MKDIR_LIB)

$(TARGET) : $(OBJS)
	$(LD) -o $@ $(OBJS) $(LFLAGS) $(ADDITIONAL_LIBS)

sphenixADCTest.o : src/sphenixADCTest.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

jseb2_lib.o : src/jseb2_lib.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

diag_lib.o : $(WD_BASEDIR)/samples/shared/diag_lib.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

wdc_diag_lib.o : $(WD_BASEDIR)/samples/shared/wdc_diag_lib.c
	$(CC) -std=c99 -c $(CFLAGS) -o $@ $<

obj/checkMakeDir.o: src/checkMakeDir.C
	$(CXX) $(CXXFLAGS) -fPIC -c src/checkMakeDir.C -o obj/checkMakeDir.o $(INCLUDE)

obj/globalDebugHandler.o: src/globalDebugHandler.C
	$(CXX) $(CXXFLAGS) -fPIC -c src/globalDebugHandler.C -o obj/globalDebugHandler.o $(ROOT) $(INCLUDE)

lib/libSPHENIXADCHelper.so:
	$(CXX) $(CXXFLAGS) -fPIC -shared -o lib/libSPHENIXADCHelper.so obj/checkMakeDir.o obj/globalDebugHandler.o $(ROOT) $(INCLUDE)

bin/sphenixTokenPassing.exe: src/sphenixTokenPassing.C
	$(CXX) $(CXXFLAGS) src/sphenixTokenPassing.C -o bin/sphenixTokenPassing.exe $(ROOT) $(INCLUDE) $(LIB) -lSPHENIXADCHelper


clean:
	rm -f *~ 
	rm -f *# 
	rm -f src/*~ 
	rm -f src/*# 
	rm -rf bin
	rm -rf lib
	rm -f input/*~
	rm -f *.o
