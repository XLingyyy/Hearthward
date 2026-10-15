"""Original Hearthward timber/iron device kit. Run through the live Blender MCP."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector

OUT=Path('G:/GameFactory/Hearthward/art_source/TASK-118')
OUT.mkdir(parents=True,exist_ok=True)
scene=bpy.context.scene
collection=bpy.data.collections.new('Hearthward_Round2_Devices')
scene.collection.children.link(collection)
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
def material(name,color,metallic=0):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    shader=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');shader.inputs['Base Color'].default_value=(*color,1)
    shader.inputs['Roughness'].default_value=.72;shader.inputs['Metallic'].default_value=metallic
    return m
wood=material('OldTimber',(.19,.105,.048));iron=material('ForgedIron',(.07,.08,.085),.8)
rope=material('Hemp',(.33,.25,.14));bronze=material('AgedBronze',(.26,.16,.065),.65)
paper=material('LinenRecord',(.52,.43,.27))
parts=[]
def place(o,name,mat):
    o.name=name;o.data.materials.append(mat)
    for c in list(o.users_collection):c.objects.unlink(o)
    collection.objects.link(o);parts.append(o)
    bpy.context.view_layer.objects.active=o
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    mod=o.modifiers.new('Worn edges','BEVEL');mod.width=.007;mod.segments=2
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return o
def box(name,at,size,mat=wood):
    bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.dimensions=size
    return place(o,name,mat)
def cylinder(name,a,b,r,mat=iron,segments=16):
    a,b=Vector(a),Vector(b);axis=b-a
    bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=axis.length,location=(a+b)/2)
    o=bpy.context.object;o.rotation_euler=axis.to_track_quat('Z','Y').to_euler()
    return place(o,name,mat)
manifest=[]
def finish(name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    o.data.calc_loop_triangles()
    manifest.append({'name':name,'dimensions_m':list(o.dimensions),'triangles':len(o.data.loop_triangles),'source':'original authored geometry','forward':'+X','up':'+Z'})
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False)
    o.location.x=len(manifest)*4;parts.clear()

for y in [-.35,.35]:
    box('upright',(0,y,.55),(.16,.16,1.1));box('foot',(0,y,.07),(.8,.22,.14))
box('cross brace',(0,0,.18),(.13,.8,.13))
cylinder('drum',(0,-.38,.85),(0,.38,.85),.19,wood)
for y in [-.25,-.18,-.11,-.04,.03,.10,.17,.24]:
    bpy.ops.mesh.primitive_torus_add(major_radius=.193,minor_radius=.016,major_segments=20,minor_segments=6,location=(0,y,.85),rotation=(math.pi/2,0,0));place(bpy.context.object,'rope winding',rope)
cylinder('axle',(0,-.51,.85),(0,.51,.85),.045)
box('crank',(0,.53,.65),(.075,.08,.45),iron);cylinder('hand grip',(0,.54,.43),(0,.76,.43),.045,wood)
finish('SM_Round2_Winch')

for y in [-.48,.48]:box('stile',(0,y,1.1),(.09,.10,2.2))
for z in [.18,.48,.78,1.08,1.38,1.68,1.98]:cylinder('rung',(0,-.5,z),(0,.5,z),.037,wood)
finish('SM_Round2_Ladder')
box('beam',(0,0,.16),(.3,3.2,.32))
for y in [-1.25,1.25]:box('iron band',(0,y,.16),(.32,.12,.34),iron)
finish('SM_Round2_Beam')
for y in [-.5,.5]:box('bracket',(0,y,.18),(.15,.15,.35),iron)
box('sliding bar',(.12,0,.18),(.13,1.4,.14),wood);cylinder('handle',(.20,-.10,.18),(.35,-.10,.18),.04)
finish('SM_Round2_Latch')
box('post',(0,0,.85),(.12,.12,1.7));box('arm',(.25,0,1.65),(.6,.09,.1))
for y in [-.15,.15]:cylinder('suspension',(.4,y,1.65),(.4,y,1.43),.009,iron,8)
box('resonant plate',(.4,0,1.15),(.035,.48,.62),bronze)
cylinder('striker',(.08,0,.65),(.08,0,1.05),.035,wood)
finish('SM_Round2_NoisePlate')
cylinder('base',(0,0,0),(0,0,.12),.14,bronze)
cylinder('stem',(0,0,.12),(0,0,.35),.035,iron)
cylinder('oil basin',(0,0,.35),(0,0,.44),.13,bronze)
for y in [-.14,.14]:cylinder('guard',(0,y,.4),(0,y,.7),.013,iron,8)
cylinder('hood',(0,0,.7),(0,0,.74),.18,bronze)
finish('SM_Round2_Lamp')
box('case',(0,0,.04),(.6,.42,.08),wood);box('tools bed',(0,0,.085),(.5,.32,.03),rope)
cylinder('hammer handle',(-.2,0,.14),(.2,0,.14),.025,wood);box('hammer head',(.17,0,.16),(.10,.18,.08),iron)
finish('SM_Round2_ToolsMemento')
box('record board',(0,0,.025),(.44,.34,.05),wood);box('folded record',(0,0,.06),(.38,.29,.02),paper)
for x in [-.12,0,.12]:cylinder('dried herb',(x,-.1,.09),(x+.06,.1,.10),.013,rope,8)
finish('SM_Round2_HerbRecord')
box('stand',(0,0,.025),(.46,.35,.05),wood);cylinder('signal horn',(-.16,0,.12),(.16,0,.18),.085,bronze)
for x in [-.14,.14]:box('horn rest',(x,0,.09),(.04,.15,.16),wood)
finish('SM_Round2_WatchMemento')

(OUT/'device_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Round2_Devices.blend'))
print(json.dumps(manifest))
