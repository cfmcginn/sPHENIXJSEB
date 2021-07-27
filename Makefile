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

CFLAGS += -DLINUX $(DEBFLAGS) -Wall
LFLAGS += -lpthread

TARGET = bin/sphenixADCTest
SRCS = src/sphenixADCTest.c 

LD = gcc

OD = ./
OBJS = $(addsuffix .o, $(addprefix $(OD)/, $(basename $(notdir $(SRCS)))))


all : mkdirBin mkdirLib $(TARGET)

mkdirBin:
	$(MKDIR_BIN)

mkdirLib:
	$(MKDIR_LIB)

$(TARGET) : $(OBJS)
	$(LD) -o $@ $(OBJS) $(LFLAGS) $(ADDITIONAL_LIBS)

sphenixADCTest.o : src/sphenixADCTest.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *~ 
	rm -f *# 
	rm -f src/*~ 
	rm -f src/*# 
	rm -rf bin
	rm -rf lib
	rm -f input/*~
	rm -f *.o
