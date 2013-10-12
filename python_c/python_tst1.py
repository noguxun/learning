from ctypes import cdll
lib = cdll.LoadLibrary('./libtst1.so')

lib.PrintHello()
