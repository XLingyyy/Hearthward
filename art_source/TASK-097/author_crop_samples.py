"""Original crop meshes with distinct vegetative and mature silhouettes."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir,write_task_meta
OUT=Path(__file__).parent;QA=ROOT/'docs/qa/TASK-097/samples';QA.mkdir(parents=True,exist_ok=True)
records=[]
def material(name,color):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;s=m.node_tree.nodes.get('Principled BSDF');s.inputs['Base Color'].default_value=(*color,1);s.inputs['Roughness'].default_value=.92;return m
for kind in ['Greens','Grain','Herb']:
 for mature in [False,True]:
  bpy.ops.wm.read_factory_settings(use_empty=True);parts=[];green=material('M_Leaf',(.09,.22,.035));stem=material('M_Stem',(.16,.23,.04));ripe=material('M_Ripe',(.45,.27,.065) if kind=='Grain' else (.60,.56,.37))
  def rod(start,end,radius,mat):
   delta=Vector(end)-Vector(start);bpy.ops.mesh.primitive_cylinder_add(vertices=6,radius=radius,depth=delta.length,location=(Vector(start)+Vector(end))/2);o=bpy.context.object;o.rotation_euler=delta.to_track_quat('Z','Y').to_euler();o.data.materials.append(mat);parts.append(o)
  def leaf(start,angle,length,width,lift,mat):
   base=Vector(start);direction=Vector((math.cos(angle),math.sin(angle),0));side=Vector((-math.sin(angle),math.cos(angle),0));vs=[];faces=[]
   for i in range(13):
    t=i/12;half=width*.5*math.sin(math.pi*t)**.75
    center=base+direction*(length*t)+Vector((0,0,length*(lift*t+.18*math.sin(math.pi*t))))
    for j in range(5):
     across=(j-2)/2;vs.append(center+side*(half*across)+Vector((0,0,-abs(across)*half*.12+math.sin(t*8+angle)*half*.06)))
   for i in range(12):
    for j in range(4):k=i*5+j;faces.append((k,k+1,k+6,k+5))
   data=bpy.data.meshes.new('Leaf');data.from_pydata(vs,[],faces);o=bpy.data.objects.new('Leaf',data);bpy.context.collection.objects.link(o);o.data.materials.append(mat)
   for face in data.polygons:face.use_smooth=True
   solid=o.modifiers.new('LeafThickness','SOLIDIFY');solid.thickness=.0006;parts.append(o)
   for i in range(0,12,3):rod(vs[i*5+2],vs[(i+3)*5+2],.0007,stem)

  if kind=='Greens':
   for i in range(9 if mature else 5):leaf((0,0,.01),i*2.4,.24 if mature else .11,.12 if mature else .055,.25+(i%3)*.15,green)
  elif kind=='Grain':
   for i in range(7 if mature else 4):
    angle=i*2.4;x=.06*math.cos(angle);y=.06*math.sin(angle);h=(.72+.06*(i%3)) if mature else (.23+.025*i);rod((x,y,0),(x+.035,y,h),.0035,ripe if mature else stem)
    for layer in range(3):leaf((x,y,h*(.2+layer*.2)),angle+layer*2,.22 if mature else .12,.015,.12,green)
    if mature:
     for j in range(7):
      for sign in [-1,1]:
       z=h-.08+j*.015;tip=(x+.035+sign*.025,y,z+.025);rod((x+.035,y,z),tip,.0065,ripe);rod(tip,(tip[0]+sign*.017,y,tip[2]+.05),.0008,ripe)
  else:
   h=.34 if mature else .15
   for i in range(3):
    angle=i*2.1;x=.03*math.cos(angle);y=.03*math.sin(angle);rod((x,y,0),(x,y,h),.004,stem)
    for j in range(3):
     for sign in [0,1]:leaf((x,y,h*(.2+j*.22)),angle+sign*math.pi,.09 if mature else .045,.04 if mature else .025,.22,green)
    if mature:
     for j in range(5):leaf((x,y,h),j*math.tau/5,.022,.012,.12,ripe)
  bpy.ops.object.select_all(action='DESELECT')
  for o in parts:o.select_set(True)
  bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.convert(target='MESH');bpy.ops.object.join();mesh=bpy.context.object;stage='Mature' if mature else 'Young';mesh.name=f'SM_{kind}_{stage}';bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
  desc={'game_id':'Hearthward','task_kind':'3d_object','task_id':f'TASK-097-{kind.lower()}-{stage.lower()}','run_id':'approved-20261007','artifact_key':'model_path'};folder=task_output_dir(desc['game_id'],desc['task_kind'],desc['task_id'],run_id=desc['run_id']);folder.mkdir(parents=True,exist_ok=True);fbx=folder/(mesh.name+'.fbx');bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',bake_anim=False);write_task_meta(folder,{**desc,'model_path':str(fbx),'source_route':'original botanical mesh construction','units':'metres'})
  bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(kind+'-'+stage+'.blend')))
  records.append({'kind':kind,'stage':stage,'descriptor':desc,'dimensions_m':list(mesh.dimensions),'ue_import':'NOT_RUN'})
  scene=bpy.context.scene;scene.world=bpy.data.worlds.new('CropReviewWorld');scene.world.color=(.18,.18,.18);scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=12;scene.render.resolution_x=800;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX'
  h=max(mesh.dimensions.z,.25)
  for pos,power in [((2,-3,4),350),((-2,2,3),250)]:
   bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.size=3;o.rotation_euler=(Vector((0,0,h*.4))-o.location).to_track_quat('-Z','Y').to_euler()
  bpy.ops.object.camera_add(location=(1,-2,1.1));camera=bpy.context.object;scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=max(h,mesh.dimensions.x,mesh.dimensions.y)*1.35;camera.rotation_euler=(Vector((0,0,h*.4))-camera.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(QA/(kind+'-'+stage+'.png'));bpy.ops.render.render(write_still=True)
(QA/'crop-source.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8');print('CROP_SAMPLES_COMPLETE',len(records))
