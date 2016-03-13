.PHONY: all release clean install

ifdef PLATFORM
override PLATFORM := $(shell echo $(PLATFORM) | tr "[A-Z]" "[a-z]")
else
PLATFORM = $(shell sh -c 'uname -s | tr "[A-Z]" "[a-z]"')
endif

define MAKE_PLATFORM
	pushd deps/lua && make $(2); rm -f liblua.so liblua.dylib; popd
	pushd deps/uv  && make PLATFORM=$(1); rm -f libuv.so libuv.dylib; popd
	pushd src      && make PLATFORM=$(1) RELEASE=$(3); popd
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
	pushd deps/lua && make clean; popd
	pushd deps/uv  && make clean; popd
	pushd src      && make clean; popd

install:
	mv src/node-lua ./
