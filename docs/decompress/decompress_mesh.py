import glob
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from struct import unpack
import numpy as np

folder = 'samples/'
mesh_extension = 'binm'

if __name__ == '__main__':
    # get grids in folder
    meshes = glob.glob(folder + '*.' + mesh_extension)

    for mesh_path in meshes:
        # read binary file
        with open(mesh_path, 'rb') as f:
            num_vertices = int.from_bytes(f.read(4), byteorder='little')
            f.read(4)        # read null float

            # read until end of file
            vertices = np.zeros((num_vertices, 3), dtype=np.float32)
            vertices_data = unpack('f'*num_vertices*16, f.read(4*num_vertices*16))
            for vertex_idx in range(num_vertices):
                vertices[vertex_idx] = vertices_data[vertex_idx*16:vertex_idx*16+3]
                vertices[vertex_idx][1] = -vertices[vertex_idx][1]         # flip z, only for our dataset

            # faces
            num_faces = int.from_bytes(f.read(4), byteorder='little')
            f.read(4)       # null int

            faces = np.zeros((num_faces, 3), dtype=np.uint32)
            faces_data = unpack('I'*num_faces*4, f.read(4*num_faces*4))

            for face_idx in range(num_faces):
                faces[face_idx] = faces_data[face_idx*4:face_idx*4+3]

            # save numpy arrays
            vertices.tofile(mesh_path.replace('.' + mesh_extension, '_vertices.npy'))
            faces.tofile(mesh_path.replace('.' + mesh_extension, '_faces.npy'))

            # modify mesh for visualization
            for vertex_idx in range(num_vertices):
                # swap y and z
                vertices[vertex_idx][1], vertices[vertex_idx][2] = vertices[vertex_idx][2], vertices[vertex_idx][1]

            fig = plt.figure()
            ax = fig.add_subplot(111, projection='3d')
            ax.plot_trisurf(vertices[:, 0], vertices[:, 1], vertices[:, 2], triangles=faces)
            ax.set_aspect('equal')
            plt.tight_layout()
            plt.savefig(mesh_path.replace('.' + mesh_extension, '.png'))
            plt.show()
