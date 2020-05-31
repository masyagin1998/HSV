#!/usr/bin/python3

#########################################
# Masyagin M.M., masyagin1998@yandex.ru #
#########################################

import argparse

import numpy as np
import soundfile as sf
from pypesq import pesq

desc_str = r"""Calculate PESQ of degraded wav file."""


def parse_args():
    parser = argparse.ArgumentParser(prog='pesq',
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     description=desc_str)
    parser.add_argument('-v', '--version', action='version', version='%(prog)s v0.1')

    parser.add_argument('-o', '--original', type=str,
                        help=r'original .wav file with speech')
    parser.add_argument('-d', '--degraded', type=str,
                        help=r'degraded .wav file by noise')

    parser.add_argument('-n', '--normalize', action='store_true',
                        help=r'normalize DC bias to zero and scale dynamic ranges')

    args = parser.parse_args()

    assert (args.original is not None)
    assert (args.degraded is not None)

    return args


def main():
    args = parse_args()

    original, sr = sf.read(args.original)
    degraded, sr = sf.read(args.degraded)

    # speex: смещение 543 сэмпла.
    # webrtc: смещение 96 сэмпла.
    degraded = degraded[:]

    min_len = min(len(original), len(degraded))

    original = original[:min_len]
    degraded = degraded[:min_len]

    if args.normalize:
        original = original - np.mean(original) * np.ones(min_len)
        degraded = degraded - np.mean(degraded) * np.ones(min_len)
        degraded = degraded * (np.max(np.abs(original)) / np.max(np.abs(degraded)))

    score = pesq(original, degraded, sr)
    print("PESQ:", score)


if __name__ == "__main__":
    main()
