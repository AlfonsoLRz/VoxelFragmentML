import numpy as np
import trimesh


class Mesh:
    def __init__(self, path):
        self.mesh = trimesh.load(path)
        self.path = path

    @staticmethod
    def explode(mesh_list):
        centers = np.array([i.mesh.centroid for i in mesh_list])
        center = np.mean(centers, axis=0)

        # move each mesh away from the center
        for i, mesh in enumerate(mesh_list):
            direction = mesh.mesh.centroid - center
            distance = np.linalg.norm(direction)
            mesh.mesh.apply_translation(direction / distance * 0.1 * distance)

    def fix_normals(self):
        self.mesh.fix_normals()

    def assign_color(self, color):
        self.mesh.visual.vertex_colors = color

    @staticmethod
    def compose_scene(mesh_list):
        meshes = [mesh.mesh for mesh in mesh_list]
        return trimesh.Scene(meshes)

    def show(self):
        self.mesh.show()

