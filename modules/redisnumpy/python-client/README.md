# NumPy Redis - True NumPy-Native Array Caching

**Cache NumPy arrays in Redis with ZERO information loss.**

## 🎯 Vision

Enable Python developers to cache NumPy arrays in Redis when scaling to millions of users, with **perfect reconstruction guaranteed**.

**Core Philosophy:** *"What you store is exactly what you get back."*

```python
import numpy as np
from client import NumPyRedis

client = NumPyRedis()

# Create complex array with all metadata
arr = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32, order='F')
arr.flags.writeable = False

# Store in Redis (scales to millions of users)
client.set("my_array", arr)

# Retrieve later - PERFECT reconstruction!
retrieved = client.get("my_array")

assert np.array_equal(arr, retrieved)
assert arr.dtype == retrieved.dtype
assert arr.flags['F_CONTIGUOUS'] == retrieved.flags['F_CONTIGUOUS']
assert arr.flags['WRITEABLE'] == retrieved.flags['WRITEABLE']

print("✅ Zero information loss achieved!")
```

---

## 🛠️ How It Works

**Simple architecture:**

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
- ✅ Just store the bytes and return them to Python
- ✅ Python's `np.load()` handles perfect reconstruction
- ✅ Zero information loss guaranteed

---

## 📋 Implementation Steps

### Phase 1: Core Functionality (Current)

**1. Python Client** ✅ DONE
- Uses `np.save()` and `np.load()` for serialization
- Comprehensive test suite validates zero information loss
- Location: `python-client/client.py`
- Run tests: `cd python-client && python client.py`

**2. C Module** ⏳ IN PROGRESS
- Simple data structure (stores `.npy` bytes directly):
  ```c
  struct NumpyArray {
      void *npy_data;    // Raw .npy format bytes (self-describing)
      size_t npy_size;   // Total bytes
  };
  ```
- Commands to implement:
  - `NP.SET key <npy_bytes>` - Store array
  - `NP.GET key` - Retrieve array
- Location: `numpy_redis.c`

**3. Build & Test** ⏳ TODO
```bash
# Build
make

# Start Redis with module
redis-server --loadmodule ./numpy_redis.so

# Run Python tests
cd python-client && python client.py
```

### Phase 2: Production Ready

- [ ] RDB persistence (save/load across restarts)
- [ ] Memory usage reporting
- [ ] Error handling

### Phase 3: Advanced Features

- [ ] Metadata commands (`NP.SHAPE`, `NP.DTYPE`)
- [ ] Batch operations (`NP.MSET`, `NP.MGET`)
- [ ] TTL support

---

## 🧪 Test Suite

The Python client includes comprehensive tests validating zero information loss:

```bash
cd python-client
python client.py
```

**Tests validate:**
- ✅ Basic arrays (1D, 2D, 3D)
- ✅ All dtypes (float32, float64, int32, int64, uint8, complex, structured)
- ✅ Memory layout (C-contiguous, Fortran-contiguous)
- ✅ Array flags (writeable, aligned)
- ✅ Byte order (little-endian, big-endian)
- ✅ Large arrays and image batches (4D tensors)

---

## 📂 Project Structure

```
modules/redisnumpy/
├── README.md              # This file - vision and implementation guide
├── numpy_redis.c          # C module (Redis module implementation)
└── python-client/
    └── client.py          # Python client with test suite
```

---

**Remember:** *"What you store is exactly what you get back."* 🚀

