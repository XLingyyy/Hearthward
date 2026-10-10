"""Original practical camp facilities. Metres, origin on ground, front -Y.

Run through Blender MCP. Export source descriptors for UEClient import_prop.
"""
from pathlib import Path
import json
import math
import random
import sys
import bpy
from mathutils import Vector

ROOT = Path('G:/GameFactory/Hearthward')
OUT = ROOT / 'art_source/TASK-098'
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
EXPORT = task_output_dir('Hearthward','3d_object','TASK-098-camp-set',run_id='20261009',create=True)
previous = [s for s in bpy.data.scenes if s.name.startswith('TASK098_CampFacilities')]
scene = bpy.data.scenes.new('TASK098_CampFacilities')
bpy.context.window.scene = scene
for old_scene in previous:
    for obj in list(old_scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    bpy.data.scenes.remove(old_scene)
scene.name='TASK098_CampFacilities'
scene.unit_settings.system = 'METRIC'
random.seed(98)
parts = []

def mat(name, color, rough=.9, metal=0):
    m = bpy.data.materials.get('M_Camp'+name) or bpy.data.materials.new('M_Camp'+name)
    m.use_nodes = True
    m.diffuse_color = (*color, 1)
    p = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Roughness'].default_value = rough
    p.inputs['Metallic'].default_value = metal
    return m

stone = mat('Stone', (.27,.255,.225))
wood = mat('Wood', (.18,.095,.042))
iron = mat('Iron', (.055,.06,.062), .72, .72)
linen = mat('Linen', (.55,.49,.36))
rope = mat('Rope', (.29,.22,.12))
coal = mat('Charcoal', (.025,.02,.018))
clay = mat('Clay', (.25,.10,.046))
blanket = mat('Blanket', (.13,.19,.18))

def finish(obj, name, material):
    obj.name = name
    obj.data.materials.append(material)
    parts.append(obj)
    return obj

def box(name, loc, size, material, bevel=.009):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object
    o.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        b = o.modifiers.new('Worn edges', 'BEVEL'); b.width = bevel; b.segments = 2
        bpy.ops.object.modifier_apply(modifier=b.name)
    return finish(o,name,material)

def rod(name, a, b, radius, material, sides=10):
    v = Vector(b)-Vector(a)
    bpy.ops.mesh.primitive_cylinder_add(vertices=sides, radius=radius, depth=v.length,
                                      location=(Vector(a)+Vector(b))*.5)
    o = bpy.context.object
    o.rotation_euler = v.to_track_quat('Z','Y').to_euler()
    return finish(o,name,material)

def ring(name, center, radius, tube, material):
    bpy.ops.mesh.primitive_torus_add(major_segments=32, minor_segments=6,
                                   location=center, major_radius=radius, minor_radius=tube)
    return finish(bpy.context.object,name,material)

def lathe(name, center, profile, material, segments=32):
    verts = [(center[0]+r*math.cos(i*2*math.pi/segments),
              center[1]+r*math.sin(i*2*math.pi/segments),center[2]+z)
             for r,z in profile for i in range(segments)]
    faces=[]
    for j in range(len(profile)-1):
        for i in range(segments):
            a=j*segments+i; b=j*segments+(i+1)%segments
            faces.append((a,b,b+segments,a+segments))
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o)
    return finish(o,name,material)

def rock(name, loc, size, material=stone):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=loc)
    o=bpy.context.object
    for v in o.data.vertices:
        v.co *= random.uniform(.87,1.10)
    o.scale=size
    return finish(o,name,material)

def hearth(radius=.36):
    for i in range(12):
        a=i*2*math.pi/12
        o=rock('Fieldstone', (radius*math.cos(a),radius*math.sin(a),.092),(.12,.105,.085))
        o.rotation_euler.z=a
    for i in range(6):
        a=i*math.pi/3
        rod('Charred split log',(-.23*math.cos(a),-.23*math.sin(a),.11),
            (.23*math.cos(a),.23*math.sin(a),.14),.048,wood)
    for i in range(12):
        rock('Ash charcoal',(random.uniform(-.17,.17),random.uniform(-.17,.17),.035),(.05,.04,.025),coal)

def cloth_surface(name, x0,x1,y0,y1,z,material,fold=.009):
    nx,ny=24,16
    verts=[]
    for j in range(ny+1):
        for i in range(nx+1):
            x=x0+(x1-x0)*i/nx; y=y0+(y1-y0)*j/ny
            dz=fold*math.sin(i*.9+j*.31)+fold*.4*math.sin(j*1.7)
            verts.append((x,y,z+dz))
    faces=[(j*(nx+1)+i,j*(nx+1)+i+1,(j+1)*(nx+1)+i+1,(j+1)*(nx+1)+i)
           for j in range(ny) for i in range(nx)]
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o)
    finish(o,name,material)
    bpy.context.view_layer.objects.active=o
    s=o.modifiers.new('Woven thickness','SOLIDIFY');s.thickness=.006
    bpy.ops.object.modifier_apply(modifier=s.name)
    return o

def bed(medical=False):
    for x in [-.91,.91]:
        for y in [-.43,.43]:
            box('Pegged leg',(x,y,.155),(.095,.095,.31),wood)
        box('End rail',(x,0,.26),(.08,.88,.085),wood)
    for y in [-.43,.43]:
        box('Side rail',(0,y,.26),(1.9,.08,.085),wood)
    for i in range(24):
        x=-.84+i*1.68/23
        rod('Cross lacing',(x,-.44,.266),(x,.44,.266),.008,rope,6)
    for i in range(12):
        y=-.38+i*.76/11
        rod('Long lacing',(-.92,y,.261),(.92,y,.261),.007,rope,6)
    cloth_surface('Straw mattress ticking',-.86,.85,-.39,.39,.288,linen,.004)
    cloth_surface('Folded blanket',-.15,.84,-.38,.38,.318,linen if medical else blanket,.01)
    box('Rolled headrest',(-.64,0,.32),(.28,.63,.08),linen,.035)
    for y in [-.33,.33]:
        rod('Pillow binding',(-.76,y,.359),(-.52,y,.359),.004,rope,6)
    if medical:
        box('Medicine tray',(.49,.22,.355),(.39,.26,.025),wood,.005)
        for x,y in [(.38,.19),(.57,.24)]:
            lathe('Herb jar',(x,y,.371),[(0,0),(.041,0),(.045,.065),(.026,.084),(.026,.095),(.02,.095),(.02,.079)],clay,16)
            rod('Jar cork',(x,y,.458),(x,y,.473),.021,wood)
        for x in [.39,.47,.55]:
            rod('Bandage roll',(x,.32,.384),(x,.39,.384),.017,linen,12)

def smelter():
    # Hollow stacked masonry; the front tuyere remains visibly open.
    for course in range(7):
        z=.09+course*.143
        rad=.34-course*.014
        for i in range(12):
            a=2*math.pi*(i+(course%2)*.5)/12
            if course<3 and math.sin(a)<-.72: continue
            o=box('Kiln stone',(rad*math.cos(a),rad*math.sin(a),z),(.184,.16,.137),stone,.012)
            o.rotation_euler.z=a+math.pi/2
    lathe('Sooted inner flue',(0,0,0),[(.18,.35),(.18,1.08),(.155,1.08),(.155,.35)],coal)
    box('Hearth slab',(0,0,.025),(.85,.87,.05),stone)
    for x in [-.15,0,.15]:
        rock('Slag',(x,-.28,.067),(.07,.07,.037),coal)
    rod('Tuyere',(.31,0,.20),(.48,0,.20),.047,iron)
    box('Bellows boards',(.36,.24,.16),(.19,.32,.06),wood)
    box('Bellows leather',(.36,.24,.12),(.16,.26,.04),rope)
    box('Raking tool',(-.4,.15,.58),(.018,.027,1.12),iron,.004)

def cooking():
    hearth(.32)
    for x,y in [(-.36,-.20),(.36,-.20),(0,.37)]:
        rod('Tripod', (x,y,.025),(0,0,1.16),.022,iron)
    rod('Suspension hook',(0,0,.69),(0,0,1.13),.009,iron)
    pot=lathe('Open cauldron',(0,0,.27),[(0,0),(.12,0),(.205,.06),(.22,.21),(.205,.27),(.19,.27),(.202,.20),(.185,.07),(.10,.025),(0,.025)],iron)
    ring('Rolled lip',(0,0,.54),.20,.012,iron)
    # Handle loop in a vertical plane, visibly attached on both sides.
    handle=ring('Pot bail',(0,0,.54),.185,.009,iron)
    handle.rotation_euler.x=math.pi/2
    for x in [-.2,.2]:
        rod('Handle lug',(x,-.02,.53),(x,.02,.53),.019,iron)
    rod('Wooden spoon',(.22,.22,.24),(.37,.28,.55),.012,wood)
    rock('Spoon bowl',(.37,.28,.55),(.032,.022,.06),wood)

specs=[]
for index,(label,kind,build) in enumerate([
    ('Campfire','campfire',hearth),('RopeBed','bed',bed),('Smelter','smelter',smelter),
    ('Cooking','cooking',cooking),('MedicalBed','medical_area',lambda:bed(True))]):
    parts.clear();build()
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join();o=bpy.context.object;o.name='SM_'+label
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    # Consistent UVs for UE tiled stone/wood materials and cloth detail.
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012)
    bpy.ops.object.mode_set(mode='OBJECT')
    o.data.calc_loop_triangles()
    bpy.ops.export_scene.fbx(filepath=str(EXPORT/(o.name+'.fbx')),use_selection=True,
        object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',
        bake_anim=False,add_leaf_bones=False)
    specs.append(dict(mesh=o.name,kind=kind,dimensions_m=list(o.dimensions),triangles=len(o.data.loop_triangles),
        materials=[m.name for m in o.data.materials],front='-Y',source='Original authored Blender geometry; no third-party input'))
    o.location=(index*2.8,0,0)

meta=dict(game_id='Hearthward',run_id='20261009',task_kind='3d_object',task_id='TASK-098-camp-set')
meta.update({s['mesh']+'_path':s['mesh']+'.fbx' for s in specs})
(EXPORT/'meta.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
(OUT/'camp-facilities.json').write_text(json.dumps(specs,indent=2),encoding='utf-8')
bpy.data.libraries.write(str(OUT/'CampFacilities.blend'),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
print(json.dumps(specs))
