"""Run: blender --background --python-exit-code 1 assets/source/lighter.blend --python assets/source/validate_lighter.py."""
import bpy
import bmesh
import math
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

def tree(obj, transform=Matrix.Identity(4)):
    mesh = bmesh.new()
    mesh.from_mesh(obj.data)
    mesh.transform(transform @ obj.matrix_world)
    result = BVHTree.FromBMesh(mesh, epsilon=0.00001)
    mesh.free()
    return result

def blender_vector(value):
    return Vector((value[0], -value[2], value[1]))

def program_bounds(obj):
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    points = [(v.x, v.z, -v.y) for v in corners]
    return [(min(p[i] for p in points), max(p[i] for p in points)) for i in range(3)]

for obj in bpy.data.objects:
    if obj.type != 'MESH':
        continue
    mesh = bmesh.new()
    mesh.from_mesh(obj.data)
    if any(not edge.is_manifold for edge in mesh.edges):
        raise RuntimeError('Non-manifold geometry: ' + obj.name)
    if any(not math.isfinite(component) for vertex in mesh.verts for component in vertex.co):
        raise RuntimeError('Non-finite geometry: ' + obj.name)
    mesh.free()

wheel_bounds = program_bounds(bpy.data.objects['wheel_teeth'])
flint_bounds = program_bounds(bpy.data.objects['flint'])
chimney_bounds = program_bounds(bpy.data.objects['chimney'])
cam_bounds = program_bounds(bpy.data.objects['cam'])
wheel_x = sum(wheel_bounds[0]) / 2
flint_x = sum(flint_bounds[0]) / 2
chimney_x = sum(chimney_bounds[0]) / 2
if not (sum(cam_bounds[0]) / 2 < chimney_x < wheel_x):
    raise RuntimeError('Reference layout must be cam / chimney / wheel from left to right')
if abs(wheel_x - flint_x) > 0.0001 or abs(wheel_bounds[1][0] - flint_bounds[1][1]) > 0.0001:
    raise RuntimeError('Flint must contact the underside of the wheel on its centerline')
if abs(bpy.context.scene['flame_anchor_application'][0] - chimney_x) > 0.0001:
    raise RuntimeError('Flame anchor is not centered in the chimney')
print('Reference layout and flint-to-wheel contact alignment: PASS')

pivot = blender_vector(bpy.context.scene['hinge_pivot_application'])
axis = blender_vector(bpy.context.scene['hinge_axis_application'])
fixed = [o for o in bpy.data.objects if o.type == 'MESH' and o.name not in ('lid', 'cam') and not o.name.startswith('hinge_')]
trees = {o.name: tree(o) for o in fixed}
cam_pivot = blender_vector(bpy.context.scene['cam_pivot_application'])
cam_closed_angle = bpy.context.scene['cam_closed_angle_degrees']
collisions = []
for angle in range(116):
    transform = Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(angle), 4, axis) @ Matrix.Translation(-pivot)
    moving = tree(bpy.data.objects['lid'], transform)
    cam_angle = math.radians(cam_closed_angle * (1 - angle / 115))
    cam_transform = Matrix.Translation(cam_pivot) @ Matrix.Rotation(cam_angle, 4, axis) @ Matrix.Translation(-cam_pivot)
    moving_cam = tree(bpy.data.objects['cam'], cam_transform)
    if moving.overlap(moving_cam):
        collisions.append((angle, 'lid/cam'))
    for name, fixed_tree in trees.items():
        if moving.overlap(fixed_tree):
            collisions.append((angle, name))
        if moving_cam.overlap(fixed_tree):
            collisions.append((angle, 'cam/' + name))
print('Lid and cam clearance scan 0..115 degrees, 1 degree steps:', collisions or 'PASS, no surface intersections')
if collisions:
    raise RuntimeError('Lid intersects internal parts')

chimney = trees['chimney']
vents = list(bpy.context.scene['vent_centers_application'])
for x, y in zip(vents[::2], vents[1::2]):
    hit, *_ = chimney.ray_cast(blender_vector((x, y, 0.8)), blender_vector((0, 0, -1)), 1.6)
    if hit is not None:
        raise RuntimeError('Blocked chimney vent')
hit, *_ = chimney.ray_cast(blender_vector((-0.08, 0.95, 0.8)), blender_vector((0, 0, -1)), 1.6)
if hit is None:
    raise RuntimeError('Chimney wall missing between vents')
print('Manifold/finite geometry and six front-to-back ventilation paths: PASS')
