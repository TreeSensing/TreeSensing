#set environment variable RM_INCLUDE_DIR to the location of redismodule.h
ifndef RM_INCLUDE_DIR
	RM_INCLUDE_DIR=./
endif

ifndef RMUTIL_LIBDIR
	RMUTIL_LIBDIR=./rmutil
endif

ifndef MODULE_NAME
	MODULE_NAME=RedisSketches
endif

# find the OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?=  -fno-common -g -ggdb
	SHOBJ_LDFLAGS ?= -shared -Bsymbolic
else
	SHOBJ_CFLAGS ?= -dynamic -fno-common -g -ggdb
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup
endif
CFLAGS = -I$(RM_INCLUDE_DIR) -I$(RMUTIL_LIBDIR) -Wall -g -fPIC -lc -lm -std=gnu99
CXXFLAGS = -I$(RM_INCLUDE_DIR) -I$(RMUTIL_LIBDIR) -Wall -g -fPIC -std=c++17 -fno-use-cxa-atexit
CC=gcc
CXX=g++

DEPS = module.o \
 		util/MurmurHash2.o

#DEPS = module.o \
	   util/MurmurHash2.o

all: rmutil $(MODULE_NAME).so

rmutil: FORCE
	$(MAKE) -C $(RMUTIL_LIBDIR)

$(MODULE_NAME).so: $(DEPS)
	$(LD) -o $@ $^ $(SHOBJ_LDFLAGS) $(LIBS) -L$(RMUTIL_LIBDIR) -lrmutil -lc 

clean:
	rm -rf *.xo *.so *.o
	rm -rf */*.o
	rm -rf $(RMUTIL_LIBDIR)/*.so $(RMUTIL_LIBDIR)/*.o $(RMUTIL_LIBDIR)/*.a

FORCE:
