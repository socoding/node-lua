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
CFLAGS += -g -pedantic -O3 -Wall -Wextra -Wno-unused-parameter -D LUA_COMPAT_ALL 
ifeq (RELEASE,$(RELEASE))
CFLAGS += -D RELEASE
endif

## define LDFLAGS
LDFLAGS += 

ifeq (sunos,$(PLATFORM))
LIBS += nsl socket
endif

ifeq (darwin,$(PLATFORM))
LDFLAGS += -undefined dynamic_lookup \
           -framework Foundation \
           -framework CoreServices \
           -framework ApplicationServices
endif

ifeq (linux,$(PLATFORM))
LIBS += dl rt
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
