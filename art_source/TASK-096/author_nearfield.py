"""Original nearfield masonry module and periodic stone/timber surface maps."""
import bpy,math,json,sys
import numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
out=task_output_dir('Hearthward','3d_object','TASK-096-nearfield','nearfield-20261008');out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
rng=np.random.default_rng(96);size=1024
def noise(cutx,cuty):
 a=rng.normal(size=(size,size));fx=np.fft.fftfreq(size)[None,:]*size;fy=np.fft.fftfreq(size)[:,None]*size
 a=np.fft.ifft2(np.fft.fft2(a)*np.exp(-.5*((fx/cutx)**2+(fy/cuty)**2))).real
 return (a-a.mean())/a.std()
def save_image(name,values,normal=False):
 image=bpy.data.images.new(name,width=size,height=size,alpha=True)
 image.colorspace_settings.name='Non-Color' if normal else 'sRGB'
 rgba=np.ones((size,size,4),dtype=np.float32);rgba[:,:,:3]=np.clip(values,0,1)
 image.pixels.foreach_set(rgba.ravel());image.filepath_raw=str(out/(name+'.png'));image.file_format='PNG';image.save();image.pack();return image
def material(label,height,color,variation,strength):
 diffuse=save_image('T_'+label+'_D',np.array(color)[None,None,:]*(1+height[:,:,None]*variation))
 dx=(np.roll(height,-1,1)-np.roll(height,1,1))*strength;dy=(np.roll(height,-1,0)-np.roll(height,1,0))*strength
 normal=np.stack((-dx,-dy,np.ones_like(dx)),axis=-1);normal/=np.linalg.norm(normal,axis=-1)[:,:,None]
 normalmap=save_image('T_'+label+'_N',normal*.5+.5,True)
 mat=bpy.data.materials.new('M_'+label);mat.use_nodes=True;mat.diffuse_color=(*color,1)
 nodes=mat.node_tree.nodes;links=mat.node_tree.links;p=next(n for n in nodes if n.type=='BSDF_PRINCIPLED');p.inputs['Roughness'].default_value=.9
 c=nodes.new('ShaderNodeTexImage');c.image=diffuse;links.new(c.outputs['Color'],p.inputs['Base Color'])
 n=nodes.new('ShaderNodeTexImage');n.image=normalmap;conv=nodes.new('ShaderNodeNormalMap');links.new(n.outputs['Color'],conv.inputs['Color']);links.new(conv.outputs['Normal'],p.inputs['Normal'])
 return mat
stoneheight=.6*noise(9,9)+.25*noise(50,50)+.12*noise(180,180)
woodheight=.45*noise(55,2)+.23*noise(200,4)+.12*noise(9,2)
stone=material('RoughStone',stoneheight,(.42,.43,.44),.16,1.1)
wood=material('OldTimber',woodheight,(.32,.215,.125),.24,.65)
objects=[]
def block(name,loc,dimensions):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=dimensions
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 bevel=o.modifiers.new('Worn arris','BEVEL');bevel.width=.012;bevel.segments=2;bpy.ops.object.modifier_apply(modifier=bevel.name)
 o.data.materials.append(stone)
 uv=o.data.uv_layers.active
 for poly in o.data.polygons:
  axis=max(range(3),key=lambda i:abs(poly.normal[i]));axes=[i for i in range(3) if i!=axis]
  for loop in poly.loop_indices:
   v=o.data.vertices[o.data.loops[loop].vertex_index].co+o.location;uv.data[loop].uv=(v[axes[0]]/.8,v[axes[1]]/.8)
 objects.append(o)
for sign in [-1,1]:
 for row in range(8):
  width=.48 if row%2==0 else .60
  block('Jamb', (sign*(1.4+width/2),0,(row+.5)*.475),(width,1.10,.459))
for i in range(7):block('Lintel',(-1.2+i*.4,0,4.02),(.389,1.12,.44))
for sign in [-1,1]:block('Capital',(sign*1.71,0,4.02),(.62,1.12,.44))
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();frame=bpy.context.object;frame.name='SM_BedroomDoorframe'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
frame.data.calc_loop_triangles();triangles=len(frame.data.loop_triangles)
assert triangles<10000
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
bpy.ops.export_scene.fbx(filepath=str(out/(frame.name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_scale_options='FBX_SCALE_ALL',bake_anim=False)
scene.world=bpy.data.worlds.new('ReviewSky');scene.world.use_nodes=True
background=next(n for n in scene.world.node_tree.nodes if n.type=='BACKGROUND');background.inputs['Color'].default_value=(.45,.53,.66,1);background.inputs['Strength'].default_value=.35
bpy.ops.object.light_add(type='AREA',location=(0,-4,6));lamp=bpy.context.object;lamp.data.energy=1800;lamp.data.size=5;lamp.rotation_euler=(Vector((0,0,2))-lamp.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(6,-9,5));camera=bpy.context.object;camera.data.lens=48;camera.rotation_euler=(Vector((0,0,2))-camera.location).to_track_quat('-Z','Y').to_euler();scene.camera=camera
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1000;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-096/Nearfield.blend'))
report={'source':'Original procedural construction and periodic noise maps; no downloaded imagery','clear_width_cm':280,'clear_height_cm':380,'triangles':triangles,'size_m':list(frame.dimensions),'texture_size':[size,size],'collision':'none; existing native doorway retained','normal_convention':'OpenGL; flip green when importing UE'}
(out/'author.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report))
