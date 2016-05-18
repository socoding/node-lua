.PHONY: all clean install

ifdef PLATFORM
override PLATFORM := $(shell echo $(PLATFORM) | tr "[A-Z]" "[a-z]")
else
PLATFORM = $(shell sh -c 'uname -s | tr "[A-Z]" "[a-z]"')
endif

ROOT_PATH = $(shell pwd)

define MAKE_PLATFORM
	##not essential builds
	cd mime  && make PLATFORM=$(1); cd $(ROOT_PATH)
	cd cjson && make PLATFORM=$(1); cd $(ROOT_PATH)
	cd lfs   && make PLATFORM=$(1); cd $(ROOT_PATH)
	cd lpeg  && make PLATFORM=$(1); cd $(ROOT_PATH)
	cd pb    && make PLATFORM=$(1); cd $(ROOT_PATH)
endef

all:
	$(call MAKE_PLATFORM,$(PLATFORM))
	
clean:
	##not essential builds
	cd mime  && make clean; cd $(ROOT_PATH)
	cd cjson && make clean; cd $(ROOT_PATH)
	cd lfs   && make clean; cd $(ROOT_PATH)
	cd lpeg  && make clean; cd $(ROOT_PATH)
	cd pb    && make clean; cd $(ROOT_PATH)

install:
	##not essential install
	cp mime/mime.so		mime.so
	cp cjson/cjson.so	cjson.so
	cp lfs/lfs.so		lfs.so
	cp lpeg/lpeg.so		lpeg.so
	cp pb/protobuf.so	protobuf.so
