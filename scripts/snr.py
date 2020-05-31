#!/usr/bin/python3

#########################################
# Masyagin M.M., masyagin1998@yandex.ru #
#########################################

# Hansen J, Pellom B., An effective quality evaluation protocol for speech enhancement algorithms,
# Proceedings of International Conference on Spoken Language Processing, 1998 г.

import argparse

import numpy as np
import soundfile as sf

desc_str = r"""Calculate SNR and Segmental SNR of degraded wav file."""

MIN_ALLOWED_SNR = -10.0
MAX_ALLOWED_SNR = +35.0

EPSILON = 0.0000001


def parse_args():
    parser = argparse.ArgumentParser(prog='snr',
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     description=desc_str)
    parser.add_argument('-v', '--version', action='version', version='%(prog)s v0.1')

    parser.add_argument('-o', '--original', type=str,
                        help=r'original .wav file with speech')
    parser.add_argument('-d', '--degraded', type=str,
                        help=r'degraded .wav file by noise')

    parser.add_argument('-n', '--normalize', action='store_true',
                        help=r'normalize DC bias to zero and scale dynamic ranges')

    parser.add_argument('--frame', type=int, default=0,
                        help=r'frame size in samples')
    parser.add_argument('--overlap', type=int, default=0,
                        help=r'overlap in percents')

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

    snr = 10.0 * np.log10(np.sum(np.square(original)) / np.sum(np.square(original - degraded)))

    if args.frame == 0:
        frame_size_smpls = int(np.floor(2.0 * sr / 100.0))
    else:
        frame_size_smpls = args.frame

    if args.overlap == 0:
        overlap_perc = 50.0
    else:
        overlap_perc = args.overlap

    overlap_size_smpls = int(np.floor(frame_size_smpls * overlap_perc / 100.0))
    step_size = frame_size_smpls - overlap_size_smpls

    window = np.hanning(frame_size_smpls)

    n = int(min_len / step_size - (frame_size_smpls / step_size))

    seg_snrs = []

    ind = 0

    for i in range(n):
        original_frame = original[ind:ind + frame_size_smpls] * window
        degraded_frame = degraded[ind:ind + frame_size_smpls] * window

        speech_power = np.sum(np.square(original_frame))
        noise_power = np.sum(np.square(original_frame - degraded_frame))

        seg_snr = 10.0 * np.log10(speech_power / (noise_power + EPSILON) + EPSILON)
        seg_snr = np.max([seg_snr, MIN_ALLOWED_SNR])
        seg_snr = np.min([seg_snr, MAX_ALLOWED_SNR])

        seg_snrs.append(seg_snr)

        ind += step_size

    seg_snr = np.mean(seg_snrs)

    print("SNR:", snr)
    print("Segmental SNR:", seg_snr)


if __name__ == "__main__":
    main()
