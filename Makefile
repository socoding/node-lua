.PHONY: all release clean install

ifdef PLATFORM
override PLATFORM := $(shell echo $(PLATFORM) | tr "[A-Z]" "[a-z]")
else
PLATFORM = $(shell sh -c 'uname -s | tr "[A-Z]" "[a-z]"')
endif

ROOT_PATH = $(shell pwd')

define MAKE_PLATFORM
	##main essential builds
	cd deps/lua  && make $(2); cd $(ROOT_PATH)
	cd deps/uv   && make PLATFORM=$(1); rm -f libuv.so libuv.dylib; cd $(ROOT_PATH)
	cd src       && make PLATFORM=$(1) RELEASE=$(3); cd $(ROOT_PATH)
	##not essential builds
	cd clib/mime && make PLATFORM=$(1) RELEASE=$(3); cd $(ROOT_PATH)
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
	cd deps/lua  && make clean; cd $(ROOT_PATH)
	cd deps/uv   && make clean; cd $(ROOT_PATH)
	cd src       && make clean; cd $(ROOT_PATH)
	##not essential builds
	cd clib/mime && make clean; cd $(ROOT_PATH)

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