# NumPy Redis Module Development Roadmap

## 🎯 Goal
Build a Redis module that enables native storage and retrieval of NumPy arrays with zero-copy performance, following Redis module best practices.

## 🏗️ Architecture Strategy

### **Hybrid Approach: Best of Both Worlds**
- **Start Simple**: Follow `hellotype.c` pattern for learning (single file, clear structure)
- **Build Professional**: Implement in `modules/redisnumpy/` as separate `.so` file (like `vector-sets`)
- **Evolve Gradually**: Refactor into multi-file structure as complexity grows

### **Why This Approach?**
✅ **Simplicity of `hellotype.c`** - Easy to learn, single file, clear patterns
✅ **Professionalism of `vector-sets`** - Separate `.so`, distributable, runtime loading
✅ **No Redis Core Dependency** - Only uses `redismodule.h` API (plugin architecture)
✅ **Clear Upgrade Path** - Start simple, add complexity incrementally

## 📁 File Structure Evolution

### **Week 1: Minimal Structure (Learning Phase)**
```
modules/redisnumpy/
├── numpy_redis.c               # Single-file module (like hellotype.c)
├── Makefile                    # Simple build configuration
├── README.md                   # Basic documentation
└── ROADMAP.md                  # This file
```

### **Week 2-3: Growing Structure (Development Phase)**
```
modules/redisnumpy/
├── numpy_redis.c               # Main module + command handlers
├── numpy_types.h               # Data structure definitions
├── numpy_utils.c               # Helper functions (add when needed)
├── numpy_utils.h               # Helper headers
├── Makefile                    # Enhanced build system
├── tests/
│   ├── test_basic.py           # Basic functionality tests
│   └── test_persistence.py     # RDB/AOF tests
├── README.md                   # Module documentation
└── ROADMAP.md                  # This file
```

### **Week 4+: Production Structure (Polish Phase)**
```
modules/redisnumpy/
├── numpy_redis.c               # Module initialization + OnLoad
├── numpy_commands.c            # Command implementations
├── numpy_types.h               # Data structures
├── numpy_persistence.c         # RDB/AOF handlers
├── numpy_utils.c               # Utility functions
├── numpy_config.c              # Configuration (optional)
├── Makefile                    # Full build system
├── tests/
│   ├── test_basic.py
│   ├── test_persistence.py
│   └── test_performance.py
├── README.md
└── ROADMAP.md
```

## 🚀 Development Phases

### **Phase 1: Get Basics Working (Week 1)** 🎯
**Goal:** Understand Redis module fundamentals and get a minimal working module

**Study First:**
1. Read `src/modules/hellotype.c` line by line (343 lines)
   - Understand `RedisModule_OnLoad()` pattern
   - Study custom data type creation
   - Learn command registration
   - See RDB save/load implementation
   - Understand memory management with `RedisModule_Alloc/Free`

2. Read `src/redismodule.h` API documentation
   - Understand the function pointer mechanism
   - See available API functions
   - Learn the module initialization flow

**Create:**
1. **`numpy_redis.c`** - Single-file module (350-400 lines)
   ```c
   #include "../../src/redismodule.h"

   // Data structure
   struct NumpyArray {
       uint8_t dtype;           // Data type (0=float32, 1=float64, etc.)
       uint32_t ndim;           // Number of dimensions
       uint64_t *shape;         // Array shape
       void *data;              // Raw data buffer
       size_t data_size;        // Total bytes
   };

   // Memory management
   struct NumpyArray *createNumpyArray(...);
   void releaseNumpyArray(struct NumpyArray *arr);

   // Commands
   int NumpySet_RedisCommand(...);  // NP.SET key dtype ndim shape data
   int NumpyGet_RedisCommand(...);  // NP.GET key

   // Persistence (RDB)
   void *NumpyArrayRdbLoad(...);
   void NumpyArrayRdbSave(...);
   void NumpyArrayFree(...);
   size_t NumpyArrayMemUsage(...);

   // Module initialization
   int RedisModule_OnLoad(...);
   ```

2. **`Makefile`** - Simple build system
   ```makefile
   CC = cc
   CFLAGS = -O2 -Wall -Wextra -g -std=c11
   SHOBJ_LDFLAGS = -shared

   all: numpy_redis.so

   numpy_redis.so: numpy_redis.o
       $(CC) -o $@ $^ $(SHOBJ_LDFLAGS) -lc

   clean:
       rm -f *.o *.so
   ```

**Test:**
```bash
# Build
cd modules/redisnumpy && make

# Run Redis with module
redis-server --loadmodule ./numpy_redis.so

# Test commands
redis-cli
> NP.SET myarray <data>
> NP.GET myarray
```

**Deliverable:** ✅ Basic NP.SET/NP.GET commands working with simple 1D arrays

---

### **Phase 2: Add Persistence & More Commands (Week 2)** 💾
**Goal:** Make arrays persist across Redis restarts and add metadata commands

**Enhance:**
1. **`numpy_redis.c`** - Add more commands
   ```c
   // New commands
   int NumpyShape_RedisCommand(...);  // NP.SHAPE key
   int NumpyDtype_RedisCommand(...);  // NP.DTYPE key
   int NumpySize_RedisCommand(...);   // NP.SIZE key
   int NumpyInfo_RedisCommand(...);   // NP.INFO key

   // Improve RDB save/load
   // - Handle multi-dimensional arrays
   // - Support all data types
   // - Add version encoding

   // Add AOF rewrite (optional)
   void NumpyArrayAofRewrite(...);
   ```

**Test:**
```bash
# Test persistence
redis-cli NP.SET arr1 <data>
redis-cli SAVE
redis-cli SHUTDOWN
redis-server --loadmodule ./numpy_redis.so
redis-cli NP.GET arr1  # Should return saved data

# Test metadata commands
redis-cli NP.SHAPE arr1
redis-cli NP.DTYPE arr1
redis-cli NP.SIZE arr1
redis-cli NP.INFO arr1
```

**Deliverable:** ✅ Arrays persist across restarts + metadata commands working

---

### **Phase 3: Refactor & Optimize (Week 3)** 🔧
**Goal:** Split into multiple files and improve code organization

**Refactor:**
1. **Split `numpy_redis.c` into:**
   - `numpy_redis.c` - Module initialization only
   - `numpy_types.h` - Data structure definitions
   - `numpy_commands.c` - Command implementations
   - `numpy_persistence.c` - RDB/AOF handlers
   - `numpy_utils.c` - Helper functions

2. **Add error handling:**
   ```c
   // Validate array dimensions
   // Check data type compatibility
   // Handle memory allocation failures
   // Proper error messages
   ```

3. **Optimize performance:**
   ```c
   // Zero-copy where possible
   // Efficient serialization
   // Memory pooling (if needed)
   ```

**Update Makefile:**
```makefile
OBJS = numpy_redis.o numpy_commands.o numpy_persistence.o numpy_utils.o

numpy_redis.so: $(OBJS)
    $(CC) -o $@ $^ $(SHOBJ_LDFLAGS) -lc
```

**Deliverable:** ✅ Clean, maintainable code structure

---

### **Phase 4: Test & Polish (Week 4)** ✨
**Goal:** Comprehensive testing and production readiness

**Create Tests:**
1. **`tests/test_basic.py`**
   ```python
   import redis
   import numpy as np

   def test_set_get_1d():
       # Test 1D arrays

   def test_set_get_2d():
       # Test 2D arrays

   def test_different_dtypes():
       # Test float32, int64, etc.
   ```

2. **`tests/test_persistence.py`**
   ```python
   def test_rdb_save_load():
       # Test RDB persistence

   def test_aof_rewrite():
       # Test AOF rewrite
   ```

3. **`tests/test_performance.py`**
   ```python
   def benchmark_vs_pickle():
       # Compare with pickle+Redis

   def test_large_arrays():
       # Test with 100MB+ arrays
   ```

**Polish:**
- Add comprehensive error messages
- Write detailed README.md
- Add inline documentation
- Performance profiling
- Memory leak testing

**Deliverable:** ✅ Production-ready, fully tested module

---

## 📋 Implementation Checklist

### **Week 1: Core Module Functions** (Following HelloType Pattern)
- [ ] Study `src/modules/hellotype.c` completely
- [ ] Study `src/redismodule.h` API documentation
- [ ] Understand `dlopen/dlsym` and plugin architecture
- [ ] Create `numpy_redis.c` (single file, ~350 lines)
- [ ] Implement `struct NumpyArray` data structure
- [ ] Implement `createNumpyArray()` - Object creation
- [ ] Implement `releaseNumpyArray()` - Memory cleanup
- [ ] Implement `NumpySet_RedisCommand()` - NP.SET command
- [ ] Implement `NumpyGet_RedisCommand()` - NP.GET command
- [ ] Implement `NumpyArrayRdbLoad()` - Load from RDB
- [ ] Implement `NumpyArrayRdbSave()` - Save to RDB
- [ ] Implement `NumpyArrayFree()` - Object destruction
- [ ] Implement `NumpyArrayMemUsage()` - Memory estimation
- [ ] Implement `RedisModule_OnLoad()` - Module initialization
- [ ] Create `Makefile` for building `.so` file
- [ ] Test: Build module successfully
- [ ] Test: Load module into Redis
- [ ] Test: NP.SET/NP.GET with 1D float32 array

### **Week 2: Persistence & Metadata Commands**
- [ ] Test RDB save/load with Redis restart
- [ ] Implement `NumpyShape_RedisCommand()` - NP.SHAPE
- [ ] Implement `NumpyDtype_RedisCommand()` - NP.DTYPE
- [ ] Implement `NumpySize_RedisCommand()` - NP.SIZE
- [ ] Implement `NumpyInfo_RedisCommand()` - NP.INFO
- [ ] Support multi-dimensional arrays (2D, 3D, etc.)
- [ ] Add version encoding to RDB format
- [ ] Implement `NumpyArrayAofRewrite()` - AOF rewrite (optional)
- [ ] Test: All metadata commands
- [ ] Test: Persistence across restarts
- [ ] Test: 2D and 3D arrays

### **Week 3: Refactoring & Optimization**
- [ ] Split into multiple files (if needed)
- [ ] Create `numpy_types.h` - Data structures
- [ ] Create `numpy_utils.c` - Helper functions
- [ ] Add comprehensive error handling
- [ ] Add input validation (dimensions, data types)
- [ ] Optimize serialization/deserialization
- [ ] Add proper error messages
- [ ] Update Makefile for multi-file build
- [ ] Test: All commands still work
- [ ] Test: Error handling edge cases

### **Week 4: Testing & Polish**
- [ ] Create `tests/test_basic.py` - Basic functionality
- [ ] Create `tests/test_persistence.py` - RDB/AOF tests
- [ ] Create `tests/test_performance.py` - Benchmarks
- [ ] Test all data types (float32, float64, int32, int64, etc.)
- [ ] Test large arrays (100MB+)
- [ ] Test edge cases (empty arrays, 1-element arrays)
- [ ] Memory leak testing (valgrind)
- [ ] Performance profiling
- [ ] Write comprehensive README.md
- [ ] Add inline code documentation
- [ ] Final integration testing

### **Commands to Implement**
- [ ] `NP.SET key dtype ndim shape data` - Store NumPy array
- [ ] `NP.GET key` - Retrieve NumPy array (returns serialized format)
- [ ] `NP.SHAPE key` - Get array dimensions (e.g., [3, 4, 5])
- [ ] `NP.DTYPE key` - Get data type (e.g., "float32")
- [ ] `NP.SIZE key` - Get total element count
- [ ] `NP.INFO key` - Get all metadata (dtype, shape, size, memory)

### **Data Types to Support**
**Phase 1 (Week 1-2):**
- [ ] `float32` (4 bytes)
- [ ] `float64` (8 bytes)
- [ ] `int32` (4 bytes)
- [ ] `int64` (8 bytes)

**Phase 2 (Week 3-4):**
- [ ] `int8`, `int16`
- [ ] `uint8`, `uint16`, `uint32`, `uint64`
- [ ] `bool`

**Future (Optional):**
- [ ] `complex64`, `complex128`

---

## 🎯 Success Metrics

### **Functionality**
- [ ] All basic commands working (NP.SET, NP.GET, NP.SHAPE, NP.DTYPE, NP.SIZE, NP.INFO)
- [ ] Full Redis persistence compatibility (RDB save/load)
- [ ] Support for arrays up to 8 dimensions
- [ ] Support for at least 4 data types (float32, float64, int32, int64)

### **Performance**
- [ ] 5-10x faster than pickle+Redis for large arrays (>1MB)
- [ ] Memory usage within 10% of raw NumPy arrays
- [ ] Zero-copy retrieval where possible

### **Quality**
- [ ] No memory leaks (verified with valgrind)
- [ ] Comprehensive test coverage (>80%)
- [ ] Clear error messages for all failure cases
- [ ] Well-documented code and README

---

## 🧠 Key Learning Points

### **Why This Architecture Works**
1. **Plugin Architecture**: Module doesn't link to Redis code, only uses `redismodule.h` API
2. **Dynamic Loading**: Redis uses `dlopen()` to load `.so` at runtime
3. **Function Pointers**: `RedisModule_Init()` fills in API function pointers dynamically
4. **Binary Compatibility**: Module works across Redis versions with same API version
5. **Language Agnostic**: Any language that produces `.so` with `RedisModule_OnLoad` works

### **Following HelloType Pattern**
- Single file initially (easier to learn)
- Clear data structure (`struct NumpyArray`)
- Standard command pattern (`*_RedisCommand` functions)
- RDB save/load for persistence
- Memory management with `RedisModule_Alloc/Free`
- Proper module initialization in `RedisModule_OnLoad()`

### **Differences from HelloType**
- **Location**: `modules/redisnumpy/` (not `src/modules/`)
- **Build**: Separate `.so` file (not compiled into Redis)
- **Loading**: `--loadmodule` flag (not built-in)
- **Data**: NumPy arrays (not linked lists)
- **Commands**: `NP.*` (not `HELLOTYPE.*`)

---

## 🔄 Immediate Next Steps

### **Day 1-2: Study Phase**
1. ✅ Read `src/modules/hellotype.c` completely (343 lines)
   - Focus on: data structures, command handlers, RDB functions, OnLoad
2. ✅ Read `src/redismodule.h` header (understand API)
3. ✅ Understand the plugin architecture (why no code borrowing needed)

### **Day 3-4: Initial Implementation**
1. Create `modules/redisnumpy/numpy_redis.c`
2. Define `struct NumpyArray` data structure
3. Implement basic memory management functions
4. Implement `RedisModule_OnLoad()` skeleton

### **Day 5-7: First Working Version**
1. Implement `NP.SET` command (1D arrays only)
2. Implement `NP.GET` command (1D arrays only)
3. Implement RDB save/load
4. Create Makefile
5. Build and test!

**First milestone:** Get a 1D float32 array to store and retrieve successfully! 🎉

---

## 📚 Reference Materials

### **Study These Files**
- `src/modules/hellotype.c` - Your primary reference (343 lines)
- `src/redismodule.h` - The module API contract
- `modules/vector-sets/vset.c` - Advanced example (2,588 lines)
- `modules/vector-sets/Makefile` - Build system reference

### **Key Concepts to Understand**
- Redis Module API (function pointers)
- RDB persistence format
- Redis data types vs module types
- Memory management in modules
- Command registration and handling
- Error handling patterns

### **Don't Need to Study**
- ❌ Redis core internals (`src/server.c`, `src/db.c`, etc.)
- ❌ Redis networking code
- ❌ Redis cluster code
- ❌ Other Redis modules (except hellotype and vector-sets)

