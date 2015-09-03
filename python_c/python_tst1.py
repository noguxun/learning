from ctypes import *
import struct

# Reference:
#  http://docs.python.org/3.3/library/ctypes.html

lib = cdll.LoadLibrary('./libtst1.so')

lib.PrintHello()
lib.PrintMyWord(b"Come back to me, my dear cat")
lib.PrintMyWord(c_char_p(b'Show me your hands'))


some_int = lib.GetInt1(3);
print(some_int)

some_int = lib.GetInt2(c_int(5), c_int(5));
print(some_int)

lib.GetString.restype = c_char_p
binary_string = lib.GetString()
#print(binary_string)
print(binary_string.decode('utf-8'))

arry_size =  10
data_arry = (c_int * arry_size)()
lib.GetArrayData(arry_size, data_arry)
for index in range(arry_size):
    print(type(data_arry[index]))
    print(data_arry[index])

lib.GetBytes.restype = POINTER(c_ubyte)
length = c_int();
data_arry = lib.GetBytes(byref(length))
print(type(data_arry))
print(data_arry)
for index in range(length.value):
    print(data_arry[index])
    print(type(data_arry[index]))

py_str = string_at(data_arry, length)
print(py_str)
print(type(py_str))

uppacked_data = struct.unpack("2B8s", py_str)
print(uppacked_data)
print(type(uppacked_data))





