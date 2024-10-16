import glob
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
from struct import unpack

folder = 'samples/'
grid_extension = 'rle'

if __name__ == '__main__':
    # get grids in folder
    grids = glob.glob(folder + '*.' + grid_extension)

    for grid_path in grids:
        # read binary file
        with open(grid_path, 'rb') as f:
            # read three unsigned integers
            width = int.from_bytes(f.read(4), byteorder='little')
            height = int.from_bytes(f.read(4), byteorder='little')
            depth = int.from_bytes(f.read(4), byteorder='little')
            grid = np.zeros(width * height * depth, dtype=np.uint8)

            # read until end of file
            offset = 0
            while True:
                binary_data = f.read(6)
                if not binary_data:
                    break
                data = unpack('<HI', binary_data)
                rle_chunk = {'value': data[0], 'length': data[1]}
                grid[offset:offset + rle_chunk['length']] = rle_chunk['value']
                offset += rle_chunk['length']

            grid = grid.reshape((width, height, depth))     # to grid
            grid = np.flip(grid, 1)                    # only for our dataset
            max_dim = max(width, height, depth)             # pad to make it a square
            pad_width = [(0, max_dim - width), (0, max_dim - height), (0, max_dim - depth)]
            grid = np.pad(grid, pad_width, mode='constant', constant_values=0)
            grid.tofile(grid_path.replace('.' + grid_extension, '.npy'))  # save as numpy array

            # for matplotlib visualization
            grid = np.swapaxes(grid, 1, 2)  # swap y and z
            downscaled_grid = grid[::2, ::2, ::2]  # make it smaller for visualization

            fig = plt.figure()
            ax = fig.add_subplot(111, projection='3d')
            ax.voxels(downscaled_grid, shade=True)
            ax.set_aspect('equal')
            plt.tight_layout()
            plt.savefig(grid_path.replace('.' + grid_extension, '.png'))
            plt.show()
