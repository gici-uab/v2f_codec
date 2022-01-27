#!/usr/bin/env python3
"""Quantize and then render some sample images
"""
__author__ = "Miguel Hernández-Cabronero"
__since__ = "2021/11/26"

import os
import re
import enb
import numpy as np

class RenderQuantized(enb.isets.ImageVersionTable):
    def __init__(self, original_base_dir, version_raw_dir, version_png_dir, x_min, y_min, width, height, qstep):
        super().__init__(
            original_base_dir=original_base_dir,
            version_base_dir=version_raw_dir,
            version_name=f"{self.__class__.__name__}",
            check_generated_files=False)
        self.raw_dir = os.path.abspath(version_raw_dir)
        self.png_dir = os.path.abspath(version_png_dir)
        self.x_min = int(x_min)
        self.y_min = int(y_min)
        self.width = int(width)
        self.height = int(height)
        self.qstep = int(qstep)



    def version(self, input_path, output_path, row):
        output_raw_path = re.sub("-(\d+)x\d+x\d+", f"-Q{self.qstep}" + r"-\1" + f"x{self.height}x{self.width}",
                             output_path)
        crop = enb.isets.load_array_bsq(
            file_or_path=input_path,
            image_properties_row=row)
        
        crop = crop[self.x_min:self.x_min + self.width, self.y_min:self.y_min + self.height, :]
        crop = crop.astype(">u2")
        if self.qstep != 1:
            crop //= self.qstep
            crop *= self.qstep
            crop += (self.qstep // 2)
            crop = np.clip(crop, 0, 255)
        crop = crop.astype(">u1")
        enb.isets.dump_array_bsq(crop, output_raw_path)

        new_properties = dict(row)
        new_properties["width"] = self.width
        new_properties["height"] = self.height

        enb.isets.raw_path_to_png(raw_path=output_raw_path,
                                  image_properties_row=new_properties,
                                  png_path=f"{os.path.abspath(output_raw_path).replace(self.raw_dir, self.png_dir)[:-4]}.png")


if __name__ == "__main__":
    for qstep in [1, 2, 3, 4, 5, 6, 7, 8]:
        width = 128
        height = 128
        x_min = 5120/2 - 2*width
        y_min = 5120/2 - 2*height
        rq = RenderQuantized(original_base_dir=os.path.join(os.path.dirname(os.path.abspath(__file__)), "datasets"),
                             version_raw_dir=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                           f"rendered/rendered_q{qstep}"),
                             version_png_dir=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                           f"rendered_png/rendered_q{qstep}"),
                             x_min=x_min, y_min=y_min,
                             width=width, height=height,
                             qstep=qstep)
        rq.get_df()
