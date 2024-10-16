import glob
import matplotlib.pyplot as plt
import plyfile

folder = 'samples/'
mesh_extension = 'ply'

if __name__ == '__main__':
    pcs = glob.glob(folder + '*.' + mesh_extension)

    for pc_path in pcs:
        ply_data = plyfile.PlyData.read(pc_path)

        x = ply_data['vertex']['x']
        y = ply_data['vertex']['z']
        z = -ply_data['vertex']['y']

        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        ax.scatter(x, y, z)
        ax.set_aspect('equal')
        plt.tight_layout()
        plt.savefig(pc_path.replace('.' + mesh_extension, '.png'))
        plt.show()
