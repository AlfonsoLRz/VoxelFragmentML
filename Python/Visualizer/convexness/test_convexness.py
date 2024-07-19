import convexness
import rendering
import trimesh

# input
mesh = trimesh.load_mesh("models/dragon.obj")

# rotate mesh
#mesh.apply_transform(trimesh.transformations.rotation_matrix(-np.pi / 2, [1, 0, 0]))

# scale to fit in unit cube
mesh.apply_scale(10 / mesh.extents.max())

points, point_convexness, convexness = (convexness.measure_convexness(mesh, 1000, 10))

print("Convexness:", convexness)

rendering.render_points([points], geometry=["ParticleSetConstSize"], r=0.2, colors=[point_convexness])



