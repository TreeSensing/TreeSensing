import sys
sys.path.append('../../../CPU')

from src.sketch_compress import *
import pandas as pd
import json
import time
import tenseal as ts


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


def test_time_encryption():
    def homomorphic(A_frags_ds, y_frags_ds):
        nonlocal encrypt_secs
        nonlocal decrypt_secs
        print("Running homomorphic encryption and decryption")
        context = ts.context(ts.SCHEME_TYPE.BFV, poly_modulus_degree=4096, plain_modulus=1032193)

        counter_cnt = len(y_frags_ds[0]) * 0.05

        y_front_ds = []
        for i in range(args.d):
            temp = []
            for y_frag in y_frags_ds[i]:
                for j, v in enumerate(y_frag):
                    if j > counter_cnt:
                        break
                    temp.append(v)
            y_front_ds.append(temp)

        # Encrypt
        time_start = time.time()
        y_encrypted_ds = []
        for i in range(args.d):
            y_encrypted = ts.bfv_vector(context, y_front_ds[i])
            y_encrypted_ds.append(y_encrypted)
        encrypt_secs = time.time() - time_start

        # Decrypt
        time_start = time.time()
        for i in range(args.d):
            y_decrypted = y_encrypted_ds[i].decrypt()
            # assert all(??)
        decrypt_secs = time.time() - time_start
        return A_frags_ds, y_frags_ds

    wns = range(10000, 110000, 10000)  # x-axis
    df = {'w': wns, 'encrypt': [], 'decrypt': []}
    ratio = 2

    total = len(wns)
    curr = 0

    for wn in wns:
        print("\033[1;34m[{} / {}]\033[0m w = {}".format(curr, total, wn))
        w = [wn] * args.d

        temp_encrypt = []
        temp_decrypt = []
        for _ in range(10):
            encrypt_secs, decrypt_secs = 0, 0
            ret = compressive_sensing(
                args.sketch, args.read_num, args.d, w, args.seed, args.sep_thld, args.round_thld, ratio, wn // 250,
                args.k, args.num_threads, decompress_method=None, func=homomorphic)
            temp_encrypt.append(encrypt_secs)
            temp_decrypt.append(decrypt_secs)
            print("Encryption time = {} s, decryption time = {} s".format(encrypt_secs, decrypt_secs))

        df['encrypt'].append(np.average(reject_outliers(temp_encrypt)))
        df['decrypt'].append(np.average(reject_outliers(temp_decrypt)))
        curr += 1

    save_csv(pd.DataFrame(df), 'SECS_CM_encrypt_decrypt.csv')


class PseudoArgs:
    def __init__(self, k):
        self.cfg = '../../../CPU/config.json'
        self.read_num = -1
        self.sketch = SketchType.CM
        self.d = 3
        self.k = k
        self.sep_thld = 9200
        self.round_thld = 1
        self.seed = 997
        self.cr_method = 1
        self.num_threads = 8
        self.debug = False
        self.test = 'test_time_encryption'


if __name__ == '__main__':
    global args
    args = PseudoArgs(k=500)

    global global_cfg
    with open(args.cfg) as f:
        global_cfg = json.load(f)

    global lib, crlib
    lib, crlib = init(args, '../../../CPU/build')

    test_time_encryption()
