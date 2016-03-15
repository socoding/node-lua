.PHONY: all release clean install

ifdef PLATFORM
override PLATFORM := $(shell echo $(PLATFORM) | tr "[A-Z]" "[a-z]")
else
PLATFORM = $(shell sh -c 'uname -s | tr "[A-Z]" "[a-z]"')
endif

define MAKE_PLATFORM
	##main essential builds
	pushd deps/lua  && make $(2); popd
	pushd deps/uv   && make PLATFORM=$(1); rm -f libuv.so libuv.dylib; popd
	pushd src       && make PLATFORM=$(1) RELEASE=$(3); popd
	##not essential builds
	pushd clib/mime && make PLATFORM=$(1) RELEASE=$(3); popd
endef

ifeq (darwin,$(PLATFORM)) 
	LUA_PLATFORM = macosx
else
	LUA_PLATFORM = $(PLATFORM)
endif

all:
	$(call MAKE_PLATFORM,$(PLATFORM),$(LUA_PLATFORM))

release:
	$(call MAKE_PLATFORM,$(PLATFORM),$(LUA_PLATFORM),RELEASE)
	
clean:
	##main essential builds
	pushd deps/lua  && make clean; popd
	pushd deps/uv   && make clean; popd
	pushd src       && make clean; popd
	##not essential builds
	pushd clib/mime && make clean; popd

# Where to install. The installation starts in the src and doc directories,
# so take care if INSTALL_TOP is not an absolute path.
INSTALL_TOP= /usr/local
INSTALL_BIN= $(INSTALL_TOP)/bin
INSTALL_LIB= $(INSTALL_TOP)/lib
INSTALL= cp -p
MKDIR= mkdir -p

install:
	mkdir -p /usr/local/bin /usr/local/lib
	##main essential install
	cp -f src/node-lua ./node-lua
	cp -f src/node-lua /usr/local/bin/node-lua
	cp deps/lua/libnlua.so ./libnlua.so
	cp deps/lua/libnlua.so /usr/local/lib/libnlua.so
	##not essential install
	cp clib/mime/mime.so clib/mime.so