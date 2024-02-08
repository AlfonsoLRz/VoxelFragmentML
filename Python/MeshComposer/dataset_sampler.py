import numpy as np
import os
import shutil
from tqdm import tqdm

dataset_folder = '../../../Datasets/Vessel_02/'
target_mesh_extension = '.obj'
save_folder = '../../../Datasets/Vessels_200/'
num_aimed_meshes = 200
clean_save_folder = True

# Retrieve meshes that meet the requirements [recursive]
meshes = []
for root, dirs, files in os.walk(dataset_folder):
    for file in files:
        if file.endswith(target_mesh_extension):
            meshes.append(os.path.join(root, file))
print('Found ' + str(len(meshes)) + ' meshes')

# Sample meshes
if len(meshes) > num_aimed_meshes:
    random_indices = np.random.choice(len(meshes), num_aimed_meshes, replace=False)
    meshes = [meshes[i] for i in random_indices]

# Save sampled meshes
if not os.path.exists(save_folder):
    os.makedirs(save_folder)

if clean_save_folder:
    # Remove all files in the folder
    for root, dirs, files in os.walk(save_folder):
        for file in files:
            os.remove(os.path.join(root, file))

print('Saving ' + str(len(meshes)) + ' meshes')
for mesh in tqdm(meshes):
    shutil.copy(mesh, save_folder)
print('Saved ' + str(len(meshes)) + ' meshes')