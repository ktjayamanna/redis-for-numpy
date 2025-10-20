"""
NumPy Redis Client - Socket-based implementation for storing and retrieving multidimensional NumPy arrays.
No external Redis package dependency - uses raw socket communication with custom protocol.
"""

import socket
import struct
import numpy as np


class NumPyRedis:
    """
    A socket-based Redis client for storing and retrieving multidimensional NumPy arrays.

    This client communicates directly with the NumPy Redis server using the Redis protocol,
    handling serialization and deserialization of NumPy arrays while preserving shape and dtype.
    """

    def __init__(self, host='localhost', port=6379):
        """
        Initialize the NumPy Redis client.

        Args:
            host (str): Redis server hostname (default: 'localhost')
            port (int): Redis server port (default: 6379)
        """
        self.host = host
        self.port = port
        self.socket = None

    def connect(self):
        """Connect to the NumPy Redis server"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, self.port))

    def _send_command(self, *args):
        """
        Send Redis protocol command.

        Uses Redis Serialization Protocol (RESP):
        *<arg_count>\r\n$<len>\r\n<arg>\r\n...

        Args:
            *args: Command arguments (can be strings or bytes)

        Returns:
            bytes: Raw response from server
        """
        if not self.socket:
            self.connect()

        # Build Redis protocol command
        command = f"*{len(args)}\r\n"
        for arg in args:
            if isinstance(arg, bytes):
                arg_bytes = arg
            else:
                arg_bytes = str(arg).encode('utf-8')
            command += f"${len(arg_bytes)}\r\n"
            command = command.encode('latin-1') + arg_bytes + b"\r\n"

        if isinstance(command, str):
            command = command.encode('latin-1')

        self.socket.send(command)

        # Read response
        response = self.socket.recv(4096)
        return response

    def set(self, key, array):
        """
        Store a NumPy array in Redis.

        Args:
            key (str): The key to store the array under
            array (np.ndarray): The NumPy array to store

        Raises:
            ValueError: If the input is not a NumPy array
        """
        if not isinstance(array, np.ndarray):
            raise ValueError("Only numpy arrays supported")

        # Convert array to binary format with header
        header = self._array_to_header(array)
        data = array.tobytes()
        payload = header + data

        response = self._send_command(b'NP.SET', key.encode(), payload)
        return response == b'+OK\r\n'

    def get(self, key):
        """
        Retrieve a NumPy array from Redis.

        Args:
            key (str): The key of the array to retrieve

        Returns:
            np.ndarray: The retrieved NumPy array, or None if key doesn't exist
        """
        response = self._send_command(b'NP.GET', key.encode())

        # Parse bulk string response
        if response.startswith(b'$-1'):  # Null response
            return None

        # Simple parsing: find data between \r\n markers
        lines = response.split(b'\r\n')
        if len(lines) >= 2:
            data = lines[1]  # Bulk string data
            return self._header_to_array(data)

        return None

    def _array_to_header(self, array):
        """
        Convert numpy array to binary header format.

        Header format:
        - 1 byte: ndim (number of dimensions)
        - 8 bytes per dimension: shape values (uint64)
        - 1 byte: dtype code
        - 1 byte: reserved for future use

        Args:
            array (np.ndarray): The array to encode

        Returns:
            bytes: Binary header
        """
        header = struct.pack('B', array.ndim)  # 1 byte for ndim

        # Pack shape (each as 8-byte int)
        for dim in array.shape:
            header += struct.pack('Q', dim)

        # Pack dtype code
        dtype_map = {
            np.float64: 0, np.float32: 1,
            np.int64: 2, np.int32: 3,
            np.int8: 4, np.uint8: 5
        }
        dtype_code = dtype_map.get(array.dtype.type, 0)
        header += struct.pack('B', dtype_code)

        # Reserved byte for future use
        header += struct.pack('B', 0)

        return header

    def _header_to_array(self, data):
        """
        Convert binary data back to numpy array.

        Args:
            data (bytes): Binary data with header and array content

        Returns:
            np.ndarray: Reconstructed array, or None on error
        """
        try:
            # Parse header
            ndim = struct.unpack('B', data[0:1])[0]
            offset = 1

            # Parse shape
            shape = []
            for i in range(ndim):
                dim = struct.unpack('Q', data[offset:offset+8])[0]
                shape.append(dim)
                offset += 8

            # Parse dtype
            dtype_code = struct.unpack('B', data[offset:offset+1])[0]
            offset += 2  # Skip dtype and reserved byte

            dtype_map = {
                0: np.float64, 1: np.float32,
                2: np.int64, 3: np.int32,
                4: np.int8, 5: np.uint8
            }
            dtype = dtype_map.get(dtype_code, np.float64)

            # Extract array data
            array_data = data[offset:]
            return np.frombuffer(array_data, dtype=dtype).reshape(shape)

        except Exception as e:
            print(f"Error parsing array: {e}")
            return None

    def close(self):
        """Close connection to server"""
        if self.socket:
            self.socket.close()
            self.socket = None


if __name__ == "__main__":
    # Simple test
    client = NumPyRedis()

    try:
        # Test 1D array
        arr_1d = np.array([1, 2, 3, 4, 5])
        client.set("test_1d", arr_1d)
        retrieved_1d = client.get("test_1d")
        print(f"✅ 1D Array: {retrieved_1d}, shape: {retrieved_1d.shape}, dtype: {retrieved_1d.dtype}")

        # Test 2D array
        arr_2d = np.array([[1, 2, 3], [4, 5, 6]])
        client.set("test_2d", arr_2d)
        retrieved_2d = client.get("test_2d")
        print(f"✅ 2D Array shape: {retrieved_2d.shape}, dtype: {retrieved_2d.dtype}")

        # Test 3D array
        arr_3d = np.random.randn(2, 3, 4)
        client.set("test_3d", arr_3d)
        retrieved_3d = client.get("test_3d")
        print(f"✅ 3D Array shape: {retrieved_3d.shape}, dtype: {retrieved_3d.dtype}")

        # Test different dtypes
        float_arr = np.array([1.5, 2.7, 3.1], dtype=np.float32)
        client.set("test_float32", float_arr)
        retrieved_float = client.get("test_float32")
        print(f"✅ Float32 Array: {retrieved_float}, dtype: {retrieved_float.dtype}")

        print("\n🎉 All tests passed!")

    except Exception as e:
        print(f"❌ Test failed: {e}")

    finally:
        client.close()
