import itertools
import math
import numpy as np
import scipy
import tqdm
import trimesh


def measure_convexness(mesh, num_samples, n_ray_splits=10):
    # samples n points from the mesh
    samples, face_id = mesh.sample(num_samples, return_index=True)

    # compute the barycentric coordinates of each sample
    barycentric_cd = trimesh.triangles.points_to_barycentric(triangles=mesh.triangles[face_id], points=samples)

    # interpolate vertex normals from barycentric coordinates
    interpolated_normal = trimesh.unitize((mesh.vertex_normals[mesh.faces[face_id]] * trimesh.unitize(barycentric_cd)
                                           .reshape((-1, 3, 1))).sum(axis=1))

    # push points a bit inwards according to the bounding box
    max_p, min_p = np.max(samples, axis=0), np.min(samples, axis=0)
    bounding_box_diagonal = np.linalg.norm(max_p - min_p)
    pushed_samples = samples - interpolated_normal * bounding_box_diagonal * 1e-4

    # detect which ones are inside
    inside = mesh.contains(pushed_samples)

    # filter out the ones that are outside
    pushed_samples = pushed_samples[inside]

    # shoot rays for all pairs
    point_indices = [i for i in range(0, pushed_samples.shape[0])]
    permutations = np.array(list(itertools.combinations(point_indices, 2)))
    direction = pushed_samples[permutations[:, 1]] - pushed_samples[permutations[:, 0]]
    #direction = direction / np.linalg.norm(direction, axis=1)[:, None]

    # split in n iterations
    convexity = .0
    n, n_rays = n_ray_splits, 0
    split_size = math.ceil(direction.shape[0] / n)

    for i in range(n):
        start = i * split_size
        end = min((i + 1) * split_size, direction.shape[0])

        current_permutations = permutations[start:end]
        ray_origins = pushed_samples[current_permutations[:, 0]]
        ray_directions = direction[start:end]
        locations, index_ray, _ = mesh.ray.intersects_location(ray_origins=ray_origins, ray_directions=ray_directions,
                                                               multiple_hits=False)

        # ray origins according to the returned index_ray
        ray_origins = ray_origins[index_ray]
        ray_directions = ray_directions[index_ray]
        length = np.linalg.norm(ray_directions, axis=1)

        # find parametric distance according to directions (may not be normalized)
        T = (locations[:, 0] - ray_origins[:, 0]) / ray_directions[:, 0]

        #
        epsilon = 1e-8 * bounding_box_diagonal
        bad = np.logical_and(T > epsilon, T < 1 - epsilon)
        bad = np.logical_not(bad)
        convexity += np.sum(bad)
        n_rays += bad.shape[0]

    # zero-aware division
    convexity /= n_rays

    return convexity
