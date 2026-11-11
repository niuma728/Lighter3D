"""Run with Blender 4.5: blender --background --python assets/source/create_lighter.py.

Blender uses Z up; exported OBJ vertices explicitly use the application's Y-up
assembly coordinates. No object is individually centered or rescaled on export.
"""

from pathlib import Path
import math
import bpy

ROOT = Path(__file__).resolve().parents[2]
MODELS = ROOT / "assets/models"
MODELS.mkdir(parents=True, exist_ok=True)
CHIMNEY_X = -0.08
WHEEL_CENTER = (0.50, 1.035)
CAM_PIVOT = (-0.60, 0.86, 0.0)
VENTS = [(-0.08, 1.21), (-0.25, 1.02), (0.09, 1.02),
         (-0.31, 0.80), (-0.08, 0.80), (0.15, 0.80)]
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)


def blender_position(position):
    x, y, z = position
    return (x, -z, y)


def apply_modifier(obj, modifier):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def box(name, center, size, radius):
    bpy.ops.mesh.primitive_cube_add(size=1, location=blender_position(center))
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = (size[0], size[2], size[1])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bevel = obj.modifiers.new("Rounded edges", "BEVEL")
    bevel.width = radius
    bevel.segments = 4
    apply_modifier(obj, bevel)
    return obj


def shell(name, center, size, cut_center, cut_size):
    obj = box(name, center, size, 0.09)
    cutter = box(name + "_cavity", cut_center, cut_size, 0.045)
    subtract(obj, cutter)
    return obj


def hinge_segment(index):
    z = (index - 2) * 0.12
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=0.09, depth=0.113,
        end_fill_type="NGON", location=blender_position((-0.94, 0.565, z)),
        rotation=(math.pi / 2, 0, 0))
    obj = bpy.context.object
    obj.name = f"hinge_{index}"
    bevel = obj.modifiers.new("Hinge edge", "BEVEL")
    bevel.width = 0.008
    bevel.segments = 2
    apply_modifier(obj, bevel)
    return obj


def cylinder(name, center, radius, depth, axis="Y", vertices=40):
    rotation = (0, 0, 0)
    if axis == "Z":
        rotation = (math.pi / 2, 0, 0)
    elif axis == "X":
        rotation = (0, math.pi / 2, 0)
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth,
        location=blender_position(center), rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    return obj


def subtract(obj, cutter):
    boolean = obj.modifiers.new("Through opening", "BOOLEAN")
    boolean.operation = "DIFFERENCE"
    boolean.solver = "EXACT"
    boolean.object = cutter
    apply_modifier(obj, boolean)
    bpy.data.objects.remove(cutter, do_unlink=True)


def toothed_wheel():
    vertices = []
    count = 128
    center_x, center_y = WHEEL_CENTER
    for z in (-0.115, 0.115):
        for i in range(count):
            angle = 2 * math.pi * i / count
            radius = 0.230 if i % 2 == 0 else 0.210
            vertices.append(blender_position((center_x + radius * math.cos(angle),
                                             center_y + radius * math.sin(angle), z)))
    faces = [tuple(reversed(range(count))), tuple(range(count, 2 * count))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    mesh = bpy.data.meshes.new("Wheel teeth")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("wheel_teeth", mesh)
    bpy.context.collection.objects.link(obj)
    # Recalculate outward normals after explicit application-coordinate construction.
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def profile_part(name, outline, center_z, thickness, bevel_width=0.012):
    count = len(outline)
    vertices = [blender_position((x, y, z))
                for z in (center_z - thickness / 2, center_z + thickness / 2)
                for x, y in outline]
    faces = [tuple(reversed(range(count))), tuple(range(count, 2 * count))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    bevel = obj.modifiers.new("Profile edge rounding", "BEVEL")
    bevel.width = bevel_width
    bevel.segments = 3
    apply_modifier(obj, bevel)
    return obj


def ring(name, center, radius, bore, depth, axis):
    obj = cylinder(name, center, radius, depth, axis)
    subtract(obj, cylinder(name + "_bore", center, bore, depth + 0.1, axis))
    return obj


def finish(objects):
    for obj in objects:
        for face in obj.data.polygons:
            face.use_smooth = True
        normals = obj.modifiers.new("Weighted surface normals", "WEIGHTED_NORMAL")
        normals.keep_sharp = True
        apply_modifier(obj, normals)


def program_vector(v):
    return (v.x, v.z, -v.y)


def export_obj(name, objects):
    """Export evaluated triangles and corner normals in shared assembly space."""
    vertex_offset = 1
    normal_offset = 1
    with (MODELS / f"{name}.obj").open("w", encoding="utf-8", newline="\n") as out:
        out.write("# Lighter3D | Blender source | Y up, front +Z | assembly-space vertices\n")
        for obj in objects:
            mesh = obj.data
            mesh.calc_loop_triangles()
            out.write(f"o {obj.name}\n")
            for vertex in mesh.vertices:
                p = program_vector(obj.matrix_world @ vertex.co)
                out.write("v %.7f %.7f %.7f\n" % p)
            normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
            for normal in mesh.corner_normals:
                n = program_vector((normal_matrix @ normal.vector).normalized())
                out.write("vn %.7f %.7f %.7f\n" % n)
            for triangle in mesh.loop_triangles:
                corners = [f"{vertex_offset + mesh.loops[i].vertex_index}//{normal_offset + i}"
                           for i in triangle.loops]
                out.write("f " + " ".join(corners) + "\n")
            vertex_offset += len(mesh.vertices)
            normal_offset += len(mesh.loops)


body = shell("body", (0, -0.475, 0), (1.8, 2.05, 0.68),
             (0, -0.30, 0), (1.66, 2.19, 0.54))
lid = shell("lid", (0, 1.04, 0), (1.8, 0.92, 0.68),
            (0, 0.88, 0), (1.66, 1.04, 0.54))
hinges = [hinge_segment(i) for i in range(5)]
parts = {"body": [body], "lid": [lid],
         "hinge_fixed": hinges[::2], "hinge_lid": hinges[1::2]}
parts["insert"] = [box("insert", (0, -0.30, 0), (1.56, 1.93, 0.47), 0.035)]
chimney = box("chimney", (CHIMNEY_X, 0.99, 0), (0.68, 0.68, 0.48), 0.025)
subtract(chimney, box("chimney_cavity", (CHIMNEY_X, 1.15, 0), (0.60, 0.90, 0.40), 0.012))
for x, y in VENTS:
    subtract(chimney, cylinder("front_back_vent", (x, y, 0), 0.045, 0.8, "Z", 24))
for y in (0.88, 1.10):
    for z in (-0.11, 0.11):
        subtract(chimney, cylinder("side_vent", (CHIMNEY_X, y, z), 0.035, 0.9, "X", 24))
parts["chimney"] = [chimney]
parts["wheel"] = [toothed_wheel()]
wheel_x, wheel_y = WHEEL_CENTER
wheel_outline = [(0.25, 0.84), (0.25, 1.29), (0.30, 1.26),
                 (0.56, 1.10), (0.595, 1.04), (0.57, 0.96), (0.51, 0.91)]
cam_outline = [(-0.77, 0.73), (-0.96, 0.80), (-1.03, 0.82), (-1.05, 0.88),
               (-1.02, 0.925), (-0.97, 0.94), (-0.78, 0.94), (-0.67, 0.97),
               (-0.59, 1.02), (-0.52, 1.01), (-0.45, 0.91),
               (-0.47, 0.82), (-0.63, 0.76), (-0.69, 0.68)]
cam_support_outline = [(-0.75, 0.67), (-0.67, 0.77), (-0.65, 0.91),
                       (-0.60, 0.96), (-0.55, 0.93), (-0.44, 0.67)]
parts["supports"] = []
for z in (-0.15, 0.15):
    support = profile_part("wheel_support", wheel_outline, z, 0.045)
    subtract(support, cylinder("wheel_axle_hole", (wheel_x, wheel_y, z), 0.079, 0.15, "Z"))
    parts["supports"].append(support)
    support = profile_part("cam_support", cam_support_outline, z, 0.035)
    subtract(support, cylinder("cam_axle_hole", (CAM_PIVOT[0], CAM_PIVOT[1], z), 0.049, 0.15, "Z"))
    parts["supports"].append(support)
parts["supports"] += [cylinder("wheel_spindle", (wheel_x, wheel_y, 0), 0.042, 0.39, "Z"),
                       cylinder("cam_spindle", CAM_PIVOT, 0.046, 0.39, "Z"),
                       cylinder("flint_guide_base", (wheel_x, 0.675, 0), 0.12, 0.02)]
cam = profile_part("cam", cam_outline, 0, 0.19, 0.02)
subtract(cam, cylinder("cam_pivot_hole", CAM_PIVOT, 0.049, 0.5, "Z"))
parts["cam"] = [cam]
parts["flint"] = [cylinder("flint", (wheel_x, 0.70, 0), 0.055, 0.21)]
parts["flint_guide"] = [ring("flint_guide", (wheel_x, 0.73, 0), 0.10, 0.058, 0.085, "Y")]
for z in (-0.175, 0.175):
    parts["flint_guide"].append(ring("wheel_bearing", (wheel_x, wheel_y, z), 0.077, 0.045, 0.018, "Z"))
parts["wick"] = [cylinder("wick", (CHIMNEY_X, 0.985, 0), 0.047, 0.50, vertices=20)]
finish([obj for objects in parts.values() for obj in objects])

metal = bpy.data.materials.new("Silver metal")
metal.diffuse_color = (0.64, 0.68, 0.72, 1)
metal.use_nodes = True
principled = metal.node_tree.nodes.get("Principled BSDF")
principled.inputs["Metallic"].default_value = 0.85
principled.inputs["Roughness"].default_value = 0.24
for name, objects in parts.items():
    material = metal
    if name in ("wheel", "flint", "wick", "cam", "flint_guide"):
        material = metal.copy()
        material.name = name + " material"
        color = (0.06, 0.065, 0.07, 1)
        if name == "wick":
            color = (0.32, 0.23, 0.12, 1)
        elif name == "flint_guide":
            color = (0.60, 0.43, 0.12, 1)
        material.diffuse_color = color
        material.node_tree.nodes.get("Principled BSDF").inputs["Base Color"].default_value = color
    for obj in objects:
        obj.data.materials.append(material)

bpy.context.scene.unit_settings.system = "NONE"
bpy.context.scene["coordinate_convention"] = "OBJ/application: X right, Y up, front +Z"
bpy.context.scene["hinge_pivot_application"] = [-0.94, 0.565, 0.0]
bpy.context.scene["hinge_axis_application"] = [0.0, 0.0, 1.0]
bpy.context.scene["open_angle_degrees"] = 115.0
bpy.context.scene["wheel_pivot_application"] = [wheel_x, wheel_y, 0.0]
bpy.context.scene["flame_anchor_application"] = [CHIMNEY_X, 1.235, 0.0]
bpy.context.scene["cam_pivot_application"] = list(CAM_PIVOT)
bpy.context.scene["cam_closed_angle_degrees"] = -65.0
bpy.context.scene["vent_centers_application"] = [v for point in VENTS for v in point]
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "assets/source/lighter.blend"))
for name, objects in parts.items():
    export_obj(name, objects)
print("Exported", len(parts), "assembled lighter parts to", MODELS)
