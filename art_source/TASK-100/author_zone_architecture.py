"""Run in live Blender through MCP. Metres; X is the open through-route."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector

ROOT = Path('G:/GameFactory/Hearthward/art_source/TASK-100')
OUT = Path('G:/GameFactory/test_data/outputs/Hearthward/20261009/assets/3d_object/TASK-100-zones')
OUT.mkdir(parents=True, exist_ok=True)
scene = bpy.data.scenes.new('TASK100_ZoneArchitecture')
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
materials = []
for name, color in [('M_ZoneStone',(.29,.28,.25,1)),('M_ZoneTimber',(.19,.105,.052,1)),('M_ZoneIron',(.10,.105,.11,1))]:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    shader = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    shader.inputs['Base Color'].default_value = color
    shader.inputs['Roughness'].default_value = .88
    mat.diffuse_color = color
    materials.append(mat)

parts=[]; colliders=[]; current=''
def box(name, center, size, material=0, rotation=0, collision=True, bevel=.025):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj=bpy.context.object;obj.name=name;obj.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    obj.rotation_euler[1]=rotation
    obj.data.materials.append(materials[material])
    parts.append(obj)
    if collision:
        c=obj.copy();c.data=obj.data.copy();scene.collection.objects.link(c)
        c.name=f'UCX_{current}_{len(colliders):03d}';c.hide_render=True
        colliders.append(c)
    if bevel:
        modifier=obj.modifiers.new('Soft worn edges','BEVEL');modifier.width=bevel;modifier.segments=2
        bpy.context.view_layer.objects.active=obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    return obj

def post(x,y,height,stone=False):
    box('StoneFoot',(x,y,.12),(.65,.65,.74),0)
    box('Upright',(x,y,height/2),(.32,.32,height),0 if stone else 1)

def roof(length,width,eave,rise):
    # Individual overlapping timber roof courses, no unsupported paper-thin roof.
    half=length/2+.35
    slope=math.atan2(rise,half)
    for side in [-1,1]:
        for j in range(10):
            x=side*half*(j+.5)/10
            z=eave+rise*(1-abs(x)/half)
            box('RoofCourse',(x,0,z),(half/10/math.cos(slope)+.04,width+.7,.12),1,side*slope)
    box('Ridge',(0,0,eave+rise+.10),(.22,width+.9,.24),1)
    for x in [-length/2,length/2]:
        box('EaveBeam',(x,0,eave-.1),(.25,width+.6,.3),1)

report=[]
def begin(name):
    global parts,colliders,current
    parts=[];colliders=[];current=name

def finish():
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join();obj=bpy.context.object;obj.name=current;obj.data.name=current
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True)
    for c in colliders:c.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(OUT/(current+'.fbx')),use_selection=True,
        object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',
        add_leaf_bones=False,bake_anim=False,use_custom_props=False)
    report.append({'mesh':current,'dimensions_m':list(obj.dimensions),'vertices':len(obj.data.vertices),
        'convex_collision_parts':len(colliders),'passage_axis':'X','minimum_clear_width_m':2.4})
    collection=bpy.data.collections.new(current);scene.collection.children.link(collection)
    for o in [obj]+colliders:
        for c in list(o.users_collection):c.objects.unlink(o)
        collection.objects.link(o)
    # Separate production objects for source inspection; exported origin stays at zero.
    offset=(len(report)-1)*13
    for o in [obj]+colliders:o.location.y+=offset
    return obj

begin('SM_RiverGate')
for side in [-1,1]:
    # Two crenellated piers with an unblocked four metre gateway.
    for row in range(8):
        for col in range(2):
            box('GateAshlar',(0,side*(2.02+(col+.5)*.69),row*.54+.24),
                (1.8,.66,.51),0,bevel=.045)
    box('PierCap',(0,side*2.71,4.40),(2.0,1.65,.28),0)
    for x in [-.65,.65]:box('Merlon',(x,side*2.71,4.86),(.48,1.45,.65),0)
box('GateLintel',(0,0,3.82),(1.5,4.25,.52),0)
for y in [-1.8,1.8]:box('GateIronStrap',(-.92,y,2.6),(.10,.16,1.6),2,collision=False)
finish()

begin('SM_WorkshopShelter')
for x in [-2.7,2.7]:
    for y in [-2.8,2.8]:post(x,y,3.1)
roof(5.4,5.6,3.2,1.0)
for side in [-1,1]:
    box('BackLowStone',(0,side*2.8,.4),(4.9,.42,1.0),0)
    box('ToolRail',(0,side*2.8,1.75),(4.9,.12,.14),1)
# Freestanding vent stack at the edge leaves the centre working aisle open.
for row in range(8):box('KilnVent',(1.7,2.4,row*.48+.22),(.7,.7,.45),0)
finish()

begin('SM_DwellingPorch')
for x in [-2.5,2.5]:
    for y in [-2.4,2.4]:post(x,y,2.7)
roof(5.0,4.8,2.85,1.4)
for y in [-2.4,2.4]:
    box('HouseStoneBase',(0,y,.4),(5,.35,1.0),0)
    for x in [-1.75,-1.25,-.75,-.25,.25,.75,1.25,1.75]:
        box('HousePlank',(x,y,1.7),(.47,.15,1.6),1)
for x in [-2.5,2.5]:
    for side in [-1,1]:box('DoorSide',(x,side*1.87,1.15),(.35,1.1,2.5),0)
    box('DoorHeader',(x,0,2.52),(.40,4.8,.28),1)
    # Both doors clear; no floor slab to create a step on natural terrain.
for x in [-2.5,2.5]:
    box('DoorLintelIron',(x-.03,0,2.45),(.44,2.5,.08),2,collision=False)
finish()

begin('SM_AssemblyColonnade')
for x in [-3,0,3]:
    for y in [-3.2,3.2]:
        box('PillarFoot',(x,y,.05),(.85,.85,.7),0)
        box('Pillar',(x,y,2.15),(.55,.55,4.3),0)
        box('Capital',(x,y,4.3),(.85,.85,.32),0)
for y in [-3.2,3.2]:box('HallArchitrave',(0,y,4.55),(7.2,.85,.42),0)
for x in [-3,3]:box('HallCrossbeam',(x,0,4.85),(.38,7,.38),1)
for y in [-3.2,3.2]:box('CouncilBench',(0,y, .52),(2.3,.75,.28),1)
finish()

(ROOT/'zone-mesh-spec.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'meta.json').write_text(json.dumps({**{r['mesh']+'_path':r['mesh']+'.fbx' for r in report},
    'game_id':'Hearthward','run_id':'20261009','task_kind':'3d_object','task_id':'TASK-100-zones'},indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'ZoneArchitecture.blend'))
print(json.dumps(report))
