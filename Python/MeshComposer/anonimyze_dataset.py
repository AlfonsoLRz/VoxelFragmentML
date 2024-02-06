import os
import shutil
import trimesh
from tqdm import tqdm

dataset_folder = '../../../Datasets/Vessel_02/'
target_mesh_extension = 'obj'
save_folder = '../../../Datasets/Vessels_renamed/'
flip_y = True

# Retrieve meshes that meet the requirements [recursive]
meshes = []
for root, dirs, files in os.walk(dataset_folder):
    for file in files:
        if file.endswith(target_mesh_extension):
            meshes.append(os.path.join(root, file))

print('Found ' + str(len(meshes)) + ' meshes')

# Anonymize meshes' names
if not os.path.exists(save_folder):
    os.makedirs(save_folder)

for idx, mesh in tqdm(enumerate(meshes)):
    if flip_y:
        mesh = trimesh.load(mesh)
        mesh.apply_scale([1, -1, 1])
        mesh.export(save_folder + str(idx + 1) + '.' + target_mesh_extension, file_type=target_mesh_extension)
    else:
        new_name = os.path.join(save_folder, str(idx + 1) + '.' + target_mesh_extension)
        shutil.copy(mesh, new_name)
