"""Original practical archery props; run in Blender's main thread through MCP.

Metre sources, centimetre FBX delivery. No character or skeleton is modified.
"""
from pathlib import Path
import json
import math
import sys

import bpy
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir, write_task_meta

QA = ROOT / 'docs/qa/TASK-095/archery-kit'
QA.mkdir(parents=True, exist_ok=True)
for obj in list(bpy.data.objects):
    bpy.data.objects.remove(obj, do_unlink=True)
for collection in [bpy.data.meshes, bpy.data.materials, bpy.data.images]:
    for block in list(collection):
        if block.users == 0:
            collection.remove(block)
parts = []


def material(name, color, roughness=.82, metal=0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = (*color, 1)
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Roughness'].default_value = roughness
    bsdf.inputs['Metallic'].default_value = metal
    return mat


wood = material('M_Archery_Yew', (.27, .125, .041))
leather = material('M_Archery_Leather', (.065, .028, .012), .9)
edge = material('M_Archery_LeatherEdge', (.12, .057, .023), .86)
inside = material('M_Archery_Inside', (.027, .013, .007), .98)
hemp = material('M_Archery_Hemp', (.42, .32, .19), .95)
iron = material('M_Archery_Iron', (.11, .12, .13), .65, .6)
feather = material('M_Archery_Feather', (.44, .40, .31), .94)
dark_feather = material('M_Archery_FeatherBarb', (.16, .13, .095), .96)


def grain(mat, scale, dark, light):
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    coords = nodes.new('ShaderNodeTexCoord')
    stretch = nodes.new('ShaderNodeVectorMath')
    stretch.operation = 'MULTIPLY'
    stretch.inputs[1].default_value = scale
    links.new(coords.outputs['Generated'], stretch.inputs[0])
    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 1
    noise.inputs['Detail'].default_value = 2
    links.new(stretch.outputs['Vector'], noise.inputs['Vector'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = (*dark, 1)
    ramp.color_ramp.elements[1].color = (*light, 1)
    links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], nodes.get('Principled BSDF').inputs['Base Color'])


grain(wood, (85, 85, 3), (.10, .041, .012), (.34, .17, .055))
grain(leather, (170, 170, 170), (.035, .013, .005), (.095, .039, .015))
grain(feather, (5, 5, 70), (.25, .22, .16), (.50, .46, .36))


def mesh(name, vertices, faces, mat, smooth=False):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    data.materials.append(mat)
    for face in data.polygons:
        face.use_smooth = smooth
    parts.append(obj)
    return obj


def tube(name, points, radii, mat, sides=8):
    points = [Vector(p) for p in points]
    verts, faces = [], []
    for i, point in enumerate(points):
        tangent = points[min(i + 1, len(points) - 1)] - points[max(0, i - 1)]
        tangent.normalize()
        u = tangent.cross(Vector((0, 1, 0))).normalized()
        if u.length < .1:
            u = tangent.cross(Vector((1, 0, 0))).normalized()
        v = tangent.cross(u).normalized()
        rx, ry = radii[i] if isinstance(radii[i], tuple) else (radii[i], radii[i])
        for j in range(sides):
            angle = j * math.tau / sides
            verts.append(point + u * (rx * math.cos(angle)) + v * (ry * math.sin(angle)))
        if i:
            for j in range(sides):
                a = (i - 1) * sides + j
                b = (i - 1) * sides + (j + 1) % sides
                faces.append((a, b, b + sides, a + sides))
    faces.extend([tuple(reversed(range(sides))), tuple(range(len(verts) - sides, len(verts)))])
    return mesh(name, verts, faces, mat, True)


def cord(name, a, b, radius, mat=hemp):
    return tube(name, [a, b], [radius, radius], mat, 6)


def ring(name, center, rx, ry, radius, mat):
    segments = 32 if max(rx, ry) > .025 else 12
    points = [(center[0] + rx * math.cos(i * math.tau / segments),
               center[1] + ry * math.sin(i * math.tau / segments), center[2]) for i in range(segments + 1)]
    return tube(name, points, [radius] * len(points), mat, 6)


def bow():
    points, radii = [], []
    for i in range(49):
        t = -1 + i / 24
        points.append((.15 * (1 - abs(t) ** 1.6), 0, .75 * t))
        radii.append((.005 + .009 * (1 - abs(t)), .003 + .011 * (1 - abs(t) ** .7)))
    tube('YewStave', points, radii, wood, 12)
    cord('Bowstring', (0, 0, -.75), (0, 0, .75), .0009)
    for z in [-.742, .742]:
        ring('HornNock', (.003, 0, z), .006, .005, .0018, iron)
    for i in range(13):
        ring('GripWrap', (.149, 0, -.05 + i * .008), .015, .014, .0037, leather)
    for z in [-.06, .06]:
        ring('GripBinding', (.146, 0, z), .014, .014, .0016, hemp)
    # The hand-grip is the attachment origin.
    for obj in parts:
        obj.location.x -= .15


def arrow(offset=(0, 0, 0), length=.79):
    origin = Vector(offset)
    tube('Shaft', [origin + Vector((0, 0, .035)), origin + Vector((0, 0, length))],
         [.0034, .0030], wood, 8)
    verts = [origin + Vector((0, 0, 0))]
    verts += [origin + Vector((.006 * math.cos(i * math.pi / 2),
                                .006 * math.sin(i * math.pi / 2), .036)) for i in range(4)]
    verts.append(origin + Vector((0, 0, .06)))
    mesh('ForgedBodkin', verts, [(0, 1 + i, 1 + (i + 1) % 4) for i in range(4)] +
         [(5, 1 + (i + 1) % 4, 1 + i) for i in range(4)], iron)
    for k in range(3):
        angle = k * math.tau / 3
        radial = Vector((math.cos(angle), math.sin(angle), 0))
        side = Vector((-math.sin(angle), math.cos(angle), 0)) * .0004
        outline = [(length - .145, .003), (length - .118, .016),
                   (length - .032, .021), (length - .022, .003)]
        verts = [origin + radial * r + Vector((0, 0, z)) + sign * side
                 for sign in [-1, 1] for z, r in outline]
        obj = mesh('Fletching', verts, [(0, 3, 2, 1), (4, 5, 6, 7),
                   (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)], feather)
        obj.data.materials.append(dark_feather)
        for i in range(8):
            z = length - .116 + i * .01
            cord('FeatherBarb', origin + radial * .004 + Vector((0, 0, z)),
                 origin + radial * (.015 + i * .00065) + Vector((0, 0, z + .014)),
                 .00035, dark_feather)
    for z in [length - .146, length - .019]:
        for i in range(3):
            ring('FletchingBinding', origin + Vector((0, 0, z + i * .002)), .0034, .0034, .0007, hemp)
    # Two prongs make an actual visible nock groove.
    for x in [-.0024, .0024]:
        cord('NockProng', origin + Vector((x, 0, length - .005)),
             origin + Vector((x, 0, length + .006)), .0011, edge)


def quiver():
    sides = 32
    rings = [(0, .043), (.02, .047), (.18, .055), (.40, .066), (.53, .071)]
    vertices, faces = [], []
    for inner in [False, True]:
        for z, radius in rings:
            radius -= .004 if inner else 0
            for i in range(sides):
                a = math.tau * i / sides
                vertices.append((radius * .78 * math.cos(a), radius * math.sin(a), z))
    layer = len(rings) * sides
    for inner in [0, 1]:
        for j in range(len(rings) - 1):
            for i in range(sides):
                a = inner * layer + j * sides + i
                b = inner * layer + j * sides + (i + 1) % sides
                face = (a, b, b + sides, a + sides)
                faces.append(tuple(reversed(face)) if inner else face)
    for i in range(sides):
        a = (len(rings) - 1) * sides + i
        b = (len(rings) - 1) * sides + (i + 1) % sides
        faces.append((a, b, b + layer, a + layer))
    faces += [tuple(reversed(range(sides))), tuple(range(layer, layer + sides))]
    obj = mesh('OpenLeatherQuiver', vertices, faces, leather, True)
    obj.data.materials.append(inside)
    for face in obj.data.polygons[128:256]:
        face.material_index = 1
    for z, radius in [(.02, .047), (.105, .051), (.43, .067), (.528, .071)]:
        ring('RolledLeatherBinding', (0, 0, z), radius * .78, radius, .0035, edge)
    for i in range(42):
        z = .023 + i * .0115
        r = .047 + z / .53 * .024
        cord('SaddleStitch', (r * .78 + .001, -.003, z),
             (r * .78 + .001, .003, z + .005), .0008)
    # A closed back loop allows a belt or a later character-specific sling to pass through.
    for z in [.12, .43]:
        points = [(-.051, -.022, z - .024), (-.081, -.022, z - .024),
                  (-.081, -.022, z + .024), (-.051, -.022, z + .024)]
        tube('AttachmentLoop', points, [(.008, .002)] * 4, edge, 8)
        for zz in [z - .023, z + .023]:
            cord('Rivet', (-.055, -.022, zz), (-.059, -.022, zz), .003, iron)
    for i in range(5):
        angle = i * math.tau / 5
        arrow((.022 * math.cos(angle), .03 * math.sin(angle), .014), .72 + (i % 3) * .022)


scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.samples = 16
scene.render.resolution_x = 1200
scene.render.resolution_y = 1000
scene.render.resolution_percentage = 100
scene.world = bpy.data.worlds.new('ArcheryReviewWorld')
scene.world.color = (.12, .12, .12)
scene.view_settings.view_transform = 'AgX'
report, assets = [], []
for label, build in [('Longbow', bow), ('Arrow', arrow), ('Quiver', quiver)]:
    parts.clear()
    build()
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = 'SM_' + label + '_Practical'
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    if label == 'Arrow':
        obj.rotation_euler.y = -math.pi / 2
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.data.calc_loop_triangles()
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(island_margin=.015)
    bpy.ops.object.mode_set(mode='OBJECT')
    output = task_output_dir('Hearthward', '3d_object', 'TASK-095-' + label.lower(), 'blender-20261008')
    output.mkdir(parents=True, exist_ok=True)
    # Explicit color-only bake keeps the FBX independent of Blender shaders.
    image = bpy.data.images.new('T_' + label + '_BaseColor', width=1024, height=1024, alpha=False)
    for mat in obj.data.materials:
        node = mat.node_tree.nodes.new('ShaderNodeTexImage')
        node.image = image
        mat.node_tree.nodes.active = node
    scene.render.bake.margin = 8
    bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'})
    image.filepath_raw = str(output / (image.name + '.png'))
    image.file_format = 'PNG'
    image.save()
    image.pack()
    delivery = material('M_' + label + '_Practical', (.2, .1, .04))
    tex = delivery.node_tree.nodes.new('ShaderNodeTexImage')
    tex.image = image
    delivery.node_tree.links.new(tex.outputs['Color'], delivery.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
    obj.data.materials.clear()
    obj.data.materials.append(delivery)
    for face in obj.data.polygons:
        face.material_index = 0
    fbx = output / (obj.name + '.fbx')
    # The editor's MCP importer does not apply FBX unit metadata. Match the
    # established R3 exporter: actual centimetre coordinates, identity scales.
    source_data = obj.data
    obj.data = source_data.copy()
    obj.data.transform(Matrix.Scale(100, 4))
    old_units = (scene.unit_settings.system, scene.unit_settings.scale_length)
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = .01
    try:
        bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'MESH'},
                                apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z', axis_up='Y',
                                bake_anim=False, add_leaf_bones=False, path_mode='COPY', embed_textures=True)
    finally:
        export_data = obj.data
        obj.data = source_data
        bpy.data.meshes.remove(export_data)
        scene.unit_settings.system, scene.unit_settings.scale_length = old_units
        bpy.context.view_layer.update()
    write_task_meta(output, {'game_id': 'Hearthward', 'task_kind': '3d_object',
                    'task_id': 'TASK-095-' + label.lower(), 'run_id': 'blender-20261008',
                    'artifact_key': 'model_path', 'model_path': str(fbx),
                    'source_route': 'original authored geometry', 'units': 'centimetres'})
    report.append({'piece': label, 'fbx': str(fbx), 'texture': str(output / (image.name + '.png')),
                   'dimensions_m': list(obj.dimensions), 'triangles': len(obj.data.loop_triangles),
                   'source': 'Original geometry and materials authored for Hearthward',
                   'character_modified': False, 'ue_import': 'NOT_RUN', 'owner_visual': 'NOT_RUN'})
    assets.append(obj)
    obj.hide_set(True)
    obj.hide_render = True

for obj in assets:
    obj.hide_set(False)
    obj.hide_render = False
assets[0].location = (-.28, 0, .79)
assets[1].rotation_euler.y = math.pi / 2
assets[1].location = (.29, -.04, .06)
assets[2].location = (.14, .025, .04)
target = Vector((0, 0, .8))
for loc, power, size in [((2, -3, 4), 650, 3), ((-3, 1, 2), 500, 2)]:
    bpy.ops.object.light_add(type='AREA', location=loc)
    lamp = bpy.context.object
    lamp.data.energy = power
    lamp.data.size = size
    lamp.rotation_euler = (target - lamp.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.camera_add(location=(2.8, -4, 2.1))
camera = bpy.context.object
scene.camera = camera
camera.data.type = 'ORTHO'
camera.data.ortho_scale = 2.25
camera.rotation_euler = (target - camera.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / 'art_source/TASK-095/ArcheryKit.blend'))
scene.render.filepath = str(QA / 'archery-kit.png')
bpy.ops.render.render(write_still=True)
camera.location = (1.3, -2.3, 1.3)
target = Vector((.12, 0, .53))
camera.rotation_euler = (target - camera.location).to_track_quat('-Z', 'Y').to_euler()
camera.data.ortho_scale = 1.12
scene.render.filepath = str(QA / 'quiver-detail.png')
bpy.ops.render.render(write_still=True)
(QA / 'source-results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print('ARCHERY_KIT_COMPLETE', json.dumps(report))
