import glob
import numpy as np
import os
import trimesh


def load_mesh(mesh_file):
    return trimesh.load(mesh_file)


def explode(mesh_lst):
    # calculate the center of the scene
    centers = np.array([i.centroid for i in mesh_lst])
    center = np.mean(centers, axis=0)

    # move each mesh away from the center
    for i, mesh in enumerate(mesh_lst):
        direction = mesh.centroid - center
        distance = np.linalg.norm(direction)
        mesh.apply_translation(direction / distance * 0.1 * distance)


COLORS = np.array([
    [255, 128, 0, 255],
    [227, 26, 26, 255],
    [54, 125, 184, 255],
    [76, 173, 74, 255],
    [102, 194, 163, 255],
    [166, 214, 82, 255],
    [230, 138, 194, 255],
    [250, 140, 97, 255],
    [243, 153, 0, 255],
    [240, 59, 32, 255],
    [189, 0, 38, 255],
    [128, 0, 38, 255],
    [0, 0, 0, 255]
])

folder = 'assets/TU_96_2/'
target_mesh_pattern = '*.stl'
target_format = '.glb'
flip_vertical = True

# Retrieve meshes by looking for a pattern
meshes_path = glob.glob(folder + target_mesh_pattern)

print('Found ' + str(len(meshes_path)) + ' meshes')

meshes = [load_mesh(mesh) for mesh in meshes_path]
for idx, mesh in enumerate(meshes):
    mesh.fix_normals()
    mesh.visual.vertex_colors = COLORS[idx % len(COLORS)]
    if flip_vertical:
        mesh.apply_scale([1, -1, 1])

explode(meshes)

# Visualize meshes all at once, with colors
scene = trimesh.Scene(meshes)
scene.show()

# Save
common_str = os.path.commonprefix(meshes_path)
common_str = os.path.basename(common_str)
common_str = os.path.splitext(common_str)[0]
common_str = common_str.split('_')[:-1]
common_str = '_'.join(common_str)

trimesh.exchange.export.export_scene(scene, folder + common_str + target_format)
