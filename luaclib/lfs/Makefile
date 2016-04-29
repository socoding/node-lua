LFS_VERSION = 1.6.3
LUA_VERSION = 5.1
TARGET = lfs.so

# See https://github.com/keplerproject/luafilesystem for platform specific
# details.

## Linux/BSD
PREFIX ?=          /usr/local
LDFLAGS +=         -shared

## OSX (Macports)
#PREFIX ?=          /opt/local
#LDFLAGS +=         -bundle -undefined dynamic_lookup

#added by xdczju@sina.com
ifeq ($(OS),Windows_NT)
   TARGET = lfs.dll
   LDFLAGS2 += -lluajit51
endif

LUA_INCLUDE_DIR ?= $(PREFIX)/include
LUA_LIB_DIR ?=     $(PREFIX)/lib/lua/$(LUA_VERSION)

# Some versions of Solaris are missing isinf(). Add -DMISSING_ISINF to
# CFLAGS to work around this bug.

#CFLAGS ?=          -g -Wall -pedantic -fno-inline
CFLAGS ?=          -g -O3 -Wall -pedantic
override CFLAGS += -fpic -I$(LUA_INCLUDE_DIR) -DVERSION=\"$(LFS_VERSION)\"

INSTALL ?= install

.PHONY: all clean install package

all: $(TARGET)

$(TARGET): lfs.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDFLAGS2)

install:
	$(INSTALL) -d $(DESTDIR)/$(LUA_LIB_DIR)
	$(INSTALL) $(TARGET) $(DESTDIR)/$(LUA_LIB_DIR) 

clean:
	rm -f *.o *.so

package:
	git archive --prefix="lua-lfs-$(LFS_VERSION)/" master | \
		gzip -9 > "lua-lfs-$(LFS_VERSION).tar.gz"
	git archive --prefix="lua-lfs-$(LFS_VERSION)/" \
		-o "lua-lfs-$(LFS_VERSION).zip" master

		
