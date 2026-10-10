"""Blender CPU render of verified full-source hand skin and corrected axe.
Run extraction and validation first; no production assets are modified.
"""
import bpy,json,math
from pathlib import Path
from mathutils import Vector,Matrix
root=Path(__file__).resolve().parents[2];folder=root/'.agent-local/qa/TASK-104/grip'
# Entire corrected source, all original topology, in UE centimetres.
v=[];faces=[]
for line in (root/'art_source/TASK-104/grip/stone_bone_axe_handle_fit.obj').read_text().splitlines():
 if line.startswith('v '):
  p=list(map(float,line.split()[1:]));v.append(Vector((p[0]*100,-p[1]*100,p[2]*100)))
 elif line.startswith('f '):faces.append([int(x.split('/')[0])-1 for x in line.split()[1:]])
data=json.loads((root/'art_source/TASK-104/grip/handle_fit_delta.json').read_text());grip=Vector(data['grip_mesh_cm']);shaft=Vector(data['axis_mesh_unit']).normalized()
ends=json.loads((root/'docs/qa/TASK-055/stone-axe-measured-endpoints.json').read_text())['candidates']
def frame(z,x):
 x=(x-z*x.dot(z)).normalized();y=z.cross(x);return Matrix((x,y,z)).transposed()
axeframe=frame(shaft,(Vector(ends['BladeBase'])+Vector(ends['BladeTip']))*.5-grip)
for role,filename in [('Hero','Hero-full-fit.json'),('Brother','Brother-full-fit.json')]:
 d=json.loads((folder/filename).read_text());rig=json.loads((folder/(role+'-rig.json')).read_text())
 # Same frame as runtime: reference component +Z projected perpendicular to knuckles.
 B=Matrix(rig['bones']['hand_r']['matrix']).to_3x3();D=Matrix.Diagonal(Vector((1,-1,1)));handR=D@B@D
 axis=Vector(d['axis_hand']);palmframe_hand=frame(axis,handR.transposed()@Vector((0,0,1)))
 rot=palmframe_hand@axeframe.transposed();target=Vector(d['palm_hand_local'])*d['scale']
 bpy.ops.wm.read_factory_settings(use_empty=True)
 def meshobj(name,points,polygons,color):
  me=bpy.data.meshes.new(name);me.from_pydata(points,[],polygons);me.update();o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o)
  ma=bpy.data.materials.new(name);ma.diffuse_color=(*color,1);ma.use_nodes=True;ma.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*color,1);ma.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.68;o.data.materials.append(ma)
  for p in me.polygons:p.use_smooth=True
  return o
 meshobj('ActualSourceHand',d['skin'],d['faces'],(.47,.65,.79))
 meshobj('NarrowedAxe',[rot@((p-grip)*.7)+target for p in v],faces,(.36,.19,.08))
 scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=False;scene.render.resolution_x=1000;scene.render.resolution_y=850;scene.render.resolution_percentage=100
 scene.world=bpy.data.worlds.new('World');scene.world.color=(.25,.25,.25)
 for loc,energy,size in [((15,-15,25),2000,20),((-20,-8,10),1500,15),((0,20,20),1500,15)]:
  bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=energy;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(target-o.location).to_track_quat('-Z','Y').to_euler()
 bpy.ops.object.camera_add(location=target+Vector((17,-21,15)));cam=bpy.context.object;cam.rotation_euler=(target-Vector((0,1,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=23;scene.camera=cam
 scene.render.image_settings.file_format='PNG';scene.render.filepath=str(folder/(role+'-grip-render.png'));bpy.ops.render.render(write_still=True)

# Text SVG embeds the actual renders, avoiding dependence on external image paths.
import base64
panels=[]
for i,role in enumerate(('Hero','Brother')):
 encoded=base64.b64encode((folder/(role+'-grip-render.png')).read_bytes()).decode('ascii')
 panels.append('<text x="%d" y="90" font-size="24">%s | actual source-rig geometry</text><image x="%d" y="110" width="1000" height="850" href="data:image/png;base64,%s"/>' % (30+i*1020,role,10+i*1020,encoded))
svg='<svg xmlns="http://www.w3.org/2000/svg" width="2040" height="1020" viewBox="0 0 2040 1020"><rect width="2040" height="1020" fill="white"/><g font-family="sans-serif" fill="#222"><text x="30" y="35" font-size="26">TASK-104 | Narrowed handle + per-rig finger pose</text><text x="30" y="65" font-size="19">OFFLINE Blender CPU renders; untextured geometry, not UE runtime or visual acceptance.</text>'+''.join(panels)+'<text x="30" y="995" font-size="18">Core radius 0.9 cm. Source-skin axis clearance: Hero 1.124 cm; Brother 0.997 cm. Residual fingertip gaps require in-game review.</text></g></svg>'
(root/'docs/qa/TASK-104/grip/hand-grip-both-rigs.svg').write_text(svg,encoding='utf-8')
