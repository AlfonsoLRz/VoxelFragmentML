import os
import shutil
import trimesh
from tqdm import tqdm

dataset_folder = '../../../Datasets/BreakingBad/volume_constrained/artifact_compressed/'
target_mesh_extension = 'compressed_mesh.obj'
save_folder = '../../../Datasets/Artifacts/200/'
flip_y = False
max_models = 200

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

if max_models < len(meshes):
    # randomize
    import random
    random.shuffle(meshes)
    meshes = meshes[:max_models * 2]        # just in case some meshes are too small

    # check if their size is higher than 2kb
    for mesh in meshes:
        if os.path.getsize(mesh) < 2048:
            meshes.remove(mesh)
        # if less than 200kb, remove
        elif os.path.getsize(mesh) < 307200:
            meshes.remove(mesh)

    meshes = meshes[:max_models]

for idx, mesh in tqdm(enumerate(meshes)):
    if flip_y:
        mesh = trimesh.load(mesh)
        mesh.apply_scale([1, -1, 1])
        mesh.export(save_folder + str(idx + 1) + '.' + target_mesh_extension, file_type=target_mesh_extension)
    else:
        new_name = os.path.join(save_folder, str(idx + 1) + '.' + target_mesh_extension)
        shutil.copy(mesh, new_name)
