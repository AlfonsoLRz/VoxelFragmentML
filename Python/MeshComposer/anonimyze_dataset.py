import os
import shutil
from tqdm import tqdm

dataset_folder = '../../../Datasets/Vessel_02/'
target_mesh_extension = '.obj'
save_folder = '../../../Datasets/Vessels_renamed/'

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
    new_name = os.path.join(save_folder, str(idx + 1) + target_mesh_extension)
    shutil.copy(mesh, new_name)
