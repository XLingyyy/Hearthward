"""Practical costume samples on the existing imported skeletons; no animation changes."""
import bpy,json,math,shutil
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];OUT=Path(__file__).parent;QA=ROOT/'docs/qa/TASK-095/samples';QA.mkdir(parents=True,exist_ok=True)
reports=[]
roles=[('Hero',ROOT.parent/'test_data/outputs/hearthward/20260924_hero/assets/3d_object/hero/SK_Hero.fbx'),('Brother',ROOT.parent/'test_data/outputs/hearthward/20260924_brother/assets/3d_object/brother/SK_Brother.fbx'),('Guard',ROOT/'art_source/TASK-004/Tripo/敌人/outputs/short_blade_soldier/short_blade_soldier_rigged.fbx'),('Archer',ROOT/'art_source/TASK-004/Tripo/敌人/outputs/short_blade_soldier/short_blade_soldier_rigged.fbx'),('Heavy',ROOT/'art_source/TASK-004/Tripo/敌人/outputs/heavy_armored_soldier/heavy_armored_soldier_rigged.fbx')]
def mat(name,color,metal=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=.88;p.inputs['Metallic'].default_value=metal;return m
for role,source in roles:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 cache=OUT/'source-cache'/role;cache.mkdir(parents=True,exist_ok=True);copy=cache/source.name
 if not copy.exists():shutil.copyfile(source,copy)
 bpy.ops.import_scene.fbx(filepath=str(copy),use_anim=False)
 rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE'];assert len(rigs)==1;rig=rigs[0];rig.data.pose_position='REST'
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];body=max(meshes,key=lambda o:len(o.data.vertices))
 points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices];low=min(v.z for v in points);high=max(v.z for v in points);height=high-low
 bone_before=[(b.name,list(b.head_local),list(b.tail_local)) for b in rig.data.bones]
 if role in ('Hero','Brother'):
  texture=ROOT.parent/f'test_data/outputs/hearthward/20260924_{role.lower()}/assets/3d_object/{role.lower()}_basecolor/T_{role}_basecolor.JPEG'
  image=bpy.data.images.load(str(texture));image.pack()
  original=mat('M_'+role+'_Original',(.3,.2,.12));nodes=original.node_tree.nodes;tex=nodes.new('ShaderNodeTexImage');tex.image=image;original.node_tree.links.new(tex.outputs['Color'],nodes.get('Principled BSDF').inputs['Base Color'])
  cloth=original.copy();cloth.name='M_'+role+'_CoarseCloth';shader=cloth.node_tree.nodes.get('Principled BSDF');input=shader.inputs['Base Color'];output=input.links[0].from_socket;cloth.node_tree.links.remove(input.links[0]);mix=cloth.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MIX';mix.inputs[0].default_value=.32;mix.inputs[2].default_value=(.23,.15,.075,1) if role=='Hero' else (.13,.18,.14,1);cloth.node_tree.links.new(output,mix.inputs[1]);cloth.node_tree.links.new(mix.outputs[0],input)
  body.data.materials.clear();body.data.materials.append(cloth)
  attr=body.data.color_attributes.new(name='ClothBlend',type='FLOAT_COLOR',domain='CORNER')
  for poly in body.data.polygons:
   poly.material_index=0
   for loop in poly.loop_indices:
    center=body.matrix_world@body.data.vertices[body.data.loops[loop].vertex_index].co;z=(center.z-low)/height
    lower=max(0,min(1,(z-.38)/.08));upper=max(0,min(1,(.84-z)/.09));mask=lower*upper*.25
    if z<.62:mask*=max(0,min(1,(.16*height-abs(center.x))/(.025*height)))
    attr.data[loop].color=(mask,mask,mask,1)
  color=cloth.node_tree.nodes.new('ShaderNodeVertexColor');color.layer_name='ClothBlend';cloth.node_tree.links.new(color.outputs['Color'],mix.inputs[0])

 else:
  for material in body.data.materials:
   if material and material.use_nodes:
    for n in material.node_tree.nodes:
     if n.type=='BSDF_PRINCIPLED':n.inputs['Roughness'].default_value=.8
     if n.type=='TEX_IMAGE' and n.image:
      try:n.image.pack()
      except RuntimeError:pass
 leather=mat('M_'+role+'_Leather',(.08,.04,.018));iron=mat('M_'+role+'_Iron',(.18,.19,.18),.65)
 center=Vector((0,0,low+.60*height));near=[v for v in points if abs(v.z-center.z)<.035*height];xmin=min(v.x for v in near);xmax=max(v.x for v in near);ymin=min(v.y for v in near);ymax=max(v.y for v in near)
 center.x=(xmin+xmax)/2;center.y=(ymin+ymax)/2
 vertices=[];faces=[]
 for z in [-.018,.018]:
  for i in range(32):
   angle=math.tau*i/32;vertices.append((center.x+(xmax-xmin)*.54*math.cos(angle),center.y+(ymax-ymin)*.56*math.sin(angle),center.z+z*height))
 for i in range(32):faces.append((i,(i+1)%32,(i+1)%32+32,i+32))
 data=bpy.data.meshes.new('Belt');data.from_pydata(vertices,[],faces);belt=bpy.data.objects.new('Belt',data);bpy.context.collection.objects.link(belt);belt.data.materials.append(leather)
 bone=min(rig.data.bones,key=lambda b:((rig.matrix_world@b.head_local)-center).length).name
 def bind(obj,bone):
  bpy.context.view_layer.objects.active=obj;bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  obj.vertex_groups.new(name=bone).add(list(range(len(obj.data.vertices))),1,'REPLACE');mod=obj.modifiers.new('ExistingSkeleton','ARMATURE');mod.object=rig
 bpy.data.objects.remove(belt,do_unlink=True)
 if role=='Archer':
  back=Vector((xmin-.025*height,center.y,low+.72*height))
  bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=.055*height,depth=.30*height,location=back);quiver=bpy.context.object;quiver.name='ArcherQuiver';quiver.data.materials.append(leather);bind(quiver,bone)
  for i in range(5):
   loc=back+Vector((.015*height*math.cos(i*1.4),.015*height*math.sin(i*1.4),.14*height));bpy.ops.mesh.primitive_cylinder_add(vertices=6,radius=.003*height,depth=.20*height,location=loc);arrow=bpy.context.object;arrow.data.materials.append(iron);bind(arrow,bone)
 assert bone_before==[(b.name,list(b.head_local),list(b.tail_local)) for b in rig.data.bones]
 bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(role+'.blend')))
 scene=bpy.context.scene;scene.world=bpy.data.worlds.new('ReviewWorld');scene.world.color=(.12,.12,.12);scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16;scene.render.resolution_x=720;scene.render.resolution_y=1000;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX'
 target=Vector((0,0,low+.50*height))
 for loc,power in [((2,-3,4),650),((-2,2,3),450)]:
  bpy.ops.object.light_add(type='AREA',location=Vector(loc)*height);o=bpy.context.object;o.data.energy=power*height*height;o.data.size=3*height;o.rotation_euler=(target-o.location).to_track_quat('-Z','Y').to_euler()
 bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=height*1.2
 for name,loc in [('front',(.5,-3,1.5)),('back',(-.5,3,1.5))] if role in ('Hero','Brother') else [('front',(3,-.5,1.5)),('back',(-3,.5,1.5))]:
  camera.location=Vector(loc)*height;camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(QA/(role+'-'+name+'.png'));bpy.ops.render.render(write_still=True)
 reports.append({'role':role,'source':str(source),'source_height_m':height,'bone_count':len(rig.data.bones),'skeleton_unchanged':True,'ue_import':'NOT_RUN','motion_review':'NOT_RUN','owner_visual':'NOT_RUN','archer_weapon_cleanup':'REQUIRED' if role=='Archer' else 'NOT_APPLICABLE'})
(QA/'source-results.json').write_text(json.dumps(reports,ensure_ascii=False,indent=2),encoding='utf-8');print('CHARACTER_SAMPLES_COMPLETE',len(reports))
