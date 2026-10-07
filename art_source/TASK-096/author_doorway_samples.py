"""Approved grey stone/old wood material and lighting sample, plus workshop composition."""
import bpy,math,json,random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];QA=ROOT/'docs/qa/TASK-096/samples';QA.mkdir(parents=True,exist_ok=True);random.seed(96)
bpy.ops.wm.read_factory_settings(use_empty=True)
def material(name,color,roughness,metal=0,grain=False):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=roughness;p.inputs['Metallic'].default_value=metal
 if grain:
  noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=16 if metal else 7;noise.inputs['Detail'].default_value=3;bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.28;bump.inputs['Distance'].default_value=.04;m.node_tree.links.new(noise.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs['Normal'],p.inputs['Normal'])
 return m
stone=[material('M_Stone_'+str(i),(.20+i*.012,.205+i*.012,.21+i*.012),.93,grain=True) for i in range(4)];wood=material('M_OldWood',(.115,.065,.03),.91,grain=True);iron=material('M_MatteIron',(.045,.048,.05),.78,.65,True);floor=material('M_CourtyardStone',(.13,.135,.14),.95,grain=True);flagmat=material('M_WorkshopFlag',(.21,.055,.027),.95)
def box(name,loc,size,mat,bevel=.02):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 if bevel:
  m=o.modifiers.new('WornEdges','BEVEL');m.width=bevel;m.segments=2;bpy.ops.object.modifier_apply(modifier=m.name)
 return o
box('Floor',(0,1,-.1),(8,9,.2),floor)
for row in range(8):
 for side in [-1,1]:
  cursor=1.005
  widths=[.63,.63,.63] if row%2==0 else [.315,.63,.63,.315]
  for col,width in enumerate(widths):
   x=side*(cursor+width*.5);box('DoorwayStone',(x,0,.20+row*.4),(width-.025,.6,.375),stone[(row+col)%4]);cursor+=width
for col in range(4):box('Lintel',(-.75+col*.5,0,2.62),(.485,.65,.44),stone[col%4])
for col in range(6):box('Crown',(-1.50+col*.6,0,3.06),(.58,.61,.38),stone[col%4])
for side in [-1,1]:
 # Door leaves are parked against the inside wall, preserving the two-metre opening.
 for i in range(5):box('DoorPlank',(side*1.43, .25+i*.20,1.18),(.09,.19,2.30),wood,.012)
 for z in [.4,1.95]:box('DoorBand',(side*1.49,.66,z),(.035,1.05,.06),iron,.006)
# A caged lamp marks the entrance without occupying the route.
box('LampMount',(-1.60,-.42,1.65),(.12,.20,.10),iron)
for x in [-1.70,-1.50]:
 for y in [-.56,-.38]:box('LanternFrame',(x,y,1.91),(.018,.018,.38),iron,.003)
box('LanternRoof',(-1.60,-.47,2.12),(.28,.25,.055),iron,.008)
bpy.ops.object.light_add(type='POINT',location=(-1.60,-.47,1.9));lamp=bpy.context.object;lamp.name='WarmLantern';lamp.data.color=(1,.57,.25);lamp.data.energy=85;lamp.data.shadow_soft_size=.08
bpy.ops.object.light_add(type='AREA',location=(0,3,2.8));inside=bpy.context.object;inside.name='InteriorBounce';inside.data.energy=70;inside.data.color=(1,.72,.46);inside.data.size=3
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('ReviewSky');scene.world.use_nodes=True;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=24;scene.render.resolution_x=1200;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX'
bpy.ops.object.light_add(type='AREA',location=(-3,-4,6));sun=bpy.context.object;sun.name='Daylight';sun.data.energy=1400;sun.data.size=5;sun.rotation_euler=(Vector((0,0,1.3))-sun.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(3.5,-6.5,2.7));camera=bpy.context.object;scene.camera=camera;camera.data.lens=40;camera.rotation_euler=(Vector((0,.2,1.4))-camera.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(Path(__file__).parent/'Doorway.blend'))
for name,energy,ambient in [('Day',1400,.25),('Night',95,.06)]:
 sun.data.energy=energy;sun.data.color=(1,1,1) if name=='Day' else (.50,.64,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=ambient;scene.render.filepath=str(QA/('Doorway-'+name+'.png'));bpy.ops.render.render(write_still=True)
# Reuse the approved facilities for the workshop entry-to-flag composition.
for role,loc in [('Workbench',(-2.3,2.2,0)),('Forge',(2.2,1.8,0)),('Warehouse',(2.4,3.1,0))]:
 with bpy.data.libraries.load(str(ROOT/'art_source/TASK-098'/(role+'.blend')),link=False) as (available,loaded):loaded.objects=[n for n in available.objects if n=='SM_'+role+'_Practical']
 for obj in loaded.objects:
  bpy.context.collection.objects.link(obj);obj.location=loc
box('Flagpole',(0,3.7,1.5),(.055,.055,3),wood,.003)
box('WorkshopFlag',(.35,3.7,2.45),(.65,.018,.7),flagmat,.003)
sun.data.energy=1100;sun.data.color=(1,1,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.20;camera.location=(4.3,-6,4.5);camera.rotation_euler=(Vector((0,1.8,.7))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.lens=32
out100=ROOT/'art_source/TASK-100';out100.mkdir(parents=True,exist_ok=True);qa100=ROOT/'docs/qa/TASK-100/samples';qa100.mkdir(parents=True,exist_ok=True);bpy.ops.wm.save_as_mainfile(filepath=str(out100/'WorkshopEntry.blend'));scene.render.filepath=str(qa100/'Workshop-entry.png');bpy.ops.render.render(write_still=True)
camera.location=(5,7,5);camera.rotation_euler=(Vector((0,1.7,.8))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.lens=32;scene.render.filepath=str(qa100/'Workshop-inside.png');bpy.ops.render.render(write_still=True)
(QA/'source-results.json').write_text(json.dumps({'method':'Original source material/lighting sample; procedural normals and roughness; not UE runtime','door_opening_m':2.03,'day_night_same_geometry':True,'nearfield_ue_pbr_bake':'NOT_RUN','fire_corner':'NOT_RUN','owner_visual':'NOT_RUN'},indent=2),encoding='utf-8')
(qa100/'source-results.json').write_text(json.dumps({'method':'Source-only workshop entry-to-flag composition reusing TASK-098 first pieces','entry_opening_m':2.03,'central_walkway_clear_width_m':3.0,'actual_level_placement':'NOT_RUN','navigation_patrol_combat':'NOT_RUN','owner_visual':'NOT_RUN'},indent=2),encoding='utf-8')
print('DOORWAY_AND_WORKSHOP_SAMPLES_COMPLETE')
