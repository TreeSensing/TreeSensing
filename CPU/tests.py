import os
import argparse
import json
import numpy as np
import pandas as pd

from src.sketch_compress import *


# Pre-defined tower shapes.
# Shape #1, #2 and #3 are three recommended shapes in our paper.
tower_id2shape = {
    -1: [],  # Disable TowerEncoding

    1: [(4, 4), (8, 1)],          # Shape #1
    2: [(4, 2), (4, 2), (4, 1)],  # Shape #2
    3: [(8, 8), (16, 1)],         # Shape #3

    10: [(8, 4), (8, 1)],
    11: [(8, 4), (4, 1)],
    12: [(8, 4), (2, 1)],
    13: [(4, 4), (8, 1)],
    14: [(4, 4), (4, 1)],
    15: [(4, 4), (2, 1)],
    16: [(2, 4), (8, 1)],
    17: [(2, 4), (4, 1)],
    18: [(2, 4), (2, 1)],
}


def reject_outliers(arr, m=2.):
    """
    Remove outliers from arr.
    """
    arr = np.array(arr)
    d = np.abs(arr - np.median(arr))
    mdev = np.median(d)
    s = d/mdev if mdev else 0.
    return list(arr[s < m])


def save_csv(df, filename, outdir=None):
    if outdir is None:
        outdir = global_cfg["test_result_dir"]
    if not os.path.exists(outdir):
        os.mkdir(outdir)
    path = os.path.join(outdir, filename)
    df.to_csv(path, index=False, sep=',')
    print("Results saved to \033[92m{}\033[0m".format(path))


###### Below are test cases ######

def test_acc_mem():
    """
    Top-k Accuracy vs. Compression Ratio
    Metrics:
        Top-k accuracy (ARE)
    Variables:
        1. Compression ratio
        2. Memory (width)
    """
    wns = [60000, 80000, 100000, 120000]  # Lines
    ratios = range(1, 11)  # x-axis
    df_ARE = {'ratio': ratios, **{wn: [] for wn in wns}}

    total = len(wns) * len(ratios)
    curr = 0

    for wn in wns:
        w = [wn] * args.d
        for ratio in ratios:
            print("\033[1;34m[{} / {}]\033[0m w = {}, ratio = {}".format(curr, total, wn, ratio))

            if ratio == 1:
                ret = no_compress(args.sketch, args.read_num, args.d, w, args.seed, args.k)
            else:
                ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, args.sep_thld,
                                          args.round_thld, ratio, wn // 250, args.k, args.num_threads)
            df_ARE[wn].append(ret['ARE'])

            print("ARE = {}".format(ret['ARE']))
            curr += 1

    save_csv(pd.DataFrame(df_ARE), 'ARE_{}_mem.csv'.format(str(args.sketch)))


def test_fullacc_towershape():
    """
    Full Accuracy vs. Tower Shape
    Metrics:
        Full accuracy (ARE)
    Variables:
        1. Memory (width)
        2. Tower shape
    """
    wns = range(200000, 1200000, 100000)  # x-axis
    df_ARE = {'w': wns, 'no_compress': [], **{tower_shape_id: [] for tower_shape_id in range(1, 4)}}

    total = len(wns) * (3 + 1)
    curr = 0

    for wn in wns:
        print("\033[1;34m[{} / {}]\033[0m w = {}, tower shape = {}".format(curr, total, wn, "baseline"))

        w = [wn] * args.d
        ret = no_compress(args.sketch, args.read_num, args.d, w, args.seed, -1)
        df_ARE['no_compress'].append(ret['ARE'])

        print("ARE = {}".format(ret['ARE']))
        curr += 1

    for tower_shape_id in range(1, 4):
        for wn in wns:
            print("\033[1;34m[{} / {}]\033[0m w = {}, tower shape = #{}".format(curr, total, wn, tower_shape_id))

            w = [wn] * args.d
            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, 4096, args.round_thld, 4, wn // 250, -1,
                                      args.num_threads, decompress_method='python', tower_shape=tower_id2shape[tower_shape_id])
            df_ARE[tower_shape_id].append(ret['ARE'])

            print("ARE = {}".format(ret['ARE']))
            curr += 1

    save_csv(pd.DataFrame(df_ARE), 'ARE_{}_fullacc_towershape.csv'.format(str(args.sketch)))


def test_acc_separating_k():
    """
    Top-k and Full Accuracy vs. Separating Threshold
    Metrics:
        Accuracy (ARE)
    Variables:
        1. Separating threshold
        2. k
    """
    ks = [500, 1000, 2000, -1]  # Lines
    sep_thlds = [64, 128, 256, 512, 1024, 2048, 4096, 8192]
    df_ARE = {'sep_thld': [], **{k: [] for k in ks}}

    wn = 1000000
    w = [wn] * args.d

    total = len(ks) * len(sep_thlds)
    curr = 0

    for sep_thld in sep_thlds:
        df_ARE['sep_thld'].append(sep_thld)

        for k in ks:
            print("\033[1;34m[{} / {}]\033[0m k = {}, sep_thld = {}".format(curr, total, k, sep_thld))

            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, sep_thld, args.round_thld,
                                      8, wn // 250, k, args.num_threads, tower_shape=tower_id2shape[1])
            df_ARE[k].append(ret['ARE'])

            print("ARE = {}".format(ret['ARE']))
            curr += 1

        save_csv(pd.DataFrame(df_ARE), 'X_ARE_{}_separating_k.csv'.format(str(args.sketch)))

    save_csv(pd.DataFrame(df_ARE), 'X_ARE_{}_separating_k.csv'.format(str(args.sketch)))


def test_acc_rounding():
    """
    Top-k Accuracy vs. Rounding Parameter
    Metrics:
        Top-k accuracy (ARE)
    Variables:
        1. Memory (width)
        2. Rounding parameter
    """
    round_thlds = [1, 2, 4, 8]  # Lines
    wns = range(60000, 130000, 10000)  # x-axis
    df_ARE = {'w': wns, **{round_thld: [] for round_thld in round_thlds}}

    total = len(round_thlds) * len(wns)
    curr = 0

    for round_thld in round_thlds:
        for wn in wns:
            print("\033[1;34m[{} / {}]\033[0m round_thld = {}, w = {}".format(curr, total, round_thld, wn))
            w = [wn] * args.d

            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed,
                                      args.sep_thld, round_thld, 2, wn // 250, args.k, args.num_threads)
            df_ARE[round_thld].append(ret['ARE'])

            print("ARE = {}".format(ret['ARE']))
            curr += 1

    save_csv(pd.DataFrame(df_ARE), 'ARE_{}_rounding.csv'.format(str(args.sketch)))


def test_acc_ignore_level():
    """
    Flexibility of TowerEncoding
    Metrics:
        Full accuracy (ARE)
    Variables:
        1. Memory (width)
        2. Number of layers to be ignored
    """
    ignore_levels = [2, 1, 0]  # Lines
    wns = range(200000, 1200000, 100000)  # x-axis
    df_ARE = {'w': wns, **{ignore_level: [] for ignore_level in ignore_levels}}

    total = len(ignore_levels) * len(wns)
    curr = 0

    for ignore_level in ignore_levels:
        for wn in wns:
            print("\033[1;34m[{} / {}]\033[0m ignore_level = {}, w = {}".format(curr, total, ignore_level, wn))
            w = [wn] * args.d

            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, 4096, args.round_thld,
                                      4, wn//250, -1, args.num_threads, tower_shape=tower_id2shape[2], ignore_level=ignore_level)
            df_ARE[ignore_level].append(ret['ARE'])

            print("ARE = {}".format(ret['ARE']))
            curr += 1

    save_csv(pd.DataFrame(df_ARE), 'ARE_{}_ignore_level.csv'.format(str(args.sketch)))


def test_time_towerencoding():
    """
    Efficiency of TowerEncoding
    Metrics:
        1. Compression time
        2. Recovery time
    Variables:
        1. Memory (width)
        2. Tower shape
    """
    wns = range(200000, 1200000, 100000)  # x-axis
    df_comp = {'w': [], **{tower_shape_id: [] for tower_shape_id in range(1, 4)}, 'build_secs': []}
    df_decomp = {'w': [], **{tower_shape_id: [] for tower_shape_id in range(1, 4)}}

    total = len(wns) * 3
    curr = 0

    for wn in wns:
        w = [wn] * args.d
        df_comp['w'].append(wn)
        df_decomp['w'].append(wn)

        for tower_shape_id in range(1, 4):
            print("\033[1;34m[{} / {}]\033[0m w = {}, tower shape = #{}".format(curr, total, wn, tower_shape_id))

            temp_build = []
            temp_comp = []
            temp_decomp = []
            for rep in range(3):
                ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, 4096, args.round_thld, 4, wn // 250, -1,
                                          args.num_threads, do_query=False, decompress_method='python', tower_shape=tower_id2shape[tower_shape_id])
                # Tip: `res = [] * w[i]` to speed up this test
                temp_build.append(ret['build_secs'])
                temp_comp.append(ret['tower_comp_secs'])
                temp_decomp.append(ret['tower_decomp_secs'])
                print("({}) Build time = {} s, compression time = {} s, decompression time = {} s".format(
                    rep, ret['build_secs'], ret['tower_comp_secs'], ret['tower_decomp_secs']))

            df_comp[tower_shape_id].append(np.average(reject_outliers(temp_comp)))
            df_decomp[tower_shape_id].append(np.average(reject_outliers(temp_decomp)))
            curr += 1
        df_comp['build_secs'].append(np.average(reject_outliers(temp_build)))  # Last time's build time

    save_csv(pd.DataFrame(df_comp), 'SECS_{}_tower_comp.csv'.format(str(args.sketch)))
    save_csv(pd.DataFrame(df_decomp), 'SECS_{}_tower_decomp.csv'.format(str(args.sketch)))


def test_time_sketchsensing():
    """
    Efficiency of SketchSensing
    Metrics:
        1. Compression time
        2. Recovery time
    Variables:
        1. Memory (width)
        2. Compression ratio
    """
    ratios = [4, 6, 8, 10]  # Lines
    wns = range(60000, 130000, 10000)  # x-axis
    df_comp = {'w': wns, **{ratio: [] for ratio in ratios}, 'build_secs': []}
    df_decomp = {'w': wns, **{ratio: [] for ratio in ratios}}

    total = len(ratios) * len(wns)
    curr = 0

    for wn in wns:
        for ratio in ratios:
            print("\033[1;34m[{} / {}]\033[0m ratio = {}, w = {}".format(curr, total, ratio, wn))
            w = [wn] * args.d

            temp_build = []
            temp_comp = []
            temp_decomp = []
            for rep in range(3):
                ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, args.sep_thld,
                                          args.round_thld, ratio, wn // 250, args.k, args.num_threads,
                                          do_query=False, decompress_method='cpp')
                temp_build.append(ret['build_secs'])
                temp_comp.append(ret['comp_secs'])
                temp_decomp.append(ret['decomp_secs'])
                print("({}) Build time = {} s, compression time = {} s, decompression time = {} s".format(
                    rep, ret['build_secs'], ret['comp_secs'], ret['decomp_secs']))

            df_comp[ratio].append(np.average(reject_outliers(temp_comp)))
            df_decomp[ratio].append(np.average(reject_outliers(temp_decomp)))
            curr += 1
        df_comp['build_secs'].append(np.average(reject_outliers(temp_build)))  # Last time's build time

    save_csv(pd.DataFrame(df_comp), 'SECS_{}_sensing_comp.csv'.format(str(args.sketch)))
    save_csv(pd.DataFrame(df_decomp), 'SECS_{}_sensing_decomp.csv'.format(str(args.sketch)))


def test_acc_algos():
    """
    Top-k Accuracy (Comparison with Prior Art)
    Metrics:
        Top-k accuracy (ARE)
    Variables:
        1. Compression ratio
        2. Compression method (ours and prior art)
    """
    ratios = range(2, 11)  # x-axis
    df_ARE = {'ratio': ratios, 'ours': [], 'hokusai': [], 'elastic': [], 'cluster_reduce': [], 'no_compress': []}

    ret = no_compress(args.sketch, args.read_num, args.d, args.w, args.seed, args.k)
    df_ARE['no_compress'] = [ret['ARE']] * len(ratios)
    print("No compression: ARE = {}".format(ret['ARE']))

    total = len(ratios)
    curr = 0

    for ratio in ratios:
        print("\033[1;34m[{} / {}]\033[0m ratio = {}".format(curr, total, ratio))

        ret = compressive_sensing(args.sketch, args.read_num, args.d, args.w, args.seed,
                                  args.sep_thld, args.round_thld, ratio, args.w[0] // 250, args.k, args.num_threads)
        df_ARE['ours'].append(ret['ARE'])
        print("Compressive sensing: ARE = {}".format(ret['ARE']))

        ret = hokusai(args.sketch, args.read_num, args.d, args.w, args.seed, ratio, args.k)
        df_ARE['hokusai'].append(ret['ARE'])
        print("Hokusai: ARE = {}".format(ret['ARE']))

        if args.sketch == SketchType.Count:
            df_ARE['elastic'].append(np.nan)
        else:
            ret = elastic(args.sketch, args.read_num, args.d, args.w, args.seed, ratio, args.k)
            df_ARE['elastic'].append(ret['ARE'])
            print("Elastic: ARE = {}".format(ret['ARE']))

        ret = cluster_reduce(args.sketch, args.read_num, args.d, args.w, ratio, args.k, args.cr_method, args.debug)
        df_ARE['cluster_reduce'].append(ret['ARE'])
        print("Cluster Reduce: ARE = {}".format(ret['ARE']))
        curr += 1

    save_csv(pd.DataFrame(df_ARE), 'ARE_{}_algos.csv'.format(str(args.sketch)))


def test_fullacc_algos():
    """
    Full Accuracy (Comparison with Prior Art)
    Metrics:
        Full accuracy (ARE)
    Variables:
        1. Memory (width)
        2. Compression method (ours and prior art)
    """
    wns = range(200000, 1200000, 100000)  # x-axis
    df_ARE = {'w': wns, 'ours': [], 'hokusai': [], 'elastic': [], 'cluster_reduce': [], 'no_compress': []}

    total = len(wns)
    curr = 0

    for wn in wns:
        print("\033[1;34m[{} / {}]\033[0m w = {}".format(curr, total, wn))
        w = [wn] * args.d

        ret = no_compress(args.sketch, args.read_num, args.d, w, args.seed, -1)
        df_ARE['no_compress'].append(ret['ARE'])
        print("No compression: ARE = {}".format(ret['ARE']))

        ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, 4096,
                                  10, 4, wn // 250, -1, args.num_threads, tower_shape=tower_id2shape[2])
        df_ARE['ours'].append(ret['ARE'])
        print("Compressive sensing: ARE = {}".format(ret['ARE']))

        ret = hokusai(args.sketch, args.read_num, args.d, w, args.seed, 2, -1)
        df_ARE['hokusai'].append(ret['ARE'])
        print("Hokusai: ARE = {}".format(ret['ARE']))

        if args.sketch == SketchType.Count:
            df_ARE['elastic'].append(np.nan)
        else:
            ret = elastic(args.sketch, args.read_num, args.d, w, args.seed, 2, -1)
            df_ARE['elastic'].append(ret['ARE'])
            print("Elastic: ARE = {}".format(ret['ARE']))

        ret = cluster_reduce(args.sketch, args.read_num, args.d, w, 2, -1, args.cr_method)
        df_ARE['cluster_reduce'].append(ret['ARE'])
        print("Cluster Reduce: ARE = {}".format(ret['ARE']))

        curr += 1

    save_csv(pd.DataFrame(df_ARE), 'ARE_{}_full_algos.csv'.format(str(args.sketch)))


def test_time_algos():
    """
    Top-k Compression Efficiency (Comparison with Prior Art)
    Metrics:
        Top-k compression time
    Variables:
        1. Memory (width)
        2. Compression method (ours and prior art)
    """
    wns = range(60000, 130000, 10000)  # x-axis
    df_comp = {'w': wns, 'ours': [], 'hokusai': [], 'elastic': [], 'cluster_reduce': []}

    total = len(wns)
    curr = 0

    for wn in wns:
        print("\033[1;34m[{} / {}]\033[0m w = {}".format(curr, total, wn))
        w = [wn] * args.d

        ret = compressive_sensing(
            args.sketch, args.read_num, args.d, w, args.seed, args.sep_thld, args.round_thld, 4, wn // 250, args.k,
            args.num_threads, do_query=False, decompress_method=None, tower_shape=[])
        df_comp['ours'].append(ret['comp_secs'])
        print("Compressive sensing: Compression time = {} s".format(ret['comp_secs']))

        ret = hokusai(args.sketch, args.read_num, args.d, w, args.seed, 4, args.k)
        df_comp['hokusai'].append(ret['comp_secs'])
        print("Hokusai: Compression time = {} s".format(ret['comp_secs']))

        ret = elastic(args.sketch, args.read_num, args.d, w, args.seed, 4, args.k)
        df_comp['elastic'].append(ret['comp_secs'])
        print("Elastic: Compression time = {} s".format(ret['comp_secs']))

        ret = cluster_reduce(args.sketch, args.read_num, args.d, w, 4, args.k, method_id=args.cr_method)
        df_comp['cluster_reduce'].append(ret['comp_secs'])
        print("Cluster Reduce: Compression time = {} s".format(ret['comp_secs']))
        curr += 1

    save_csv(pd.DataFrame(df_comp), 'SECS_CM_algos.csv')


def test_time_full_algos():
    """
    Full Compression Efficiency (Comparison with Prior Art)
    Metrics:
        Full compression time
    Variables:
        1. Memory (width)
        2. Compression method (ours and prior art)
    """
    wns = range(200000, 1200000, 100000)  # x-axis
    df_comp = {'w': wns, 'ours': [], 'hokusai': [], 'elastic': [], 'cluster_reduce': []}

    total = len(wns)
    curr = 0

    for wn in wns:
        print("\033[1;34m[{} / {}]\033[0m w = {}".format(curr, total, wn))
        w = [wn] * args.d

        # ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, 4096, args.round_thld,
        #                           4, wn // 250, -1, args.num_threads, do_query=False, decompress_method=None, tower_shape=[])
        # df_comp['ours'].append(ret['comp_secs'] + ret['tower_comp_secs'])

        ret = compressive_sensing_comp_simd(args.read_num, args.d, w, args.seed, 4)
        df_comp['ours'].append(ret['total_comp_secs'])
        print("Compressive sensing: Compression time = {} s".format(ret['total_comp_secs']))

        ret = hokusai(args.sketch, args.read_num, args.d, w, args.seed, 4, -1)
        df_comp['hokusai'].append(ret['comp_secs'])
        print("Hokusai: Compression time = {} s".format(ret['comp_secs']))

        ret = elastic(args.sketch, args.read_num, args.d, w, args.seed, 4, -1)
        df_comp['elastic'].append(ret['comp_secs'])
        print("Elastic: Compression time = {} s".format(ret['comp_secs']))

        ret = cluster_reduce(args.sketch, args.read_num, args.d, w, 4, -1, method_id=args.cr_method)
        df_comp['cluster_reduce'].append(ret['comp_secs'])
        print("Cluster Reduce: Compression time = {} s".format(ret['comp_secs']))

    save_csv(pd.DataFrame(df_comp), 'SECS_CM_full_algos.csv')


def test_shiftbf():
    wns = [500000, 700000, 900000, 1100000]  # Lines
    sbf_sizes = np.array(range(10, 61, 10)) * 8 * 1000  # x-axis
    print(sbf_sizes)
    df = {'sbf_size': [], **{wn: [] for wn in wns}}

    for sbf_size in sbf_sizes:
        df['sbf_size'].append(sbf_size)
        for wn in wns:
            w = [wn] * args.d

            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, 4096, args.round_thld,
                                      4, wn // 250, -1, args.num_threads, tower_shape=tower_id2shape[1], sbf_size=sbf_size)
            df[wn].append(ret['ARE'])

    save_csv(pd.DataFrame(df), 'ARE_{}_shiftbf_verybig.csv'.format(str(args.sketch)))


def test_topk_datasets():
    """
    Test the accuracy of our method and prior art. Dataset is specified in config.json maunaully.
    Metrics:
        Top-k accuracy (ARE)
    Variables:
        Compression method (ours and prior art)
    """
    df = {'compressive_sensing': [], 'elastic': [], 'cluster_reduce': [], 'hokusai': []}
    ratio = 8

    no_compress(args.sketch, args.read_num, args.d, args.w, args.seed, args.k)

    ret = compressive_sensing(args.sketch, args.read_num, args.d, args.w, args.seed, args.sep_thld,
                              args.round_thld, ratio, args.w[0] // 250, args.k, args.num_threads)
    df['compressive_sensing'].append(ret['ARE'])

    ret = elastic(args.sketch, args.read_num, args.d, args.w, args.seed, ratio, args.k)
    df['elastic'].append(ret['ARE'])

    ret = cluster_reduce(args.sketch, args.read_num, args.d, args.w, ratio, args.k, args.cr_method, args.debug)
    df['cluster_reduce'].append(ret['ARE'])

    ret = hokusai(args.sketch, args.read_num, args.d, args.w, args.seed, ratio, args.k)
    df['hokusai'].append(ret['ARE'])

    save_csv(pd.DataFrame(df), 'ARE_topk_datasets.csv')


def test_full_datasets():
    """
    Test the accuracy of our method and prior art. Dataset is specified in config.json maunaully.
    Metrics:
        Full accuracy (ARE)
    Variables:
        Compression method (ours and prior art)
    """
    df = {'compressive_sensing': [], 'elastic': [], 'cluster_reduce': [], 'hokusai': []}
    ratio = 2

    ret = compressive_sensing(args.sketch, args.read_num, args.d, args.w, args.seed, 4096,
                              args.round_thld, ratio, args.w[0] // 250, -1, args.num_threads, tower_shape=tower_id2shape[1])
    df['compressive_sensing'].append(ret['ARE'])

    ret = elastic(args.sketch, args.read_num, args.d, args.w, args.seed, ratio, -1)
    df['elastic'].append(ret['ARE'])

    ret = cluster_reduce(args.sketch, args.read_num, args.d, args.w, ratio, -1, args.cr_method, args.debug)
    df['cluster_reduce'].append(ret['ARE'])

    ret = hokusai(args.sketch, args.read_num, args.d, args.w, args.seed, ratio, -1)
    df['hokusai'].append(ret['ARE'])

    save_csv(pd.DataFrame(df), 'ARE_full_datasets.csv')


def test_billion_dataset():
    """
    Test multiple metrics on a billion-scale dataset.
    Metrics:
        1. Accuracy (ARE)
        2. Compression error (CE)
        3. Compression time
        4. Decompression time
    Variables:
        1. Memory (witdh)
        2. k
    """
    ks = [-1, 5000, 10000, 20000]  # Lines
    wns = list(range(4000000, 24000000, 2000000))  # x-axis
    df_ARE = {'w': [], **{k: [] for k in ks}}
    df_CE = {'w': [], 'CE': []}
    df_time = {'w': [], 'comp_secs': [], 'decomp_secs': []}

    total = len(ks) * len(wns)
    curr = 0

    for wn in wns:
        df_ARE['w'].append(wn)
        df_CE['w'].append(wn)
        df_time['w'].append(wn)

        k, extra_ks = ks[0], ks[1:]
        print("\033[1;34m[{} / {}]\033[0m w = {}, ks = {}".format(curr, total, wn, ks))
        w = [wn] * args.d
        ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, args.sep_thld, args.round_thld,
                                  8, wn // 250, k, args.num_threads, tower_shape=tower_id2shape[1], extra_ks=extra_ks)
        df_ARE[k].append(ret['ARE'])
        for i, exk in enumerate(extra_ks):
            df_ARE[exk].append(ret['extra_AREs'][i])
        df_CE['CE'].append(ret['CE'])
        df_time['comp_secs'].append(ret['comp_secs'] + ret['tower_comp_secs'])
        df_time['decomp_secs'].append(0 + ret['tower_decomp_secs'])  # ret['decomp_secs']

        print(ret)
        curr += 1

        save_csv(pd.DataFrame(df_ARE), "X_billion_{}_ARE.csv".format(str(args.sketch)))
        save_csv(pd.DataFrame(df_CE), "X_billion_{}_CE.csv".format(str(args.sketch)))
        save_csv(pd.DataFrame(df_time), "X_billion_{}_time.csv".format(str(args.sketch)))

    save_csv(pd.DataFrame(df_ARE), "X_billion_{}_ARE.csv".format(str(args.sketch)))
    save_csv(pd.DataFrame(df_CE), "X_billion_{}_CE.csv".format(str(args.sketch)))
    save_csv(pd.DataFrame(df_time), "X_billion_{}_time.csv".format(str(args.sketch)))


def test_build_time():
    """
    Metrics:
        Sketch build time
    Variables:
        Memory (witdh)
    """
    wns = range(200000, 1200000, 100000)  # x-axis
    df_build = {'w': wns, 'build_secs': []}

    for wn in wns:
        w = [wn] * args.d
        ret = no_compress(args.sketch, args.read_num, args.d, w, args.seed, 10)
        df_build['build_secs'].append(ret['build_secs'])
        print(ret)

    save_csv(pd.DataFrame(df_build), 'X_SECS_build.csv')


def test_tower_ratio():
    """
    Metrics:
        Full accuracy (ARE)
    Variables:
        1. Compression ratio of tower
        2. Memory (width)
    """
    wns = [400000, 800000, 1200000, 1600000]  # Lines
    tower_shape_ids = [10, 11,   12,   13,   14,  15, 16,   17, 18]  # x-axis
    sep_thlds = [65536, 4096, 1024, 4096, 256, 64, 1024, 64, 16]
    real_ratios = [2.78261, 3.04762, 3.2, 4.26667, 4.92308, 5.33333, 5.81818, 7.11111, 8]
    df_ARE = {'real_ratio': [], **{wn: [] for wn in wns}}

    total = len(wns) * len(tower_shape_ids)
    curr = 0

    for i, tower_shape_id in enumerate(tower_shape_ids):
        df_ARE['real_ratio'].append(real_ratios[i])
        for wn in wns:
            print("\033[1;34m[{} / {}]\033[0m tower_shapd_id = {} (sep_thld={}), w = {}".format(curr,
                  total, tower_shape_id, sep_thlds[i], wn))
            w = [wn] * args.d

            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, sep_thlds[i], args.round_thld,
                                      4, wn//250, -1, args.num_threads, tower_shape=tower_id2shape[tower_shape_id])
            df_ARE[wn].append(ret['ARE'])

            print(ret['ARE'])
            curr += 1

        save_csv(pd.DataFrame(df_ARE), 'X_ARE_{}_tower_ratio.csv'.format(str(args.sketch)))

    save_csv(pd.DataFrame(df_ARE), 'X_ARE_{}_tower_ratio.csv'.format(str(args.sketch)))


def test_only_sensing():
    """
    Metrics:
        Accuracy (ARE)
    Variables:
        1. k
        2. Whether to use TowerEncoding or not
    """
    ks = [100, 500, 5000, 50000, -1]  # x-axis
    sep_thlds = [23000, 9301, 800, 500, 500]
    wns = [30000, 65000, 90000, 400000, 1000000]
    tower_shape_ids = [-1, 1]  # Bars
    df_ARE = {'k': ks, **{tower_shape_id: [] for tower_shape_id in tower_shape_ids}}

    total = len(tower_shape_ids) * len(ks)
    curr = 1

    for i, k in enumerate(ks):
        wn = wns[i]
        w = [wn] * args.d
        for tower_shape_id in tower_shape_ids:
            sep_thld = 4096 if tower_shape_id >= 0 else sep_thlds[i]
            print("\033[1;34m[{} / {}]\033[0m k = {}, tower_shape_id = {}".format(curr, total, k, tower_shape_id))

            ret = compressive_sensing(args.sketch, args.read_num, args.d, w, args.seed, sep_thld, args.round_thld,
                                      2, wn//250, k, args.num_threads, tower_shape=tower_id2shape[tower_shape_id])
            df_ARE[tower_shape_id].append(ret['ARE'])
            print("ARE = {}".format(ret['ARE']))

            curr += 1

    save_csv(pd.DataFrame(df_ARE), 'X_ARE_only_sensing.csv')


def test_manual_py():
    wn = int(15728640 / 4 / args.d)  # 10 MB
    w = [wn] * args.d
    sketch = Sketch(args.read_num, args.d, SketchType.CM)
    for i in range(args.d):
        sketch.init_arr(i, None, w[i], -1, 997 + i)
    sketch.insert_dataset()
    print("no_compress ARE", no_compress(SketchType.CM, args.read_num, args.d, w, args.seed, args.k)['ARE'])

    ratio = 32

    A_frags_ds = []
    y_frags_ds = []
    for i in range(args.d):
        temp = (c_uint * w[i])()
        sketch.copy_array(i, temp)
        temp = np.array(temp)
        for j in range(len(temp)):
            if temp[j] < args.sep_thld:
                temp[j] = 0
        A_frags, y_frags, _ = ts_compress_array(temp, ratio, wn//500)
        A_frags_ds.append(A_frags)
        y_frags_ds.append(y_frags)

    sketch_copy = Sketch(args.read_num, args.d, SketchType.CM, is_compsen=True)
    for i in range(args.d):
        res, _ = ts_decompress_array(A_frags_ds[i], y_frags_ds[i], has_neg=False)
        out_data = (c_uint * w[i])(*res)
        sketch_copy.init_arr(i, out_data, w[i], -1, 997 + i)

    ARE, AAE = sketch_copy.query_dataset(args.k)
    print("ARE = {}".format(ARE))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TowerSensing test cases")
    parser.add_argument('--read_num', type=int, default=-1, help='number of packets to read from dataset')
    parser.add_argument('--k', type=int, default=500, help='report result for top-k frequent flows')
    parser.add_argument('--d', type=int, default=3, help='number of arrays in a sketch')
    parser.add_argument('--w', type=int, default=[65000, 65000, 65000],
                        nargs='+', help='number of counters in each array')
    parser.add_argument('--seed', type=int, default=997, help='base seed')
    parser.add_argument('--sep_thld', type=int, default=9300, help='separating threshold')
    parser.add_argument('--round_thld', type=int, default=1, help='rounding parameter in SketchSensing')
    parser.add_argument('--num_frags', type=int, default=300, help='number of fragments in each array')
    parser.add_argument('--sketch', type=SketchType.from_string, default=SketchType.CM,
                        choices=list(SketchType), help='sketch type')
    parser.add_argument('--num_threads', type=int, default=8, help='number of threads')
    parser.add_argument('--cr_method', type=int, default=4, help='compress method for Cluster Reduce')
    parser.add_argument('--cfg', type=str, default='config.json', help='path of global config')
    parser.add_argument('--test', type=str, required=True, help='test case name')
    parser.add_argument('--debug', dest='debug', action='store_true', help='turn on debugging')
    parser.set_defaults(debug=False)

    global args
    args = parser.parse_args()
    print(args)

    global global_cfg
    with open(args.cfg) as f:
        global_cfg = json.load(f)

    init(args)
    globals()[args.test]()
