#include "../redismodule.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

static RedisModuleType *NumpyType;

struct NumpyArray
{
    uint8_t dtype;
    uint32_t ndim;
    uint64_t *shape;
    void *data;
    size_t data_size;
};


