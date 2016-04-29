PB_VERSION = 0.5
LUA_VERSION = 5.1
TARGET = protobuf.so

# See https://github.com/cloudwu/pbc for specific details.

## Linux/BSD
PREFIX ?=          /usr/local
LDFLAGS +=         -shared

## OSX (Macports)
#PREFIX ?=          /opt/local
#LDFLAGS +=         -bundle -undefined dynamic_lookup

#added by xdczju@sina.com
ifeq ($(OS),Windows_NT)
   TARGET = protobuf.dll
   LDFLAGS2 += -lluajit51
endif

LUA_INCLUDE_DIR ?= $(PREFIX)/include
LUA_LIB_DIR ?=     $(PREFIX)/lib/lua/$(LUA_VERSION)

# Some versions of Solaris are missing isinf(). Add -DMISSING_ISINF to
# CFLAGS to work around this bug.

#CFLAGS ?=          -g -Wall -pedantic -fno-inline
CFLAGS ?=          -g -O3 -Wall -pedantic
override CFLAGS += -fpic -I$(LUA_INCLUDE_DIR) -Isrc -DVERSION=\"$(PB_VERSION)\"

INSTALL ?= install

.PHONY: all clean install package

all: $(TARGET)


OBJS = binding/lua/pbc-lua.o src/context.o src/varint.o src/array.o src/pattern.o src/register.o src/proto.o src/map.o src/alloc.o src/rmessage.o src/wmessage.o src/bootstrap.o src/stringpool.o src/decode.o
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDFLAGS2)

install:
	$(INSTALL) -d $(DESTDIR)/$(LUA_LIB_DIR)
	$(INSTALL) $(TARGET) $(DESTDIR)/$(LUA_LIB_DIR)

clean:
	rm -f *.o *.so

package:
	git archive --prefix="lua-pb-$(PB_VERSION)/" master | \
		gzip -9 > "lua-pb-$(PB_VERSION).tar.gz"
	git archive --prefix="lua-pb-$(PB_VERSION)/" \
		-o "lua-pb-$(PB_VERSION).zip" master

		

