# Compiler settings
CC = gcc
CFLAGS = -fPIC -std=c99 -Wall -Wextra -O2
LDFLAGS = -shared

# Detect OS
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')

# Shared library compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC
	SHOBJ_LDFLAGS ?= -shared
ifneq (,$(findstring armv,$(uname_M)))
	SHOBJ_LDFLAGS += -latomic
endif
ifneq (,$(findstring aarch64,$(uname_M)))
	SHOBJ_LDFLAGS += -latomic
endif
else
	SHOBJ_CFLAGS ?= -W -Wall -dynamic -fno-common -g -ggdb -std=c99 -O3 -fPIC
	SHOBJ_LDFLAGS ?= -bundle -undefined dynamic_lookup
endif

# Paths
REDIS_ROOT = ../../
MODULE_HEADER = $(REDIS_ROOT)/src/redismodule.h

# Target
TARGET = numpy_redis.so
SOURCE = numpy_redis.c

# Build
all: $(TARGET)

$(TARGET): $(SOURCE) $(MODULE_HEADER)
	$(CC) -I$(REDIS_ROOT)/src $(SHOBJ_CFLAGS) $(SHOBJ_LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean

