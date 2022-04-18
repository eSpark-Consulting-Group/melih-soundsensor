
class Buffer:

    def __init__(self, size, value=0x00, endianness='little'):
        self.__size = size
        self.__endianness = endianness
        self.__buffer = bytearray(value.to_bytes(1, self.__endianness)*self.__size)

    @property
    def size(self):
        return self.__size
    
    def write_bytes(self, offset, data):
        assert(self.__size >= offset + len(data))
        self.__buffer[offset: offset+len(data)] = data

    def write_int(self, offset, value, size):
        assert(size > 0)
        assert(self.__size >= offset + size)
        self.write_bytes(offset, value.to_bytes(size, self.__endianness))

    def write_str(self, offset, string):
        buffer = bytes(string, 'utf8') + b'\x00'
        assert(self.__size >= offset + len(buffer))
        self.write_bytes(offset, buffer)

    def get(self):
        return bytearray(self.__buffer)

class BufferView:
    def __init__(self, buffer, offset=0, size=None):
        self.__buffer = buffer
        self.__offset = offset
        self.__size = size
        if size:
            self.__size__ = size
        else:
            self.__size = self.__buffer.size - self.__offset

    @property
    def size(self):
        return self.__size
    
    def write_bytes(self, offset, data):
        self.__buffer.write_bytes(self.__offset + offset, data)

    def write_int(self, offset, value, size):
        self.__buffer.write_int(self.__offset + offset,  value, size)

    def write_str(self, offset, string):
        self.__buffer.write_str(self.__offset + offset, string)

    def get(self):
        return bytearray(self.__buffer.get()[self.__offset: self.__offset + self.__size])