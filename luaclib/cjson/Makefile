##### Available defines for CJSON_CFLAGS #####
##
## USE_INTERNAL_ISINF:      Workaround for Solaris platforms missing isinf().
## DISABLE_INVALID_NUMBERS: Permanently disable invalid JSON numbers:
##                          NaN, Infinity, hex.
##
## Optional built-in number conversion uses the following defines:
## USE_INTERNAL_FPCONV:     Use builtin strtod/dtoa for numeric conversions.
## IEEE_BIG_ENDIAN:         Required on big endian architectures.
## MULTIPLE_THREADS:        Must be set when Lua CJSON may be used in a
##                          multi-threaded application. Requries _pthreads_.

.PHONY: all clean
CC      = gcc 
RM      = rm -rf  
##PLATFORM ?= linux

TARGET = cjson.so
all: $(TARGET)

## source file path  
SRC_PATH := .

## used headers  file path  
INCLUDE_PATH := ../../deps/lua  

## used include librarys file path  
LIBRARY_PATH := ../../deps/lua

## need libs, add at here
LIBS := nlua

## define CFLAGS
CFLAGS += -g -O3 -Wall -Wextra -Wno-unused-parameter -fpic -D LUA_COMPAT_5_2 -D LUA_COMPAT_5_1 
ifeq (RELEASE,$(RELEASE))
CFLAGS += -D RELEASE
endif

ifeq (sunos,$(PLATFORM))
CFLAGS += -D USE_INTERNAL_ISINF
endif

##### Number conversion configuration #####

## Use Libc support for number conversion (default)
FPCONV_OBJS = fpconv.o

## Use built in number conversion
#FPCONV_OBJS = g_fmt.o dtoa.o
#CFLAGS += -DUSE_INTERNAL_FPCONV

## Compile built in number conversion for big endian architectures
#CFLAGS += -DIEEE_BIG_ENDIAN


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
OBJS = lua_cjson.o strbuf.o $(FPCONV_OBJS)

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
	$(RM) *.o $(TARGET)
