"""Original additive facility upgrades, metres; retain existing footprint and transactions."""
import bpy, math, json, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path('G:/GameFactory/Hearthward')
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
OUT=task_output_dir('Hearthward','3d_object','TASK-098-upgrades',run_id='20261009',create=True)
scene=bpy.data.scenes.new('TASK098_Upgrades');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC'
def material(name,color,metal=0):
    m=bpy.data.materials.new('M_Upgrade'+name);m.use_nodes=True
    p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=.8;p.inputs['Metallic'].default_value=metal
    return m
wood=material('Wood',(.20,.105,.048));iron=material('Iron',(.055,.06,.066),.75);clay=material('Clay',(.24,.105,.047))
parts=[];report=[];meta=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-098-upgrades',run_id='20261009')
def box(loc,size,mat):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    b=o.modifiers.new('Forged or worn edge','BEVEL');b.width=.004;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
    o.data.materials.append(mat);parts.append(o);return o
def rod(a,b,r,mat):
    v=Vector(b)-Vector(a);bpy.ops.mesh.primitive_cylinder_add(vertices=12,radius=r,depth=v.length,location=(Vector(a)+Vector(b))*.5)
    o=bpy.context.object;o.rotation_euler=v.to_track_quat('Z','Y').to_euler();o.data.materials.append(mat);parts.append(o)
def hoop(z,r,width):
    for i in range(32):
        a=i*math.tau/32;b=(i+1)*math.tau/32
        rod((r*math.cos(a),r*math.sin(a),z),(r*math.cos(b),r*math.sin(b),z),width,iron)
def save(name,index):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');o.data.calc_loop_triangles()
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',bake_anim=False,add_leaf_bones=False)
    meta[name+'_path']=name+'.fbx';report.append(dict(name=name,triangles=len(o.data.loop_triangles),dimensions_m=list(o.dimensions)))
    o.location.x=index*2;parts.clear()
for kind in ['Workbench','Smelter','Forge','Cooking']:
    for level in [2,3]:
        if kind=='Workbench':
            if level==2:
                # Lower tool shelf and leg straps.
                for y in [-.23,0,.23]:box((0,y,.29),(1.02,.22,.035),wood)
                for x in [-.5,.5]:
                    for y in [-.3,.3]:box((x,y,.43),(.106,.106,.05),iron)
            else:
                # Peg rack and chisels above the rear work edge.
                for x in [-.51,.51]:box((x,.33,.93),(.045,.045,.48),wood)
                box((0,.33,1.08),(1.1,.055,.16),wood)
                for x in [-.36,-.18,0,.18,.36]:
                    rod((x,.285,.94),(x,.285,1.10),.013,iron);rod((x,.285,1.08),(x,.285,1.17),.022,wood)
        elif kind=='Smelter':
            if level==2:
                hoop(.42,.345,.014);hoop(.79,.299,.014)
                for a in [0,math.pi*.5,math.pi,math.pi*1.5]:rod((.345*math.cos(a),.345*math.sin(a),.42),(.299*math.cos(a),.299*math.sin(a),.79),.013,iron)
            else:
                # Open flue extension; no cap over the smoke outlet.
                for i in range(16):
                    a=i*math.tau/16;o=box((.235*math.cos(a),.235*math.sin(a),1.19),(.10,.095,.28),clay);o.rotation_euler.z=a
                hoop(1.30,.27,.016)
        elif kind=='Forge':
            if level==2:
                for z in [.14,.37]:
                    for y in [-.298,.298]:box((0,y,z),(.67,.024,.055),iron)
                    for x in [-.335,.335]:box((x,0,z),(.024,.59,.055),iron)
            else:
                for x in [-.48,.48]:box((x,.37,.52),(.055,.055,1.04),wood)
                box((0,.37,1.04),(1.04,.065,.09),wood)
                for x in [-.35,-.1,.15,.35]:
                    rod((x,.32,.72),(x,.32,1.04),.015,iron)
                    box((x,.32,.75),(.1,.045,.06),iron)
        else:
            if level==2:
                # Low side trivet within original 108 x 112 cm footprint.
                for x in [-.4,.4]:rod((x,.39,.02),(x,.39,.39),.019,iron)
                for x in [-.4,-.2,0,.2,.4]:rod((x,.25,.39),(x,.49,.39),.012,iron)
                for y in [.25,.49]:rod((-.4,y,.39),(.4,y,.39),.015,iron)
            else:
                for x in [-.32,0,.32]:
                    rod((x,.20,.72),(x,.20,1.03),.014,wood)
                    bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=1,location=(x,.20,.70));o=bpy.context.object;o.scale=(.04,.018,.064);o.data.materials.append(iron);parts.append(o)
                rod((-.36,.20,1.04),(.36,.20,1.04),.022,iron)
        save('SM_'+kind+'_L'+str(level),len(report))
# A separate brazier beside the existing anvil, wholly inside its original footprint.
for x in [-.18,.18]:
    for y in [.44,.68]:box((x,y,.24),(.035,.035,.48),iron)
box((0,.56,.49),(.47,.29,.055),iron)
for x in [-.24,.24]:box((x,.56,.56),(.025,.32,.14),iron)
for y in [.40,.72]:box((0,y,.56),(.49,.025,.14),iron)
for x in [-.15,-.05,.05,.15]:box((x,.56,.53),(.09,.22,.07),clay)
save('SM_ForgeHearth',len(report))
(OUT/'meta.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
(ROOT/'art_source/TASK-098/facility-upgrades.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
bpy.data.libraries.write(str(ROOT/'art_source/TASK-098/FacilityUpgrades.blend'),{scene},path_remap='RELATIVE',fake_user=True,compress=True)
print(json.dumps(report))
