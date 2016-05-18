.PHONY: all clean
CC      = gcc 
RM      = rm -rf  
##PLATFORM ?= linux

TARGET = protobuf.so
all: $(TARGET)

## source file path  
SRC_PATH := .

## used headers  file path  
INCLUDE_PATH := ../../deps/lua src 

## used include librarys file path  
LIBRARY_PATH := ../../deps/lua

## need libs, add at here
LIBS := nlua

## define CFLAGS
CFLAGS += -g -O3 -Wall -Wextra -Wno-unused-parameter -fpic -D LUA_COMPAT_5_2 -D LUA_COMPAT_5_1 
ifeq (RELEASE,$(RELEASE))
CFLAGS += -D RELEASE
endif

## define LDFLAGS
LDFLAGS += -shared

ifeq (darwin,$(PLATFORM))
LDFLAGS += -undefined dynamic_lookup \
		   -framework Foundation \
           -framework CoreServices \
           -framework ApplicationServices
endif

ifeq (linux,$(PLATFORM))
LIBS += dl
endif


## get all source files  
SRCS += $(wildcard $(SRC_PATH)/*.c)

## all .o based on all .c
## OBJS := $(SRCS:.c=.o)
OBJS = binding/lua/pbc-lua.o src/context.o src/varint.o src/array.o src/pattern.o src/register.o src/proto.o src/map.o src/alloc.o src/rmessage.o src/wmessage.o src/bootstrap.o src/stringpool.o src/decode.o

## get all include path  
CFLAGS += $(foreach dir, $(INCLUDE_PATH), -I$(dir))  
.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

## get all library path  
LDFLAGS += $(foreach lib, $(LIBRARY_PATH), -L$(lib))  
## get all librarys  
LDFLAGS += $(foreach lib, $(LIBS), -l$(lib))  
$(TARGET): $(OBJS) 
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) *.o src/*.o binding/lua/*.o $(TARGET) 
