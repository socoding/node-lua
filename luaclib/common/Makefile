
.PHONY: all clean
CC      = g++ 
RM      = rm -rf  
##PLATFORM ?= linux

TARGET = libcommon.a
all: $(TARGET)

## source file path  
SRC_PATH := ../../src

## used headers  file path  
INCLUDE_PATH := ../../src ../../deps/lua ../../deps/uv/include  

## define CFLAGS
CFLAGS += -g -O3 -Wall -Wextra -Wno-unused-parameter -fpic -std=c++0x -D LUA_COMPAT_5_2 -D LUA_COMPAT_5_1 

ifeq (RELEASE,$(RELEASE))
CFLAGS += -D RELEASE
endif

## get all include path  
CFLAGS += $(foreach dir, $(INCLUDE_PATH), -I$(dir))  

buffer.o : $(SRC_PATH)/buffer.cpp 
	$(CC) -c $(CFLAGS) -o $@ $< 

OBJS := buffer.o

$(TARGET): $(OBJS) 
	ar cr $@ $(OBJS)

clean:
	$(RM) *.o $(TARGET)
