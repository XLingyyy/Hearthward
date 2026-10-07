"""Author the approved practical facility samples in Blender, in metres."""
from pathlib import Path
import json
import math
import sys
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir, write_task_meta

OUT = Path(__file__).resolve().parent
QA = ROOT / 'docs/qa/TASK-098/samples'
QA.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def material(name, color, roughness, metallic=0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = (*color, 1)
    shader.inputs['Roughness'].default_value = roughness
    shader.inputs['Metallic'].default_value = metallic
    return mat

wood = material('M_PracticalWood', (.22, .105, .044), .88)
endwood = material('M_PracticalEndwood', (.30, .17, .075), .9)
iron = material('M_PracticalIron', (.11, .115, .12), .69, .75)
cloth = material('M_PracticalCloth', (.42, .34, .20), .95)
objects = []

def box(name, loc, size, mat, bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    if bevel:
        mod = obj.modifiers.new('WornEdge', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    objects.append(obj)
    return obj

def cylinder(name, loc, radius, depth, mat, rotation=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=radius, depth=depth, location=loc)
    obj = bpy.context.object
    obj.name = name
    if rotation:
        obj.rotation_euler = rotation
    obj.data.materials.append(mat)
    objects.append(obj)
    return obj

def workbench():
    for x in [-.5, .5]:
        for y in [-.30, .30]:
            box('Leg', (x,y,.34), (.09,.09,.68), wood)
        box('CrossRail', (x,0,.2), (.075,.68,.075), wood)
    box('LongBrace',(0,0,.18),(1.05,.07,.075),wood)
    for y in [-.25,0,.25]:
        box('TopPlank',(0,y,.70),(1.25,.24,.09),endwood)
    box('ViceFixed',(.39,-.31,.80),(.18,.07,.14),wood)
    box('ViceJaw',(.39,-.46,.80),(.18,.06,.14),wood)
    cylinder('ViceScrew',(.39,-.39,.77),.016,.30,iron,(math.pi/2,0,0))
    cylinder('ViceHandle',(.39,-.55,.77),.012,.22,wood)
    box('Workpiece',(-.15,.06,.775),(.45,.19,.055),wood)
    box('MalletHandle',(-.40,-.12,.78),(.035,.3,.035),wood)
    box('MalletHead',(-.40,.02,.80),(.16,.07,.07),endwood)

def forge():
    for x in [-.23,.23]:
        for y in [-.20,.20]:
            box('StumpBoard',(x,y,.23),(.20,.18,.46),wood)
    box('StumpTop',(0,0,.48),(.65,.57,.10),endwood)
    box('AnvilFoot',(0,0,.57),(.43,.29,.09),iron)
    box('AnvilWaist',(-.035,0,.70),(.22,.19,.22),iron)
    box('AnvilFace',(-.025,0,.84),(.55,.26,.075),iron,.012)
    bpy.ops.mesh.primitive_cone_add(vertices=12, radius1=.105, radius2=.016, depth=.28,
                                  location=(.34,0,.825), rotation=(0,math.pi/2,0))
    obj=bpy.context.object;obj.name='AnvilHorn';obj.scale.y=.82;obj.data.materials.append(iron);objects.append(obj)
    box('HammerHandle',(-.20,-.18,.91),(.035,.31,.035),wood)
    box('HammerHead',(-.20,-.035,.935),(.14,.075,.065),iron)
    for x in [-.09,-.035]:
        box('TongsArm',(x,.31,.59),(.022,.42,.025),iron,.003)

def warehouse():
    for x in [-.50,.50]:
        box('Skid',(x,0,.055),(.12,.72,.11),wood)
    for z in [.19,.36,.53,.70]:
        for y in [-.32,.32]:
            box('ChestFront',(0,y,z),(1.15,.055,.16),endwood)
        for x in [-.55,.55]:
            box('ChestSide',(x,0,z),(.055,.64,.16),wood)
    for y in [-.26,-.085,.085,.26]:
        box('ChestLid',(0,y,.81),(1.18,.16,.07),wood)
    for x in [-.39,.39]:
        for y in [-.355,.355]:
            box('IronStrap',(x,y,.47),(.045,.015,.72),iron,.002)
        box('LidStrap',(x,0,.854),(.045,.72,.012),iron,.002)
    box('Latch',(0,-.36,.74),(.08,.023,.16),iron,.004)
    for x in [-.39,.39]:
        for z in [.19,.53,.74]:
            cylinder('Rivet',(x,-.37,z),.012,.012,iron,(math.pi/2,0,0))

scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 20
scene.render.resolution_x = 1000
scene.render.resolution_y = 760
scene.render.resolution_percentage = 100
scene.world.color = (.18,.18,.18)
scene.view_settings.view_transform = 'AgX'
report = []
for label, build in [('Workbench',workbench),('Forge',forge),('Warehouse',warehouse)]:
    objects.clear()
    build()
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects: obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    mesh = bpy.context.object
    mesh.name = 'SM_'+label+'_Practical'
    scene.cursor.location = (0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    mesh.data.calc_loop_triangles()
    descriptor = dict(game_id='Hearthward', task_kind='3d_object',
                      task_id='TASK-098-'+label.lower(), run_id='approved-20261007', artifact_key='model_path')
    directory = task_output_dir(descriptor['game_id'],descriptor['task_kind'],descriptor['task_id'],run_id=descriptor['run_id'])
    directory.mkdir(parents=True,exist_ok=True)
    fbx = directory/(mesh.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},
                             apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',
                             bake_anim=False,add_leaf_bones=False)
    write_task_meta(directory,{**descriptor,'model_path':str(fbx),'source_route':'authored rigid assembly',
                               'units':'metres','up':'+Z','front':'-Y'})
    report.append({'piece':label,'descriptor':descriptor,'fbx':str(fbx),
                   'dimensions_m':list(mesh.dimensions),'triangles':len(mesh.data.loop_triangles),
                   'source':'original authored rigid assembly under approved 098-A','owner_visual':'NOT_RUN'})
    # The saved blend is the editable asset source, with the export at local origin.
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(label+'.blend')))
    ground=box('ReviewGround',(0,0,-.03),(200,200,.04),material('ReviewGround_'+label,(.09,.10,.105),.9),0)
    bpy.ops.object.light_add(type='AREA',location=(-3,-4,5))
    key=bpy.context.object;key.data.energy=550;key.data.shape='DISK';key.data.size=4
    key.rotation_euler=(Vector((0,0,.4))-key.location).to_track_quat('-Z','Y').to_euler()
    bpy.ops.object.light_add(type='AREA',location=(3,2,3))
    fill=bpy.context.object;fill.data.energy=350;fill.data.size=3
    fill.rotation_euler=(Vector((0,0,.4))-fill.location).to_track_quat('-Z','Y').to_euler()
    bpy.ops.object.camera_add(location=(1.8,-2.3,1.65))
    camera=bpy.context.object;scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=1.95
    for name,position in [('front',(1.8,-2.3,1.65)),('back',(-1.8,2.3,1.65)),('top',(0,-.01,3))]:
        camera.location=position
        camera.rotation_euler=(Vector((0,0,.42))-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(QA/(label+'-'+name+'.png'))
        bpy.ops.render.render(write_still=True)
        assert Path(scene.render.filepath).is_file()
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
(QA/'source-results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('FACILITY_SAMPLES_COMPLETE',len(report),flush=True)
