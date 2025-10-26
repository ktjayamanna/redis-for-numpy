# NumPy Redis Module (C)

**Redis module for storing NumPy arrays with ZERO information loss.**

## 🎯 Vision

Store NumPy's `.npy` format bytes directly in Redis. No parsing, no reconstruction - just store and retrieve bytes.

**Core Philosophy:** *"Store the bytes, let NumPy handle the rest."*

---

## 🛠️ Architecture

```
Python Client                Redis Module (C)           Redis Storage
─────────────                ────────────────           ─────────────
np.array()                                              
    ↓                                                   
np.save() → .npy bytes  →    Store bytes as-is    →    Binary blob
                             (no parsing!)              
                                                        
np.load() ← .npy bytes  ←    Return bytes as-is   ←    Binary blob
    ↓                                                   
np.array()                                              
```

**Why this works:**
- ✅ NumPy's `.npy` format is self-describing (contains all metadata)
- ✅ No need to parse dtype, shape, strides in C
- ✅ Just store the bytes and return them
- ✅ Python's `np.load()` handles perfect reconstruction
- ✅ Zero information loss guaranteed

---

## 📦 Data Structure

**Simple and elegant:**

```c
struct NumpyArray {
    void *npy_data;    // Raw .npy format bytes (self-describing)
    size_t npy_size;   // Total bytes
};
```

**That's it!** No dtype parsing, no shape parsing, no stride calculations.

---

## 🔧 Implementation

### Commands to Implement

#### 1. `NP.SET key <npy_bytes>`

Store NumPy array bytes.

```c
int NumpySet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // 1. Get key
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    
    // 2. Get .npy bytes from client
    size_t npy_size;
    const char *npy_data = RedisModule_StringPtrLen(argv[2], &npy_size);
    
    // 3. Create array object (just stores the bytes)
    struct NumpyArray *arr = createNumpyArray((void*)npy_data, npy_size);
    
    // 4. Store in Redis
    RedisModule_ModuleTypeSetValue(key, NumpyArrayType, arr);
    RedisModule_CloseKey(key);
    
    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}
```

#### 2. `NP.GET key`

Retrieve NumPy array bytes.

```c
int NumpyGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // 1. Get key
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    
    // 2. Check if key exists
    if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
        RedisModule_ReplyWithNull(ctx);
        RedisModule_CloseKey(key);
        return REDISMODULE_OK;
    }
    
    // 3. Get array object
    struct NumpyArray *arr = RedisModule_ModuleTypeGetValue(key);
    
    // 4. Return .npy bytes to client
    RedisModule_ReplyWithStringBuffer(ctx, arr->npy_data, arr->npy_size);
    RedisModule_CloseKey(key);
    
    return REDISMODULE_OK;
}
```

### Helper Functions

#### Create Array

```c
struct NumpyArray *createNumpyArray(void *npy_data, size_t npy_size) {
    struct NumpyArray *arr = RedisModule_Alloc(sizeof(*arr));
    arr->npy_data = RedisModule_Alloc(npy_size);
    memcpy(arr->npy_data, npy_data, npy_size);
    arr->npy_size = npy_size;
    return arr;
}
```

#### Free Array

```c
void NumpyArrayFree(void *value) {
    struct NumpyArray *arr = value;
    RedisModule_Free(arr->npy_data);
    RedisModule_Free(arr);
}
```

#### Memory Usage

```c
size_t NumpyArrayMemUsage(const void *value) {
    const struct NumpyArray *arr = value;
    return sizeof(*arr) + arr->npy_size;
}
```

### RDB Persistence

#### Save to RDB

```c
void NumpyArrayRdbSave(RedisModuleIO *rdb, void *value) {
    struct NumpyArray *arr = value;
    RedisModule_SaveUnsigned(rdb, arr->npy_size);
    RedisModule_SaveStringBuffer(rdb, arr->npy_data, arr->npy_size);
}
```

#### Load from RDB

```c
void *NumpyArrayRdbLoad(RedisModuleIO *rdb, int encver) {
    if (encver != 0) return NULL;
    
    size_t npy_size = RedisModule_LoadUnsigned(rdb);
    size_t len;
    char *npy_data = RedisModule_LoadStringBuffer(rdb, &len);
    
    struct NumpyArray *arr = createNumpyArray(npy_data, npy_size);
    RedisModule_Free(npy_data);
    
    return arr;
}
```

### Module Initialization

```c
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (RedisModule_Init(ctx, "numpy", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;
    
    // Define type methods
    RedisModuleTypeMethods tm = {
        .version = REDISMODULE_TYPE_METHOD_VERSION,
        .rdb_load = NumpyArrayRdbLoad,
        .rdb_save = NumpyArrayRdbSave,
        .free = NumpyArrayFree,
        .mem_usage = NumpyArrayMemUsage
    };
    
    // Create type
    NumpyArrayType = RedisModule_CreateDataType(ctx, "numpyarry", 0, &tm);
    if (NumpyArrayType == NULL) return REDISMODULE_ERR;
    
    // Register commands
    if (RedisModule_CreateCommand(ctx, "np.set", NumpySet_RedisCommand, 
                                   "write deny-oom", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;
    
    if (RedisModule_CreateCommand(ctx, "np.get", NumpyGet_RedisCommand, 
                                   "readonly", 1, 1, 1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;
    
    return REDISMODULE_OK;
}
```

---

## 🏗️ Build

### Makefile

```makefile
# Compiler and flags
CC = gcc
CFLAGS = -fPIC -std=c99 -Wall -Wextra -O2
LDFLAGS = -shared

# Paths
REDIS_ROOT = ../../
MODULE_HEADER = $(REDIS_ROOT)/src/redismodule.h

# Target
TARGET = numpy_redis.so

# Build
all: $(TARGET)

$(TARGET): numpy_redis.c $(MODULE_HEADER)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
```

### Build Commands

```bash
# Build the module
make

# Start Redis with module
redis-server --loadmodule ./numpy_redis.so

# Test with redis-cli
redis-cli
> NP.SET test_key <binary_data>
> NP.GET test_key
```

---

## 🧪 Testing

### With Python Client

```bash
# Start Redis with module
redis-server --loadmodule ./numpy_redis.so

# In another terminal, run Python tests
cd python-client
python client.py
```

**Expected output:**
```
======================================================================
NumPy Redis - Zero Information Loss Test Suite
======================================================================

[Test 1] Basic 1D array
✅ 1D Array: shape=(5,), dtype=float64

[Test 2] 2D arrays with various dtypes
✅ 2D Array (float32): shape=(2, 3), dtype=float32
...

🎉 ALL TESTS PASSED - ZERO INFORMATION LOSS ACHIEVED!
```

---

## 📋 Implementation Checklist

### Phase 1: Core Functionality
- [ ] Define `struct NumpyArray`
- [ ] Implement `createNumpyArray()`
- [ ] Implement `NumpyArrayFree()`
- [ ] Implement `NumpySet_RedisCommand()`
- [ ] Implement `NumpyGet_RedisCommand()`
- [ ] Implement `RedisModule_OnLoad()`
- [ ] Create Makefile
- [ ] Build and test with Python client

### Phase 2: Persistence
- [ ] Implement `NumpyArrayRdbSave()`
- [ ] Implement `NumpyArrayRdbLoad()`
- [ ] Test save/load across Redis restarts

### Phase 3: Production Ready
- [ ] Implement `NumpyArrayMemUsage()`
- [ ] Add error handling
- [ ] Add input validation
- [ ] Performance testing

---

## 📂 File Structure

```
modules/redisnumpy/
├── README.md              # This file - C module implementation guide
├── numpy_redis.c          # C module implementation
├── Makefile               # Build configuration (to be created)
└── python-client/
    ├── README.md          # Python client guide
    └── client.py          # Python client with tests
```

---

## 🔑 Key Design Decisions

### 1. Store Bytes Directly
**Don't parse `.npy` format in C.** Just store the bytes.
- ✅ Simple C code (just memcpy)
- ✅ No parsing overhead
- ✅ Works with any `.npy` version
- ✅ All dtypes automatically supported

### 2. Let NumPy Handle Reconstruction
**Python's `np.load()` does all the work.**
- ✅ Perfect reconstruction guaranteed
- ✅ No need to understand `.npy` format internals
- ✅ Future-proof

### 3. Minimal Data Structure
**Just 2 fields: data pointer and size.**
- ✅ Simple to implement
- ✅ Easy to maintain
- ✅ Efficient memory usage

---

**Remember:** *"Store the bytes, let NumPy handle the rest."* 🚀

