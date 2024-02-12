import glob
import numpy as np
import os

import open3d.cpu.pybind.utility
from open3d.cpu.pybind.io import read_point_cloud


def load_point_cloud(pc_file):
    return read_point_cloud(pc_file)

def explode(pc_lst):
    # calculate the center of the scene
    centers = np.array([i.get_center() for i in pc_lst])
    center = np.mean(centers, axis=0)

    # move each mesh away from the center
    for i, point_cloud in enumerate(pc_lst):
        direction = point_cloud.get_center() - center
        distance = np.linalg.norm(direction)
        point_cloud.translate(direction / distance * 0.1 * distance)


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

folder = 'assets/TU_96_2/'
target_mesh_pattern = '*.ply'
target_format = '.ply'
flip_vertical = True

# Retrieve meshes by looking for a pattern
point_clouds_path = glob.glob(folder + target_mesh_pattern)

print('Found ' + str(len(point_clouds_path)) + ' point clouds')

point_clouds = [load_point_cloud(point_cloud) for point_cloud in point_clouds_path]
for idx, point_cloud in enumerate(point_clouds):
    color = np.asarray(COLORS[idx % len(COLORS)]) / 255.0
    point_cloud.paint_uniform_color(color)

    if flip_vertical:
        T = np.eye(4)
        T[1, 1] = -1
        point_cloud.transform(T)

explode(point_clouds)

# Visualize point clouds all at once, with colors
# open3d.visualization.draw_geometries(point_clouds)

# Save
common_str = os.path.commonprefix(point_clouds_path)
common_str = os.path.basename(common_str)
common_str = os.path.splitext(common_str)[0]
common_str = common_str.split('_')[:-1]
common_str = '_'.join(common_str)

# Merge all into one point cloud
merged_point_cloud = open3d.geometry.PointCloud()
for point_cloud in point_clouds:
    merged_point_cloud += point_cloud

# Export with colors
open3d.io.write_point_cloud(folder + common_str + target_format, merged_point_cloud)
