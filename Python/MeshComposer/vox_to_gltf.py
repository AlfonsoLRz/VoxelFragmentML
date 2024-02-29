from voxypy.models import Entity, Voxel
import glob
import numpy as np
import os
import trimesh
from trimesh.transformations import translation_matrix
from tqdm import tqdm


def downscale(voxelization, factor):
    """
    Downscale the voxelization by a factor
    :param voxelization: the voxelization to downscale
    :param factor: the factor to downscale by
    :return: the downscale voxelization
    """
    new_voxelization = np.zeros((voxelization.shape[0] // factor, voxelization.shape[1] // factor,
                                 voxelization.shape[2] // factor), dtype=np.uint8)
    for i in range(0, voxelization.shape[0], factor):
        for j in range(0, voxelization.shape[1], factor):
            for k in range(0, voxelization.shape[2], factor):
                new_voxelization[i // factor, j // factor, k // factor] = np.max(
                    voxelization[i:i + factor, j:j + factor, k:k + factor])

    return new_voxelization


COLORS = np.array([
    [255, 128, 0],
    [227, 26, 26],
    [54, 125, 184],
    [76, 173, 74],
    [102, 194, 163],
    [166, 214, 82],
    [230, 138, 194],
    [250, 140, 97],
    [243, 153, 0],
    [240, 59, 32],
    [189, 0, 38],
    [128, 0, 38],
    [0, 0, 0]
])

folder = 'assets/BA_87bis_1/'
target_mesh_pattern = '*.vox'
target_format = '.glb'
flip_vertical = True

# Read .vox files
vox_files = glob.glob(folder + target_mesh_pattern)
print('Found ' + str(len(vox_files)) + ' .vox files')

# Cube mesh
cube = trimesh.creation.box((1, 1, 1))

# Read binary .vox files
for idx, vox_file in enumerate(vox_files):
    voxelization = Entity().from_file(vox_file)
    file_name = os.path.basename(vox_file)

    # Create voxelization mesh
    scene = trimesh.Scene()
    voxelization_data = voxelization.get_dense()
    voxelization_data = np.transpose(voxelization_data, (0, 2, 1))
    voxelization_data = downscale(voxelization_data, 2)
    dimensions = voxelization_data.shape

    for i, j, k in tqdm(np.argwhere(voxelization_data)):
        if voxelization_data[i, j, k] <= 1:
            continue
        # if flip_vertical:
        #     j = -j

        new_cube = cube.copy()
        new_cube.visual.vertex_colors = COLORS[(voxelization_data[i, j, k] - 2) % len(COLORS)]

        scene.add_geometry(new_cube, 'box' + str(i) + 'x' + str(j) + 'x' + str(k),
                           transform=translation_matrix((dimensions[0] - i, dimensions[1] - j, dimensions[2] - k)))

    # Visualize voxelization
    # scene.show()

    # Save voxelization
    save_path = os.path.join(folder, file_name + target_format)
    scene.export(save_path)