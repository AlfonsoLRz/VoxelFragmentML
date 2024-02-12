from ctypes import *
class RLEData(Structure):
    _fields_ = [("size", c_size_t), ("buff", c_char_p)]

import glob
from pyvox.models import Vox
from pyvox.parser import VoxParser

folder = 'assets/TU_96_2/'
target_mesh_pattern = '*.rle'
target_format = '.gltf'
flip_vertical = True

# Read .rle files
rle_files = glob.glob(folder + target_mesh_pattern)
print('Found ' + str(len(rle_files)) + ' .rle files')

# Read binary .rle files
for idx, rle_file in enumerate(rle_files):
    # read first unsigned int to determine length
    with open(rle_file, 'rb') as f:
        x_length = int.from_bytes(f.read(4), byteorder='little')
        y_length = int.from_bytes(f.read(4), byteorder='little')
        z_length = int.from_bytes(f.read(4), byteorder='little')

        print('Lengths: ' + str(x_length) + ' ' + str(y_length) + ' ' + str(z_length))

        length = x_length * y_length * z_length
        read_indices = 0

        while read_indices < length:
            # Read uint16_t
            _ = int.from_bytes(f.read(2), byteorder='little')
            color = int.from_bytes(f.read(2), byteorder='little')

            # Read size_t
            size = int.from_bytes(f.read(4), byteorder='little')

            print('Color: ' + str(color) + ' Size: ' + str(size))

            read_indices += size
