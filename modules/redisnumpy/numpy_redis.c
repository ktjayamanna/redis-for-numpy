#include "../redismodule.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

static RedisModuleType *NumpyType;

struct NumpyArray
{
    void *npy_data;
    size_t npy_size;
};

struct NumpyArray *createNumpyArray(void *npy_data, size_t npy_size) {
    struct NumpyArray *arr = RedisModule_Alloc(sizeof(*arr));
    arr->npy_data = RedisModule_Alloc(npy_size);
    memcpy(arr->npy_data, npy_data, npy_size);
    arr->npy_size = npy_size;
    return arr;
}

void NumpyArrayFree(void *value) {
    struct NumpyArray *arr = value;
    RedisModule_Free(arr->npy_data);
    RedisModule_Free(arr);
}

/**
 * NP.SET key <npy_bytes> - Store NumPy array bytes in Redis.
 *
 * Store NumPy array bytes in Redis. The array is stored in a binary blob
 * without any parsing or reconstruction in C. The bytes are stored as-is, so
 * all metadata (dtype, shape, strides, flags, byte order) is preserved.
 *
 * @param ctx The Redis module context.
 * @param argv A string array containing the command arguments.
        argv is an array:
        ┌─────────────────────────────────────┐
        │ argv[0] → "NP.SET"                  │
        │ argv[1] → "my_key"                  │
        │ argv[2] → <128 bytes of .npy data>  │
        │ argv[3] → NULL (end of array)       │
        └─────────────────────────────────────┘
 * @param argc The number of arguments in argv.
 * @return REDISMODULE_OK on success, otherwise an error code.
 */
int NumpySet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // 1. Get key
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    
    // 2. Get .npy bytes from client
    size_t npy_size;
    const char *npy_data = RedisModule_StringPtrLen(argv[2], &npy_size);
    
    // 3. Create array object (just stores the bytes)
    struct NumpyArray *arr = createNumpyArray((void*)npy_data, npy_size);
    
    // 4. Store in Redis
    RedisModule_ModuleTypeSetValue(key, NumpyType, arr);
    RedisModule_CloseKey(key);
    
    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
    }
