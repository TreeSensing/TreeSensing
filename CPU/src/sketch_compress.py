from ctypes import *
import os
import math
import numpy as np
from scipy.sparse import coo_matrix
import cvxpy as cvx
from enum import IntEnum
from tqdm import tqdm


class SketchType(IntEnum):
    CM = 0
    Count = 1
    CU = 2
    CMM = 3
    CML = 4
    CSM = 5

    @staticmethod
    def from_string(s):
        try:
            s = 'Count' if s.upper() == 'COUNT' else s.upper()
            return SketchType[s]
        except KeyError:
            raise ValueError()

    def __str__(self):
        return self.name


class CompAlgoType(IntEnum):
    HOKUSAI = 0
    ELASTIC = 1
    COMPSEN = 2
    CLUSRED = 3


INVALID_COUNTER = 1000000


class Sketch:
    """
    It encapsulates data structures in *Sketch.h. A Sketch object manipulates real sketch
    data structures by calling functions in the shared libraries (build/*Sktech.so) compiled
    from sketch_api.cpp.
    """

    def __init__(self, read_num, d, sketch_type, is_compsen=False):
        self.obj = getattr(lib, '{}Sketch_new'.format(str(sketch_type)))(read_num, d, is_compsen)
        self.sketch_name = '{} Sketch'.format(str(sketch_type))
        self.sketch_type = sketch_type
        self.d = d
        self.w = [0 for _ in range(d)]

    def __del__(self):
        getattr(lib, '{}Sketch_delete'.format(str(self.sketch_type)))(self.obj)

    def init_arr(self, d_i, in_data, w, w0, seed):
        self.w[d_i] = w
        lib.init_arr(self.obj, d_i, in_data, w, w0, seed)

    def copy_array(self, d_i, out_data):
        lib.copy_array(self.obj, d_i, out_data)

    def insert_dataset(self, start=0, end=-1):
        build_secs = lib.insert_dataset(self.obj, start, end)
        return build_secs

    def query_dataset(self, k, ignore_max=0):
        c_AAE = c_double()
        ARE = lib.query_dataset(self.obj, k, c_AAE, ignore_max)
        return ARE, c_AAE.value

    def compress(self, ratio, algo, sep_thld=0, round_thld=1, num_frags=300, num_threads=1, tower_shape=[], sbf_size=-1):
        c_tower_shape = [x for level in tower_shape for x in level]
        c_tower_shape.append(0)  # Terminator
        c_tower_shape_ptr = (c_int * len(c_tower_shape))(*c_tower_shape)

        c_tower_secs = c_double()
        secs = lib.compress(self.obj, ratio, algo, sep_thld, round_thld,
                            num_frags, num_threads, c_tower_shape_ptr, sbf_size, c_tower_secs)
        return secs, c_tower_secs.value

    def compress_4096_simd(self, ratio):
        lib.compress_4096_simd.restype = c_double
        lib.compress_4096_simd.argtypes = [c_void_p, c_int]
        secs = lib.compress_4096_simd(self.obj, ratio)
        return secs


def split_integer(a, b):
    assert a > 0 and b > 0 and b <= a
    quotient = int(a / b)
    remainder = a % b
    return [quotient] * (b - remainder) + [quotient + 1] * remainder


def ts_compress_array(x, ratio, num_frags):
    n = len(x)
    m = int(math.ceil(n / ratio))
    m1 = int(math.ceil(m / num_frags))
    nn = split_integer(n, num_frags)
    A_frags = []
    y_frags = []
    debug_frags_in = []

    i = 0
    for n1 in nn:
        x1 = x[i: i+n1]
        debug_frags_in.append(x1)
        A = np.random.binomial(1, 0.5, (m1, n1))
        A_frags.append(A)
        y = np.dot(A, x1)
        y_frags.append(y)
        i += n1
    return A_frags, y_frags, debug_frags_in


def ts_compress_array_gm(x, ratio, num_frags, p=1):
    n = len(x)
    m = int(math.ceil(n / ratio))
    m1 = int(math.ceil(m / num_frags))
    nn = split_integer(n, num_frags)
    A = []
    yy = []
    debug_frags_in = []

    if p <= 0 or p > 1:
        raise ValueError('p must in (0, 1] !')

    i = 0
    for n1 in nn:
        x1 = x[i: i+n1]
        debug_frags_in.append(x1)
        # generate GM
        count = int(m1*n1*p)
        rows = np.random.randint(0, m1, count)
        cols = np.random.randint(0, n1, count)
        data = np.random.randn(len(rows))
        A1 = coo_matrix((data, (rows, cols)), shape=(m1, n1)).toarray()
        A.append(A1)
        y1 = np.dot(A1, x1)
        yy.append(y1)
        i += n1
    return A, yy, debug_frags_in


def ts_decompress_array(A_frags, y_frags, has_neg=False):
    num_frags = len(A_frags)
    result = []
    debug_frags_out = []
    invalid = 0

    for i in tqdm(range(num_frags)):
        vx = cvx.Variable(A_frags[i].shape[1])
        objective = cvx.Minimize(cvx.norm(vx, 1))
        constraints = [A_frags[i] @ vx == y_frags[i]]
        if not has_neg:
            constraints.append(vx >= 0)
        prob = cvx.Problem(objective, constraints)
        prob.solve(verbose=False)
        res = [round(x) for x in vx.value]
        debug_frags_out.append(res)

        cnt = 0
        for e in res:
            if e > 0:
                cnt += 1
        if cnt * 2 >= len(y_frags[i]):
            res = [INVALID_COUNTER for _ in range(len(res))]
            invalid += 1

        result = result + res

    scc_ratio = (num_frags - invalid) / num_frags
    print("scc ratio %f" % scc_ratio)
    return result, debug_frags_out


def ts_decompress_array_l2(A, yy, has_neg=False):
    num_frags = len(A)
    result = []
    debug_frags_out = []
    invalid = 0

    # for i in tqdm(range(num_frags)):
    for i in range(num_frags):
        vx = cvx.Variable(A[i].shape[1])
        objective = cvx.Minimize(cvx.norm(vx, 0.5))
        constraints = [A[i] @ vx == yy[i]]
        if not has_neg:
            constraints.append(vx >= 0)
        prob = cvx.Problem(objective, constraints)
        prob.solve(verbose=False)
        res = [round(x) for x in vx.value]
        debug_frags_out.append(res)

        r1 = []
        cnt = 0
        cnt_ = 0
        for j in res:
            j = int(j)
            r1.append(j)
            if j > 0:
                cnt += 1
            if j < 0:
                cnt_ += 1
        if cnt * 2 >= len(yy[i]) or cnt_ > 0:
            r1 = [INVALID_COUNTER for _ in range(len(r1))]
            invalid += 1

        result = result + r1

    return result, invalid, debug_frags_out


def ts_decompress_array_somp(A, yy, has_neg=False):
    num_frags = len(A)
    result = []
    debug_frags_out = []
    n_nonzero_coefs = 200
    invalid = 0

    # for i in tqdm(range(num_frags)):
    for i in range(num_frags):
        omp = OrthogonalMatchingPursuit(
            n_nonzero_coefs=n_nonzero_coefs, normalize=False)
        omp.fit(A[i], yy[i])
        res = omp.coef_

        r1 = []
        cnt = 0
        cnt_ = 0
        for j in res:
            j = int(j)
            r1.append(j)
            if j > 0:
                cnt += 1
            if j < 0:
                cnt_ += 1
        if cnt * 2 >= len(yy[i]) or cnt_ > 0:
            r1 = [INVALID_COUNTER for _ in range(len(r1))]
            invalid += 1

        result = result + r1

    return result, invalid, debug_frags_out


def ts_decompress_array_irls(A, yy, has_neg=False, L=500):
    num_frags = len(A)
    result = []
    debug_frags_out = []
    invalid = 0

    # for i in tqdm(range(num_frags)):
    for i in range(num_frags):
        T_Mat = A[i]
        y = yy[i]
        hat_x_tp = np.dot(T_Mat.T, y)
        eps = 1
        p = 1  # solution for l-norm p
        times = 1
        while (eps > 10e-9) and (times < L):  # iter time
            weight = (hat_x_tp**2+eps)**(p/2-1)
            Q_Mat = np.diag(1/weight)
            # hat_x=Q_Mat*T_Mat'*inv(T_Mat*Q_Mat*T_Mat')*y
            temp = np.dot(np.dot(T_Mat, Q_Mat), T_Mat.T)
            temp = np.dot(np.dot(Q_Mat, T_Mat.T), np.linalg.inv(temp))
            hat_x = np.dot(temp, y)
            if(np.linalg.norm(hat_x-hat_x_tp, 2) < np.sqrt(eps)/100):
                eps = eps/10
            hat_x_tp = hat_x
            times = times+1

        r1 = []
        cnt = 0
        cnt_ = 0
        for j in hat_x:
            j = int(j)
            r1.append(j)
            if j > 0:
                cnt += 1
            if j < 0:
                cnt_ += 1
        if cnt * 2 >= len(yy[i]) or cnt_ > 0:
            r1 = [INVALID_COUNTER for _ in range(len(r1))]
            invalid += 1

        result = result + r1

    return result, invalid, debug_frags_out


def copy_A_and_y_frags(sketch, d_i, ratio, num_frags):
    m = int(math.ceil(sketch.w[d_i] / ratio))
    m1 = int(math.ceil(m / num_frags))
    nn = split_integer(sketch.w[d_i], num_frags)
    A_size = 0
    for n1 in nn:
        A_size += m1 * n1
    A_data = (c_int * A_size)()
    yy_data = (c_int * (m1 * num_frags))()

    lib.copy_A_and_y_frags(sketch.obj, d_i, A_data, yy_data)

    A_arr = np.array(A_data)
    yy_arr = np.array(yy_data)

    A_i = 0
    A = []
    for n1 in nn:
        A_frag = []
        for _ in range(m1):
            row = []
            for _ in range(n1):
                row.append(A_arr[A_i])
                A_i += 1
            A_frag.append(row)
        A.append(np.array(A_frag))
    yy_i = 0
    yy = []
    for _ in range(num_frags):
        yy_frag = []
        for __ in range(m1):
            yy_frag.append(yy_arr[yy_i])
            yy_i += 1
        yy.append(np.array(yy_frag))
    return A, yy


def compressive_sensing(sketch_type, read_num, d, w, seed, sep_thld, round_thld, ratio,
                        num_frags, k, num_threads, do_query=True, decompress_method='python',
                        func=None, tower_shape=[], ignore_level=None, sbf_size=-1, extra_ks=[]):
    sketch = Sketch(read_num, d, sketch_type)
    for i in range(d):
        sketch.init_arr(i, None, w[i], -1, seed + i)
    build_secs = sketch.insert_dataset()

    # Debug: Record counters just after insert_dataset()
    debug_all_counters_ds = [[0 for j in range(w[i])] for i in range(d)]
    debug_large_counters_ds = [[0 for j in range(w[i])] for i in range(d)]
    debug_small_counters_ds = [[0 for j in range(w[i])] for i in range(d)]
    for i in range(d):
        temp = (c_uint * w[i])()
        sketch.copy_array(i, temp)
        temp = np.array(temp)
        debug_all_counters_ds[i] = temp
        for j in range(len(temp)):
            if temp[j] < sep_thld:
                debug_small_counters_ds[i][j] = temp[j]
            else:
                debug_large_counters_ds[i][j] = temp[j]
        # print("d={}, small nonzero counters={}, large nonzero counters={}, total={}".format(i, np.count_nonzero(debug_small_counters_ds[i]), np.count_nonzero(debug_large_counters_ds[i]), w[i]))

    # Compress
    comp_secs, tower_comp_secs = sketch.compress(ratio, CompAlgoType.COMPSEN, sep_thld, round_thld,
                                                 num_frags, num_threads, tower_shape, sbf_size)

    # Do something with the compressed result
    if func:
        A_frags_ds, y_frags_ds = [], []
        for i in range(d):
            A_frags, y_frags = copy_A_and_y_frags(sketch, i, ratio, num_frags)
            A_frags_ds.append(A_frags)
            y_frags_ds.append(y_frags)
        A_frags_ds, y_frags_ds = func(A_frags_ds, y_frags_ds)

    # Decompress and query
    decomp_secs, tower_decomp_secs = -1, -1
    ARE, AAE = -1, -1
    CE = 0  # compression error
    extra_AREs = []

    if decompress_method == 'cpp':
        decomp_secs = lib.ts_decompress(sketch.obj, num_threads)
        if do_query:
            ARE, AAE = sketch.query_dataset(k)

    elif decompress_method == 'python':
        sketch_copy = Sketch(read_num, d, sketch_type, is_compsen=True)
        tower_decomp_secs = 0
        for i in range(d):
            # Debug: Skip decompressing
            # res = debug_large_counters_ds[i]
            # for j, v in enumerate(debug_small_counters_ds[i]):
            #     res[j] += v

            # Recover large counters with SketchSensing
            A_frags, y_frags = copy_A_and_y_frags(sketch, i, ratio, num_frags)
            res, debug_frags_out = ts_decompress_array(A_frags, y_frags, has_neg=(sketch_type == SketchType.Count))

            # Recover small counters with TowerTree
            if tower_shape:
                small_data = (c_uint * w[i])()
                tower_decomp_secs += lib.copy_tower_small_counters(sketch.obj, i, small_data,
                                                                   ignore_level if ignore_level is not None else 0)
                small_counters = np.array(small_data)
                for j in range(len(res)):
                    res[j] += small_counters[j]

            CE += abs(np.array(res) - np.array(debug_all_counters_ds[i])).sum()

            out_data = (c_int * w[i])(*res) if sketch_type == SketchType.Count else (c_uint * w[i])(*res)
            sketch_copy.init_arr(i, out_data, w[i], -1, seed+i)
        if do_query:
            ignore_max = 256 if ignore_level is not None else 0
            ARE, AAE = sketch_copy.query_dataset(k, ignore_max)
            for exk in extra_ks:
                exARE, exAAE = sketch_copy.query_dataset(exk)
                extra_AREs.append(exARE)

    CE /= sum(w)

    ret = {'ARE': ARE, 'AAE': AAE, 'CE': CE, 'comp_secs': comp_secs, 'decomp_secs': decomp_secs,
           'tower_comp_secs': tower_comp_secs, 'tower_decomp_secs': tower_decomp_secs, 'extra_AREs': extra_AREs,
           'build_secs': build_secs}
    return ret


def compressive_sensing_comp_simd(read_num, d, w, seed, ratio):
    sketch = Sketch(read_num, d, SketchType.CM)
    for i in range(d):
        sketch.init_arr(i, None, w[i], -1, seed + i)
    sketch.insert_dataset()

    total_comp_secs = sketch.compress_4096_simd(ratio)

    ret = {'total_comp_secs': total_comp_secs}
    return ret


def hokusai(sketch_type, read_num, d, w, seed, ratio, k):
    sketch = Sketch(read_num, d, sketch_type)
    for i in range(d):
        sketch.init_arr(i, None, w[i], -1, seed+i)
    sketch.insert_dataset()

    secs_used = sketch.compress(ratio, CompAlgoType.HOKUSAI)[0]
    ARE, AAE = sketch.query_dataset(k)

    ret = {'ARE': ARE, 'AAE': AAE, 'comp_secs': secs_used}
    return ret


def elastic(sketch_type, read_num, d, w, seed, ratio, k):
    assert sketch_type != SketchType.Count
    sketch = Sketch(read_num, d, sketch_type)
    for i in range(d):
        sketch.init_arr(i, None, w[i], -1, seed+i)
    sketch.insert_dataset()

    secs_used = sketch.compress(ratio, CompAlgoType.ELASTIC)[0]
    ARE, AAE = sketch.query_dataset(k)

    ret = {'ARE': ARE, 'AAE': AAE, 'comp_secs': secs_used}
    return ret


def cluster_reduce(sketch_type, read_num, d, w, ratio, k, method_id, debug=False):
    secs_used = c_double()
    ARE = crlib.run_test(read_num, d, sum(w), ratio, k, int(sketch_type), secs_used, method_id, debug)
    ret = {'ARE': ARE, 'comp_secs': secs_used.value}
    return ret


def no_compress(sketch_type, read_num, d, w, seed, k):
    sketch = Sketch(read_num, d, sketch_type)
    for i in range(d):
        sketch.init_arr(i, None, w[i], -1, seed+i)
    sketch.insert_dataset()
    ARE, AAE = sketch.query_dataset(k)
    ret = {'ARE': ARE, 'AAE': AAE}
    return ret


def init(args, lib_dir='./build'):
    """
    Load shared libraries, initialize API calls, load global_cfg and normalize parameters.
    """
    np.random.seed(0)

    global lib
    lib = cdll.LoadLibrary(os.path.join(lib_dir, '{}Sketch.so').format(str(args.sketch)))
    getattr(lib, '{}Sketch_new'.format(str(args.sketch))).restype = c_void_p
    getattr(lib, '{}Sketch_new'.format(str(args.sketch))).argtypes = [c_int32, c_int32, c_bool]
    getattr(lib, '{}Sketch_delete'.format(str(args.sketch))).restype = None
    getattr(lib, '{}Sketch_delete'.format(str(args.sketch))).argtypes = [c_void_p]

    lib.init_arr.restype = None
    lib.init_arr.argtypes = [c_void_p, c_int32, c_void_p, c_int32, c_int32, c_uint32]
    lib.insert_dataset.restype = c_double
    lib.insert_dataset.argtypes = [c_void_p, c_int, c_int]
    lib.query_dataset.restype = c_double
    lib.query_dataset.argtypes = [c_void_p, c_int32, POINTER(c_double), c_int]
    lib.copy_array.restype = None
    lib.copy_array.argtypes = [c_void_p, c_int32, c_void_p]
    lib.compress.restype = c_double
    lib.compress.argtypes = [c_void_p, c_int, c_int, c_int, c_int,
                             c_int, c_int, POINTER(c_int), c_int, POINTER(c_double)]
    lib.ts_decompress.restype = c_double
    lib.ts_decompress.argtypes = [c_void_p, c_int]
    lib.copy_A_and_y_frags.restype = None
    lib.copy_A_and_y_frags.argtypes = [c_void_p, c_int, POINTER(c_int), POINTER(c_int)]
    lib.copy_tower_small_counters.restype = c_double
    lib.copy_tower_small_counters.argtypes = [c_void_p, c_int, c_void_p, c_int]
    lib.query_and_copy.restype = None
    lib.query_and_copy.argtypes = [c_void_p, c_int, POINTER(c_int)]
    lib.calc_acc.restype = c_double
    lib.calc_acc.argtypes = [c_int, POINTER(c_int), POINTER(c_double)]
    lib.set_debug_level(args.debug)
    lib.init_config.restype = None
    lib.init_config.argtypes = [c_char_p]

    global crlib
    crlib = cdll.LoadLibrary(os.path.join(lib_dir, 'ClusterReduceWrapper.so'))
    crlib.run_test.restype = c_double
    crlib.run_test.argtypes = [c_int, c_int, c_int, c_int, c_int, c_int, POINTER(c_double), c_int, c_bool]
    crlib.distributed_data_stream.restype = c_double
    crlib.distributed_data_stream.argtypes = [c_int, c_int, c_int, POINTER(c_double), c_int]
    crlib.init_config.restype = None
    crlib.init_config.argtypes = [c_char_p]

    # Load global_cfg for C++ libraries
    c_cfg = args.cfg.encode('utf-8')
    lib.init_config(c_cfg)
    crlib.init_config(c_cfg)

    # Normalize sep_thld for CML and CSM sketches
    sep_thld_modified = False
    if args.sketch == SketchType.CML:
        log_base = 1.00026
        args.sep_thld = int(math.log(1 - args.sep_thld * (1 - log_base), log_base))
        sep_thld_modified = True
    elif args.sketch == SketchType.CSM:
        args.sep_thld = int(args.sep_thld / args.d)
        sep_thld_modified = True
    if sep_thld_modified:
        print("sep_thld is modified to {} due to probabilistically insertion".format(args.sep_thld))

    print("==============================================================")
    print("\033[1;32mTest case {} starting...\033[0m".format(args.test))
    return lib, crlib
