.PHONY: all clean
CPP     = g++  
RM      = rm -rf  

##PLATFORM ?= linux

TARGET = node-lua
all: $(TARGET)

## source file path  
SRC_PATH := .

## used headers  file path  
INCLUDE_PATH := ../deps/lua ../deps/uv/include  
  
## used include librarys file path  
LIBRARY_PATH := ../deps/lua ../deps/uv

## need libs, add at here
LIBS := nlua uv m

## define CFLAGS
CFLAGS += -g -Wall -Wextra -Wno-unused-parameter -D LUA_COMPAT_5_2 -D LUA_COMPAT_5_1 
ifeq (RELEASE,$(RELEASE))
CFLAGS += -D RELEASE -O3
endif

## define LDFLAGS
LDFLAGS += 

ifeq (sunos,$(PLATFORM))
HAVE_DTRACE ?= 1
CFLAGS += -D__EXTENSIONS__ -D_XOPEN_SOURCE=500
LIBS += kstat nsl sendfile socket 
endif

ifeq (aix,$(PLATFORM))
CFLAGS += -D_ALL_SOURCE -D_XOPEN_SOURCE=500
LIBS += perfstat 
endif

ifeq (darwin,$(PLATFORM))
LDFLAGS += -undefined dynamic_lookup \
           -framework Foundation \
           -framework CoreServices \
           -framework ApplicationServices
endif

ifeq (linux,$(PLATFORM))
LIBS += dl rt pthread
endif

ifeq (freebsd,$(PLATFORM))
ifeq ($(shell dtrace -l >/dev/null; echo $$?),0)
HAVE_DTRACE ?= 1
endif
LIBS += kvm pthread
endif

ifeq (dragonfly,$(PLATFORM))
LIBS += kvm
endif

ifeq (netbsd,$(PLATFORM))
LIBS += kvm
endif

ifeq (openbsd,$(PLATFORM))
LIBS += kvm
endif

## !!! HAVE_DTRACE not supported(undefined reference to `__dtrace_uv___tick__start' & undefined reference to `__dtrace_uv___tick__stop')
ifeq ($(HAVE_DTRACE), 1)
INCLUDE_PATH += ../deps/uv/src/unix
CFLAGS += -DHAVE_DTRACE
endif

## get all source files  
SRCS += $(wildcard $(SRC_PATH)/*.cpp)

## all .o based on all .cpp 
OBJS := $(SRCS:.cpp=.o)

## get all include path  
CFLAGS += $(foreach dir, $(INCLUDE_PATH), -I$(dir))  
.cpp.o:
	$(CPP) -c $(CFLAGS) -o $@ $<

## get all library path  
LDFLAGS += $(foreach lib, $(LIBRARY_PATH), -L$(lib))  
## get all librarys  
LDFLAGS += $(foreach lib, $(LIBS), -l$(lib))  
$(TARGET): $(OBJS) 
	$(CPP) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) *.o $(TARGET)
