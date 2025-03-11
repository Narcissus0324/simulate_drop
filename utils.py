import numpy as np
import ctypes
import time
import copy

def show_matrix(arr, width, height):
    """
    将一维数组 arr (长度=width*height)
    转化成 width x height 矩阵后打印
    注意 row=0 在最下方, 这里为了方便观看, 需要旋转一下
    """
    matrix = np.array(arr).reshape((width, height))
    # 旋转 90 度，让 row=0 显示在底部行
    matrix = np.rot90(matrix, k=1)
    print(matrix)

class IntArrayType:
    """
    帮助把 Python 中的 list/tuple/numpy.array 等类型
    转化为 ctypes.c_int * n
    """
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

def arrays_equal(a, b):
    """
    比较两个一维列表是否逐元素相等
    """
    if len(a) != len(b):
        return False
    return all(x == y for x, y in zip(a, b))


def simulate_drop_multi(dll, matrix, width, height):
    """
    Python 封装: 调用 C 库中的 simulate_drop_multi
    传入:
      - dll:      已加载的动态库对象(ctypes.CDLL)
      - matrix:   Python列表(或numpy数组), 表示盘面
      - width, height: 矩阵尺寸
    返回:
      - 掉落后的一维列表(与传入大小相同)
    """
    c_arr = IntArrayType().from_param(matrix)
    dll.simulate_drop_multi(c_arr, width, height)
    return list(c_arr)


def simulate_drop_single(dll, matrix, width, height):
    """
    Python 封装: 调用 C 库中的 simulate_drop_single
    """
    c_arr = IntArrayType().from_param(matrix)
    dll.simulate_drop_single(c_arr, width, height)
    return list(c_arr)

def simulate_drop_queue(dll, matrix, width, height):
    c_arr = IntArrayType().from_param(matrix)
    dll.simulate_drop_queue(c_arr, width, height)
    return list(c_arr)


def simulate_drop_active(dll, matrix, width, height):
    c_arr = IntArrayType().from_param(matrix)
    dll.simulate_drop_active(c_arr, width, height)
    return list(c_arr)


def run_test_case_all_methods(dll, test_name, data_list, width, height, sim_funcs=None):
    """
    自动运行一次测试，比较所有的 simulate_drop 方法。
    参数:
      - dll: 已加载的动态库对象
      - test_name: 测试名称
      - data_list: 表示盘面的一维列表
      - width, height: 矩阵尺寸
      - sim_funcs: 一个字典，key 为算法名称，value 为对应的 Python 封装函数。
                   默认使用 { "multi":simulate_drop_multi, "single":simulate_drop_single, "queue":simulate_drop_active }
    返回一个字典，其中包含初始矩阵、各算法的结果、各自的耗时以及算法间的对比结果。
    """
    if sim_funcs is None:
        sim_funcs = {
            "multi": simulate_drop_multi,
            "single": simulate_drop_single,
            "active": simulate_drop_active,
            "queue": simulate_drop_queue,
        }
        
    initial_matrix = copy.deepcopy(data_list)
    results = {}
    timings = {}
    
    for name, func in sim_funcs.items():
        matrix_copy = copy.deepcopy(data_list)
        t1 = time.time()
        res = func(dll, matrix_copy, width, height)
        t2 = time.time()
        results[name] = res
        timings[name] = t2 - t1
    
    # 自动计算所有算法间的两两比较结果
    equal_results = {}
    keys = list(sim_funcs.keys())
    for i in range(len(keys)):
        for j in range(i+1, len(keys)):
            key1, key2 = keys[i], keys[j]
            eq = arrays_equal(results[key1], results[key2])
            equal_results[f"{key1} vs {key2}"] = eq

    return {
        "test_name": test_name,
        "width": width,
        "height": height,
        "initial_matrix": initial_matrix,
        "results": results,
        "timings": timings,
        "equal_results": equal_results,
        "sim_funcs": list(sim_funcs.keys())
    }