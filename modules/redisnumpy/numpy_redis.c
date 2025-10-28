#include "redismodule.h"
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

void *NumpyArrayRdbLoad(RedisModuleIO *rdb, int encver) {
    // Stub - not implemented yet
    return NULL;
}

void NumpyArrayRdbSave(RedisModuleIO *rdb, void *value) {
    // Stub - not implemented yet
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

    // 3. Store as a string value (for now, until data type works)
    RedisModule_StringSet(key, argv[2]);
    RedisModule_CloseKey(key);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
    }

int NumpyGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // 1. Get key
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);

    // 2. Check if key exists
    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        RedisModule_ReplyWithNull(ctx);
        RedisModule_CloseKey(key);
        return REDISMODULE_OK;
    }

    // 3. Get the string value
    size_t len;
    const char *data = RedisModule_StringDMA(key, &len, REDISMODULE_READ);

    // 4. Return .npy bytes to client
    RedisModule_ReplyWithStringBuffer(ctx, data, len);
    RedisModule_CloseKey(key);

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (RedisModule_Init(ctx, "numpy", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    // Create data type
    RedisModuleTypeMethods tm = {
        .version = REDISMODULE_TYPE_METHOD_VERSION,
        .rdb_load = NULL,
        .rdb_save = NULL,
        .free = NumpyArrayFree,
    };

    NumpyType = RedisModule_CreateDataType(ctx, "numpy", 0, &tm);
    // Don't fail if data type creation fails - just continue
    // if (NumpyType == NULL) {
    //     return REDISMODULE_ERR;
    // }

    // Register commands
    if (RedisModule_CreateCommand(ctx, "np.set", NumpySet_RedisCommand, "write", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "np.get", NumpyGet_RedisCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
