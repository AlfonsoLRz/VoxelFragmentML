import numpy as np
import matplotlib.pyplot as plt
import trimesh


class TriangleMesh:
    def __init__(self, filename):
        self._mesh = trimesh.load(filename, force='mesh')
        if not self._mesh:
            raise Exception('Failed to load mesh from file: {}'.format(filename))

    def get_boundary_vertices(self, plane_y=0, separate_inner=False):
        # Get vertices close to plane_y
        boundary_lines = []
        boundary_faces = []
        head_vertices = self._mesh.vertices
        head_faces = self._mesh.faces

        for id, face in enumerate(head_faces):
            boundary_vertices = []
            for vertex in head_vertices[face]:
                if np.abs(vertex[1] - plane_y) < 0.00001:
                    boundary_vertices.append(vertex)

            if len(boundary_vertices) == 2:
                boundary_lines.extend(boundary_vertices)
                boundary_faces.append(id)

        boundary_lines_pairs = []
        for i in range(0, len(boundary_lines), 2):
            boundary_lines_pairs.append([boundary_lines[i], boundary_lines[i + 1]])

        is_inner = None
        if separate_inner is True:
            centroid = self.get_centroid()

            # Throw rays from centroid to vertices and count number of intersections
            is_inner = np.ones(len(boundary_lines_pairs), dtype=bool)
            for id, pair in enumerate(boundary_lines_pairs):
                # subtract centroid from vertices
                vertex = np.array(pair[0])
                vertex -= centroid
                vertex /= np.linalg.norm(vertex)

                # get vertices' normals
                normal = self._mesh.face_normals[boundary_faces[id]].copy()
                normal /= np.linalg.norm(normal)

                # calculate dot product
                dot_product = np.dot(vertex, normal)
                dot_product = np.sum(dot_product, axis=0)

                if dot_product > 0:
                    is_inner[id] = False

        # Calculate centroid
        boundary_vertices = []
        for pair in boundary_lines_pairs:
            boundary_vertices.append(pair[0])
            boundary_vertices.append(pair[1])
        boundary_vertices = np.array(boundary_vertices)
        boundary_centroid = np.mean(boundary_vertices, axis=0)

        return boundary_lines_pairs, boundary_centroid, is_inner

    def get_aabb(self):
        vertices = self._mesh.vertices[self._mesh.faces]
        flattened_vertices = vertices.reshape(-1, 3)
        min_x, min_y, min_z = np.min(flattened_vertices, axis=0)
        max_x, max_y, max_z = np.max(flattened_vertices, axis=0)

        return np.array([[min_x, min_y, min_z], [max_x, max_y, max_z]])

    def get_centroid(self):
        return self._mesh.centroid.copy()

    def get_projected_triangles_(self, projection_matrix):
        vertices, faces = self._mesh.vertices, self._mesh.faces
        vertices = np.c_[vertices, np.ones(len(vertices))] @ projection_matrix.T
        vertices /= vertices[:, 3].reshape(-1, 1)
        vertices = vertices[faces]
        triangles = vertices[:, :, :2]
        z = -vertices[:, :, 2].mean(axis=1)
        z_min, z_max = z.min(), z.max()
        z = (z - z_min) / (z_max - z_min)
        colors = plt.get_cmap("magma")(z)
        indices = np.argsort(z)
        triangles, colors = triangles[indices, :], colors[indices, :]

        return triangles, colors

    @staticmethod
    def get_projected_triangles(faces, projection_matrix):
        vertices = np.c_[faces, np.ones(len(faces))] @ projection_matrix.T
        vertices /= vertices[:, 3].reshape(-1, 1)
        # Reorganize into groups of three vertices
        triangles = vertices.reshape(-1, 3, 4)
        z = -triangles[:, :, 2].mean(axis=1)
        triangles = triangles[:, :, :2]
        z_min, z_max = z.min(), z.max()
        z = (z - z_min) / (z_max - z_min)
        colors = plt.get_cmap("magma")(z)
        indices = np.argsort(z)
        triangles, colors = triangles[indices, :], colors[indices, :]

        return triangles, colors

    def remove_faces(self, pointing_direction=np.array([0, 1, 0]), sign=False):
        head_faces = self._mesh.faces
        head_normals = self._mesh.face_normals
        if sign is True:
            head_faces = np.zeros(len(head_faces), dtype=bool)
        else:
            head_faces = np.ones(len(head_faces), dtype=bool)

        for id, normal in enumerate(head_normals):
            if np.dot(normal, pointing_direction) > 0.99:
                head_faces[id] = sign

        # count number of final faces
        self._mesh.update_faces(head_faces)
        print("Number of faces after removing: ", len(self._mesh.faces))

    def remove_vertices(self, plane_y=0):
        head_vertices = self._mesh.vertices
        head_faces = self._mesh.faces
        faces_bool = np.ones(len(head_faces), dtype=bool)

        for id, face in enumerate(head_faces):
            for vertex in head_vertices[face]:
                if np.abs(vertex[1] - plane_y) < 0.00001:
                    faces_bool[id] = False

        # count number of final faces
        self._mesh.update_faces(faces_bool)
        print("Number of faces after removing: ", len(self._mesh.faces))

    def plot(self):
        return self._mesh

    def print_stats(self):
        print("Number of vertices: ", len(self._mesh.vertices))
        print("Number of faces: ", len(self._mesh.faces))
        print("Number of edges: ", len(self._mesh.edges))

    def sample(self, num_samples):
        return self._mesh.sample(num_samples)
