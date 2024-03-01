import numpy as np
import ctypes


def show_matrix(arr, width, height):
    matrix = np.array(arr).reshape(width, height)
    matrix = np.rot90(matrix, 1)
    print(matrix)



class IntArrayType:
    def from_param(self, param):
        typename = type(param).__name__
        if hasattr(self, 'from_' + typename):
            return getattr(self, 'from_' + typename)(param)
        elif isinstance(param, ctypes.Array):
            return param
        else:
            raise TypeError("Can't convert %s" % typename)

    # Cast from array.array objects
    def from_array(self, param):
        if param.typecode != 'i':
            raise TypeError('must be an array of ints')
        ptr, _ = param.buffer_info()
        return ctypes.cast(ptr, ctypes.POINTER(ctypes.c_int))

    # Cast from lists/tuples
    def from_list(self, param):
        val = ((ctypes.c_int)*len(param))(*param)
        return val

    from_tuple = from_list

    # Cast from a numpy array
    def from_ndarray(self, param):
        return param.ctypes.data_as(ctypes.POINTER(ctypes.c_int))