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

install:
	##main essential install
	cp -f src/node-lua ./node-lua
	cp -f src/node-lua /usr/local/bin/
	cp deps/lua/libnlua.so ./libnlua.so
	cp deps/lua/libnlua.so /usr/lib/libnlua.so
	##not essential install
	cp clib/mime/mime.so clib/mime.so