# NumPy Redis - Multidimensional Array Storage

## Vision

Store and retrieve multidimensional NumPy arrays directly in Redis with native performance. This module enables seamless integration of NumPy arrays with Redis, supporting arrays of any dimension and data type.

## Key Features

- **Multidimensional Support**: Store 1D vectors, 2D matrices, 3D tensors, and higher-dimensional arrays
- **Type Preservation**: Maintain NumPy data types (float32, float64, int32, int64, uint8, etc.)
- **Shape Preservation**: Automatically preserve array dimensions and shapes
- **High Performance**: Optimized serialization and deserialization for large arrays
- **Simple API**: Intuitive `set()` and `get()` operations similar to standard Redis

## Use Cases

- **Machine Learning**: Store model weights, biases, and training data
- **Image Processing**: Store image batches and processed data
- **Scientific Computing**: Store multidimensional scientific data
- **Data Analysis**: Cache large NumPy arrays for quick retrieval

## Installation & Setup

### Build NumPy Redis Module

```bash
git clone https://github.com/your-org/numpy-redis
cd numpy-redis
make
```

### Start Redis Server with NumPy Module

```bash
./src/redis-server --loadmodule ./src/libnumpy_redis.so --daemonize yes
sleep 2
echo "✅ NumPy Redis is running!"
```

### Verify Connection

```bash
redis-cli PING
```

## Python Client Usage

### Installation

The Python client is located in `python-client/client.py` and requires only NumPy (no external Redis package dependency).

```bash
# Copy the client to your project
cp python-client/client.py your_project/
```

### Basic Operations

```python
import numpy as np
from client import NumPyRedis

# Connect to the server
client = NumPyRedis(host='localhost', port=6379)

# Store a 1D array
arr_1d = np.array([1, 2, 3, 4, 5])
client.set("simple_vector", arr_1d)

# Store a 2D matrix
matrix = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
client.set("identity_matrix", matrix)

# Store a 3D tensor
tensor = np.random.randn(4, 3, 2)
client.set("random_tensor", tensor)

# Retrieve arrays
retrieved_1d = client.get("simple_vector")
retrieved_matrix = client.get("identity_matrix")
retrieved_tensor = client.get("random_tensor")

# Clean up
client.close()
```

### Supported Data Types

The client supports the following NumPy data types:
- `float64` (default)
- `float32`
- `int64`
- `int32`
- `int8`
- `uint8`

```python
# Store float32 array
float_array = np.array([1.5, 2.7, 3.1], dtype=np.float32)
client.set("float_data", float_array)

# Store int64 array
int_array = np.array([100, 200, 300], dtype=np.int64)
client.set("int_data", int_array)

# Store uint8 array (useful for images)
image_array = np.random.randint(0, 256, (100, 100), dtype=np.uint8)
client.set("image_data", image_array)

# Retrieve with dtype preserved
float_data = client.get("float_data")    # dtype: float32
int_data = client.get("int_data")        # dtype: int64
image_data = client.get("image_data")    # dtype: uint8
```

### Real-World Examples

#### Image Data Storage

```python
# Single image (height, width, channels)
image_data = np.random.randint(0, 256, (480, 640, 3), dtype=np.uint8)
client.set("sample_image", image_data)

# Batch of images (batch_size, height, width, channels)
batch_images = np.random.randn(32, 224, 224, 3).astype(np.float32)
client.set("image_batch", batch_images)

# Retrieve
single_image = client.get("sample_image")      # shape: (480, 640, 3)
image_batch = client.get("image_batch")        # shape: (32, 224, 224, 3)
```

#### Neural Network Parameters

```python
# Store weights and biases
weights = np.random.randn(512, 256).astype(np.float32)
bias = np.random.randn(256).astype(np.float32)

client.set("layer_weights", weights)
client.set("layer_bias", bias)

# Retrieve for inference
layer_weights = client.get("layer_weights")    # shape: (512, 256)
layer_bias = client.get("layer_bias")          # shape: (256,)
```

## Performance Characteristics

The module is optimized for:
- **Large arrays**: Efficiently handle arrays with millions of elements
- **Batch operations**: Store and retrieve multiple arrays quickly
- **Data integrity**: Ensure no data loss or corruption during serialization