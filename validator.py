#!/usr/bin/env python3
import sys
import os
import skimage
from skimage import metrics
from skimage import io
from skimage import transform
import numpy as np

FRAME_COUNT = 1800
ACCEPT_MIN_PSNR = 32
RESIZE_FACTOR = 2

def main():
    if len(sys.argv) != 3:
        print("Usage: "+sys.argv[0]+" reference_directory own_directory")
        return

    ref_path = sys.argv[1]
    own_path = sys.argv[2]

    validation_str = ""
    success = True
    sum_psnr = 0
    min_psnr = 1000
    max_psnr = 0
    for i in range(FRAME_COUNT):
        frame_name = str(i).zfill(4)
        ref_img_path = ref_path+"/"+frame_name+".png"
        own_img_path = own_path+"/frame_"+frame_name+".bmp"

        report = frame_name + ": "
        if not os.path.exists(ref_img_path):
            print("Reference files are incomplete, quitting!!!")
            print(ref_img_path+" is missing.")
            return
        if not os.path.exists(own_img_path):
            report += "(missing image)"
            success = False
        else:
            ref_img = skimage.io.imread(ref_img_path)
            own_img = skimage.io.imread(own_img_path)
            own_img = skimage.transform.downscale_local_mean(own_img, (RESIZE_FACTOR, RESIZE_FACTOR, 1))
            own_img = own_img.astype(np.uint8)
            psnr = skimage.metrics.peak_signal_noise_ratio(ref_img, own_img)
            sum_psnr += psnr
            min_psnr = min(min_psnr, psnr)
            max_psnr = max(max_psnr, psnr)
            report += str(psnr)
            if psnr < ACCEPT_MIN_PSNR:
                success = False
                report += " BAD, BROKEN IMAGE?"
            else:
                report += " GOOD"
        validation_str += report + "\n"
        print(report)

    outcome_str = ""
    if success:
        outcome_str += "Validation result: successful.\n"
    else:
        outcome_str += "Validation result: failure.\n"

    outcome_str += "Sum PSNR: " + str(sum_psnr) + "\n"
    outcome_str += "Min PSNR: " + str(min_psnr) + "\n"
    outcome_str += "Max PSNR: " + str(max_psnr) + "\n"
    print(outcome_str)

    with open("validation_result.txt", "w") as text_file:
        text_file.write(validation_str+outcome_str)

if __name__ == "__main__":
    main()
