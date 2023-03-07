import sys
sys.path.append('../../../CPU')

import os
import json
import pandas as pd
from src.sketch_compress import *


def save_csv(df, filename, outdir=None):
    if outdir is None:
        outdir = global_cfg["test_result_dir"]
    if not os.path.exists(outdir):
        os.mkdir(outdir)
    path = os.path.join(outdir, filename)
    df.to_csv(path, index=False, sep=',')
    print("Results saved to \033[92m{}\033[0m".format(path))


def distributed_data_stream(sketch_type, w, k, ratio, algo, cr_method_id, tower_shape, sep_thld):
    if algo == CompAlgoType.CLUSRED:
        c_AAE = c_double()
        ARE = crlib.distributed_data_stream(sum(w), k, ratio, c_AAE, cr_method_id)
        AAE = c_AAE.value
    else:
        slice_idxs = [0, 3390215, 6780430, 10170645, 13560860, 16951075, 20341290, 23731505, -1]
        sketches = []
        query_result = np.zeros(k, dtype=int)

        for sk_i in range(8):
            print("Sketches {} / {}".format(sk_i, 8))
            sketch = Sketch(-1, 3, sketch_type)
            for i in range(3):
                sketch.init_arr(i, None, w[i], -1, 997+i)
            sketch.insert_dataset(slice_idxs[sk_i], slice_idxs[sk_i+1])
            ##### debug begin #####
            # debug_large_counters_ds = [[0 for j in range(w[i])] for i in range(3)]
            # debug_small_counters_ds = [[0 for j in range(w[i])] for i in range(3)]
            # for i in range(3):
            #     temp = (c_uint * w[i])()
            #     sketch.copy_array(i, temp)
            #     temp = np.array(temp)
            #     for j in range(len(temp)):
            #         if temp[j] < sep_thld:
            #             debug_small_counters_ds[i][j] = temp[j]
            #         else:
            #             debug_large_counters_ds[i][j] = temp[j]
            ##### debug end #####

            # Compress one sketch
            sketch.compress(ratio, algo, sep_thld, 1, w[i]//250, tower_shape=tower_shape)
            sketches.append(sketch)

            # Decompress and query one sketch
            if algo == CompAlgoType.COMPSEN:
                sketch_copy = Sketch(-1, 3, sketch_type, is_compsen=True)
                for i in range(3):
                    # res = debug_large_counters_ds[i]
                    # for j, v in enumerate(debug_small_counters_ds[i]):
                    #     res[j] += v

                    A_frags, y_frags = copy_A_and_y_frags(sketches[sk_i], i, ratio, w[i]//250)
                    res, debug_frags_out = ts_decompress_array(
                        A_frags, y_frags, has_neg=(sketch_type == SketchType.Count))

                    if tower_shape:
                        small_data = (c_uint * w[i])()
                        lib.copy_tower_small_counters(sketch.obj, i, small_data, 0)
                        small_counters = np.array(small_data)
                        for j in range(len(res)):
                            res[j] += small_counters[j]

                    out_data = (c_int * w[i])(*res) if sketch_type == SketchType.Count else (c_uint * w[i])(*res)
                    sketch_copy.init_arr(i, out_data, w[i], -1, 997+i)
                query_data = (c_int * k)()
                lib.query_and_copy(sketch_copy.obj, k, query_data)
            elif algo == CompAlgoType.HOKUSAI or algo == CompAlgoType.ELASTIC:
                query_data = (c_int * k)()
                lib.query_and_copy(sketches[sk_i].obj, k, query_data)
            query_result += np.array(query_data)

        c_AAE = c_double()
        ARE = lib.calc_acc(k, (c_int * k)(*query_result), c_AAE)
        AAE = c_AAE.value

    ret = {'ARE': ARE, 'AAE': AAE}
    return ret


def distributed_data_stream_baseline(sketch_type, w, k):
    slice_idxs = [0, 3390215, 6780430, 10170645, 13560860, 16951075, 20341290, 23731505, -1]
    sketches = []
    query_result = np.zeros(k, dtype=int)

    for sk_i in range(8):
        print("Sketches {} / {}".format(sk_i, 8))
        sketch = Sketch(-1, 3, sketch_type)
        for i in range(3):
            sketch.init_arr(i, None, w[i], -1, 997+i)
        sketch.insert_dataset(slice_idxs[sk_i], slice_idxs[sk_i+1])
        sketches.append(sketch)

        query_data = (c_int * k)()
        lib.query_and_copy(sketches[sk_i].obj, k, query_data)
        query_result += np.array(query_data)

    ARE = lib.calc_acc(k, (c_int * k)(*query_result), None)
    print("w = {}, ARE = {}".format(w, ARE))


###### Test cases ######

def test_distributed_app():
    algos = [CompAlgoType.COMPSEN, CompAlgoType.HOKUSAI, CompAlgoType.ELASTIC, CompAlgoType.CLUSRED]
    wns = range(10000, 110000, 10000)  # x-axis
    df_ARE = {'w': [], **{algo: [] for algo in algos}}
    ratio = 6

    for wn in wns:
        df_ARE['w'].append(wn)

        for algo in algos:
            if args.sketch == SketchType.Count and algo == CompAlgoType.ELASTIC:
                df_ARE[algo].append(np.nan)
                continue

            w = [wn] * args.d
            ret = distributed_data_stream(args.sketch, w, args.k, ratio, algo, args.cr_method, [], 200)
            print(algo, wn, ret)
            df_ARE[algo].append(ret['ARE'])
        save_csv(pd.DataFrame(df_ARE), "ARE_{}_distributed.csv".format(args.sketch))

    save_csv(pd.DataFrame(df_ARE), "ARE_{}_distributed.csv".format(args.sketch))


def test_full_distributed_app():
    algos = [CompAlgoType.COMPSEN, CompAlgoType.HOKUSAI, CompAlgoType.ELASTIC, CompAlgoType.CLUSRED]
    wns = range(200000, 1200000, 100000)  # x-axis
    df_ARE = {'w': [], **{algo: [] for algo in algos}}

    for wn in wns:
        df_ARE['w'].append(wn)

        for algo in algos:
            ratio = 6 if algo == CompAlgoType.COMPSEN else 2
            print(wn, algo, ratio)
            if args.sketch == SketchType.Count and algo == CompAlgoType.ELASTIC:
                df_ARE[algo].append(np.nan)
                continue

            w = [wn] * args.d
            ret = distributed_data_stream(args.sketch, w, 253906, ratio, algo, args.cr_method, [(4, 4), (8, 1)], 4096)
            print(algo, wn, ret)
            df_ARE[algo].append(ret['ARE'])
        save_csv(pd.DataFrame(df_ARE), "ARE_{}_full_distributed.csv".format(args.sketch))

    save_csv(pd.DataFrame(df_ARE), "ARE_{}_full_distributed.csv".format(args.sketch))


def test_distributed_baseline():
    curr = 0
    for wn in range(10000, 110000, 10000):
        print("\033[1;34m[{}]\033[0m w = {}".format(curr, wn))
        w = [wn] * args.d
        ret = distributed_data_stream_baseline(args.sketch, w, args.k)
        print(ret)
        curr += 1
    for wn in list(range(200000, 1200000, 100000)):
        print("\033[1;34m[{}]\033[0m w = {}".format(curr, wn))
        w = [wn] * args.d
        ret = distributed_data_stream_baseline(args.sketch, w, 253906)
        print(ret)
        curr += 1


class PseudoArgs:
    def __init__(self, k):
        self.cfg = '../../../CPU/config.json'
        self.sketch = SketchType.CM
        self.d = 3
        self.k = k
        self.cr_method = 1
        self.debug = False
        self.test = 'test_accuracy'


if __name__ == '__main__':
    global args
    args = PseudoArgs(k=500)

    global global_cfg
    with open(args.cfg) as f:
        global_cfg = json.load(f)

    global lib, crlib
    lib, crlib = init(args, '../../../CPU/build')

    test_full_distributed_app()
    test_distributed_baseline()
