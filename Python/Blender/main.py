import bpy
from addon_utils import enable
import random
import os


def log(msg):
    s = round(time.time() - startTime, 2)
    print("{}s {}".format(s, msg))


# make sure cell fracture is enabled
enable("object_fracture_cell")

context = bpy.context

# recursive file of .obj files in a directory
meshes = []
folder = 'C:/Users/allopezr/Documents/GitHub/BreakingBad/Vessels_200/'
export_folder = 'C:/Users/allopezr/Documents/GitHub/BreakingBad/dataset_cell_fracture_10000/'
target_pieces_mesh = 1000
modifier_name = 'DecimateMod'

for root, dirs, files in os.walk(folder):
    for file in files:
        if file.endswith('.obj'):
            print(file)
            meshes.append(os.path.join(root, file))

for mesh in meshes:
    bpy.ops.wm.obj_import(filepath=mesh)

    # decimate
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.object.select_all(action='SELECT')

    for i, obj in enumerate(bpy.context.selected_objects):
        print(len(obj.data.polygons))
        decimate_ratio = 10000.0 / len(obj.data.polygons)
        if decimate_ratio < 1.0:
            modifier = obj.modifiers.new(modifier_name, 'DECIMATE')
            modifier.ratio = decimate_ratio
            modifier.use_collapse_triangulate = True
            bpy.ops.object.modifier_apply(modifier=modifier_name)

            print(len(obj.data.polygons))

    n_pieces = 0
    it = 0

    print(mesh)

    while n_pieces < target_pieces_mesh:
        # cell fracture
        bpy.ops.object.select_all(action='DESELECT')
        bpy.ops.object.select_all(action='SELECT')

        mesh_name = mesh.split('.')[0].split('/')[-1].split('\\')[-1]
        export_folder_obj = export_folder + mesh_name + '/fracture_' + str(it) + '/'
        if not os.path.exists(export_folder_obj):
            os.makedirs(export_folder_obj)

        # random number of fragments
        n = random.randint(2, 10)
        bpy.ops.object.add_fracture_cell_objects(source_limit=n, recursion_source_limit=1)

        # save individual objects
        selected_objects = bpy.context.selected_objects
        for i, obj in enumerate(selected_objects):
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(state=True)

            export_name = export_folder_obj + 'piece_' + str(i) + '.obj'
            bpy.ops.wm.obj_export(filepath=export_name, export_selected_objects=True)
            bpy.ops.object.delete()

        n_pieces += n
        it += 1

    # delete original object
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()