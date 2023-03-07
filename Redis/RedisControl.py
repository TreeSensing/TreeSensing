from base64 import decode
from bz2 import compress
from calendar import day_abbr
from pickle import FALSE
import string
from time import time
from redis import Redis
from math import log2
import csv
import argparse
import numpy as np


# read the dataset
def read_data_caida16():
    data = []
    with open("./dataset/CAIDA_redis.dat", "rb") as file_in:
        while True:
            time = file_in.read(8)
            if not time:
                break
            _ = file_in.read(8)

            id = _.hex()
            id = int(id, 16)
            data.append(str(id))
    return data[:500000]


# benchmarking the performance of the sketch with corresponding strategy
def test_frequence(name, w, d, mode, ratio, zerothid, runs, shape):
    rd = Redis(decode_responses=True)
    # print("find data structure")
    rd.delete("test")
    pp = rd.execute_command(f"{name}.create", "test",
                            w, d, ratio, zerothid, shape)
    print("-------------------------------W value:",
          w, "-------------------------------")
    # print(pp)

    data = read_data_caida16()

    time_start = time()
    res = rd.execute_command(f"{name}.insert", "test", *data)
    time_end = time()
    throughput_insert = len(data)/(time_end - time_start)
    compressionTimeList = []
    decompressionTimeList = []
    print("Insert througput is: ", throughput_insert)
    for i in range(runs):
        if (mode == "SS"):
            
            print("run: ",i+1)

            print(
                "********************************COMPRESSION********************************")
            print("Starting test of Compression")
            time_start = time()
            pp = rd.execute_command(
                f"{name}.sketchSensingCompress", "test", ratio, zerothid, shape)
            time_end = time()
            compressionTime = time_end - time_start
            print("Compression Time Usage:", time_end - time_start)
            print("Compression Status", pp)
            print()
            print(
                "*******************************DECOMPRESSION*******************************")
            print("Starting test of Decompression")
            time_start = time()
            pp = rd.execute_command(
                f"{name}.sketchSensingDecompress", "test", ratio, zerothid)
            time_end = time()
            print("Decompression Time Usage:", time_end - time_start)
            decompressionTime = time_end - time_start
            print("Decompression Status", pp)
            data = [w, compressionTime, decompressionTime]
            compressionTimeList.append(compressionTime)
            decompressionTimeList.append(decompressionTime)
            writer.writerow(data)
        if (mode == "TS"):
            print("run: ",i+1)
            print(
                "********************************COMPRESSION********************************")
            print("Starting test of Compression")
            time_start = time()
            pp = rd.execute_command(
                f"{name}.towerSensingCompress", "test", ratio, zerothid, shape)
            time_end = time()
            compressionTime = time_end - time_start
            print("Compression Time Usage:", time_end - time_start)
            print("Compression Status", pp)
            print()
            print(
                "*******************************DECOMPRESSION*******************************")
            print("Starting test of Decompression")
            time_start = time()
            pp = rd.execute_command(
                f"{name}.towerSensingDecompress", "test", ratio, zerothid)
            time_end = time()
            print("Decompression Time Usage:", time_end - time_start)
            decompressionTime = time_end - time_start
            print("Decompression Status", pp)
            data = [w, compressionTime, decompressionTime]
            compressionTimeList.append(compressionTime)
            decompressionTimeList.append(decompressionTime)
            writer.writerow(data)
    comp_avg = np.mean(compressionTimeList)
    comp_var = np.var(compressionTimeList)
    decomp_avg = np.mean(decompressionTimeList)
    decomp_var = np.var(decompressionTimeList)

    writer.writerow(["comp_avg", comp_avg])
    writer.writerow(["comp_var", comp_var])
    writer.writerow(["decomp_avg", decomp_avg])
    writer.writerow(["decomp_var", decomp_var])
    return throughput_insert


if __name__ == "__main__":
    print("start")
    res = []

    parser = argparse.ArgumentParser()
    parser.add_argument("-Mode", type=str, required=True,
                        help="switching between the Mode of SketchSensing(SS) and TowerEncoding+SketchSensing(TS)")
    parser.add_argument("-w", type=int, required=True, help="the w value")
    parser.add_argument("-d", type=int, required=True, help="the d value")
    parser.add_argument("-r", type=int, required=False,
                        help="the compression ratio, which defaults to 3", default=3)
    parser.add_argument("-s", type=int, required=False,
                        help="the separating threshold, which defaults to 256", default=256)
    parser.add_argument("-runs", type=int, required=True,
                        help="the number of runs, this field is to be specified")
    parser.add_argument("-shape", type=int, required=False,
                        help="the shape of the tower(1st-4th:1-4),which defaults to (8,2) (8,2) (4,1), for additional shapes info, check out the readme file", default=1)

    args = parser.parse_args()
    header = ['w_value', 'Compression_time', 'Decompression_time']

    with open('COMP_DECOMP_TIME_.csv', 'w', encoding='UTF8', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        i = args.w
        d = args.d
        r = args.r
        s = args.s
        mode = args.Mode
        runs = args.runs
        shape = args.shape
        if (mode == "SS"):
            print("Working Mode:SketchSensing(SS)")
        if (mode == "TS"):
            print("Working Mode:SketchSensing+TowerEncoding(TS)")
        print("w value: ", i)
        print("d value: ", d)
        test_frequence("basic_cm", i, d, mode, s, r, runs, shape)
