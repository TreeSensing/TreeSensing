import pandas as pd
import time
import tenseal as ts
import os

d = 3

def save_csv(df, filename, outdir='./results/'):
    if not os.path.exists(outdir):
        os.mkdir(outdir)
    df.to_csv(outdir+filename, index=False, sep=',')
    print("Results saved to \033[92m{}\033[0m".format(filename))

class Sketch:
    def __init__(self, d):
        self.arr = [[] for _ in range(d)]
        self.encrypted_arr = [[] for _ in range(d)]
        self.decrypted_arr = [[] for _ in range(d)]

def read_compressed_sketches(node_num):
    sketches = [Sketch(d) for _ in range(node_num)]
    f = open('compressed_sketch_data/{}nodes.txt'.format(node_num), 'r')
    for sk_i in range(node_num):
        for i in range(d):
            line = f.readline().strip()
            sarr = line.split(' ')
            arr = [int(s) for s in sarr]
            sketches[sk_i].arr[i] = arr
    f.close()
    return sketches


def encrypt_sketches(context, sketches):
    for sk_i in range(len(sketches)):
        for i in range(d):
            sketches[sk_i].encrypted_arr[i] = ts.bfv_vector(context, sketches[sk_i].arr[i])
    return sketches


def nonlinear(context, sketches):
    time_start = time.time()
    for sk_i in range(len(sketches)):
        for i in range(d):
            sketches[sk_i].decrypted_arr[i] = sketches[sk_i].encrypted_arr[i].decrypt()
            # for j in range(len(sketches[sk_i].decrypted_arr[i])):
            #     if sketches[sk_i].decrypted_arr[i][j] == sketches[sk_i].arr[i][j]:
            #         print(sketches[sk_i].decrypted_arr[i][j], sketches[sk_i].arr[i][j])
    decrypt_secs = time.time() - time_start
    return decrypt_secs


def linear(context, sketches):
    time_start = time.time()
    combined_sketch = Sketch(d)
    combined_sketch.encrypted_arr = sketches[0].encrypted_arr.copy()
    for sk_i in range(1, len(sketches)):
        for i in range(d):
            combined_sketch.encrypted_arr[i] += sketches[sk_i].encrypted_arr[i]
    
    for i in range(d):
        combined_sketch.decrypted_arr[i] = combined_sketch.encrypted_arr[i].decrypt()


    with open('compressed_sketch_data/{}nodes_decrypted.txt'.format(node_num), 'w+') as f:
        for i in range(d):
            arr_str = ' '.join([str(v) for v in combined_sketch.decrypted_arr[i]])
            f.write(arr_str + '\n')

    decrypt_secs = time.time() - time_start #
    return decrypt_secs


if __name__ == '__main__':
    context = ts.context(ts.SCHEME_TYPE.BFV, poly_modulus_degree=4096, plain_modulus=1032193)
    
    df_nonlinear_decrypt = {'node_num': [], 'nonlinear_decrypt': []}
    df_linear_decrypt = {'node_num': [], 'linear_decrypt': []}

    for node_num in range(2, 11):
        df_nonlinear_decrypt['node_num'].append(node_num)

        sketches = encrypt_sketches(context, read_compressed_sketches(node_num))
        decrypt_secs = nonlinear(context, sketches)
        df_nonlinear_decrypt['nonlinear_decrypt'].append(decrypt_secs)
        print("node_num={}, nonlinear decrypt_secs={}".format(node_num, decrypt_secs))

    save_csv(pd.DataFrame(df_nonlinear_decrypt), 'X_SECS_nonlinear_decrypt.csv')

    for node_num in range(2, 11):
        df_linear_decrypt['node_num'].append(node_num)

        sketches = encrypt_sketches(context, read_compressed_sketches(node_num))
        decrypt_secs = linear(context, sketches)
        df_linear_decrypt['linear_decrypt'].append(decrypt_secs)
        print("node_num={}, linear decrypt_secs={}".format(node_num, decrypt_secs))

    save_csv(pd.DataFrame(df_linear_decrypt), 'X_SECS_linear_decrypt.csv')
