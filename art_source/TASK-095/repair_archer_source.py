"""Rebuild the approved archer on the original 22-bone rig.

Open art_source/TASK-095/Archer.blend first, then run this file through Blender MCP.
The original file is never overwritten. Copied arm UVs/weights are retained;
the shoulder seam is welded and receives a separate coarse-cloth material.
"""
from pathlib import Path
import bpy,bmesh,json,math
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
assert Path(bpy.data.filepath).resolve()==(ROOT/'art_source/TASK-095/Archer.blend').resolve()
assert len(bpy.data.objects['tripo_node_6406d71c'].data.vertices)==46063
original_rig=bpy.data.objects['Armature']
bone_before=[(b.name,list(b.head_local),list(b.tail_local),b.parent.name if b.parent else None) for b in original_rig.data.bones]


import bpy,bmesh,json
from mathutils import Vector
body=bpy.data.objects['tripo_node_6406d71c']
rig=bpy.data.objects['Armature']
groups={g.index:g.name for g in body.vertex_groups}
def arm_weight(v,side):
 return sum(g.weight for g in v.groups if side in groups[g.group])
# Reuse the intact arm, including its UVs and clothing, under the same rig.
arm=body.copy();arm.data=body.data.copy();bpy.context.collection.objects.link(arm);arm.name='ReconstructedBowArm'
keep={v.index for v in body.data.vertices if arm_weight(v,'1_Left_Limb')>.38 and (body.matrix_world@v.co).y<-.07 and (body.matrix_world@v.co).z<.82}
bm=bmesh.new();bm.from_mesh(arm.data);bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.index not in keep],context='VERTS')
bm.to_mesh(arm.data);bm.free()
for v in arm.data.vertices:
 p=body.matrix_world@v.co
 z=p.z
 if z>.676:
  t=min(1,(z-.676)/(.801-.676));delta=Vector((.063,-.016,-.031)).lerp(Vector((.020,-.008,-.023)),t)
 else:
  t=max(0,min(1,(z-.555)/(.676-.555)));delta=Vector((.035,0,-.043)).lerp(Vector((.063,-.016,-.031)),t)
 p.y=-p.y;p+=delta;v.co=body.matrix_world.inverted()@p
 weights={groups[g.group]:g.weight for g in v.groups}
 for g in arm.vertex_groups:g.remove([v.index])
 for name,weight in weights.items():
  if '1_Left_Limb_' in name:name='tripo::0_Right_Limb_'+str(min(2,int(name[-1])))
  arm.vertex_groups[name].add([v.index],weight,'ADD')
bm=bmesh.new();bm.from_mesh(arm.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(arm.data);bm.free()
# Remove only the original shield-arm region; the copied arm overlaps under the cloak.
cut={v.index for v in body.data.vertices if arm_weight(v,'0_Right_Limb')>.45 and (body.matrix_world@v.co).y>.07 and (body.matrix_world@v.co).z<.825}
bm=bmesh.new();bm.from_mesh(body.data);bm.verts.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.index in cut],context='VERTS')
bm.to_mesh(body.data);bm.free()
for obj in [body,arm]:
 for p in obj.data.polygons:p.material_index=0
 # Weapon surfaces form separate components after a narrow cut in front of the fist.
 bm=bmesh.new();bm.from_mesh(obj.data);bm.verts.ensure_lookup_table()
 eligible={v for v in bm.verts if (obj.matrix_world@v.co).x>.022 and .34<(obj.matrix_world@v.co).z<.52}
 components=[]
 while eligible:
  seed=eligible.pop();component={seed};pending=[seed]
  while pending:
   v=pending.pop()
   for edge in v.link_edges:
    other=edge.other_vert(v)
    if other in eligible:eligible.remove(other);component.add(other);pending.append(other)
  components.append(component)
 remove=set()
 for component in components:
  if max((obj.matrix_world@v.co).x for v in component)>.15:remove.update(component)
 bmesh.ops.delete(bm,geom=list(remove),context='VERTS')
 boundary=[e for e in bm.edges if e.is_boundary]
 # Close the small cut at the blade root; shoulder seam is kept for fitting.
 small=[e for e in boundary if all(.34<(obj.matrix_world@v.co).z<.53 for v in e.verts)]
 bmesh.ops.holes_fill(bm,edges=small,sides=0)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(obj.data);bm.free();obj.data.update()
print('REBUILT_ARM',len(arm.data.vertices),'BODY',len(body.data.vertices))


import bpy,bmesh,json
from mathutils import Vector
body=bpy.data.objects['tripo_node_6406d71c'];arm=bpy.data.objects['ReconstructedBowArm']
bm=bmesh.new();bm.from_mesh(arm.data)
remaining=set(bm.verts);components=[]
while remaining:
 seed=remaining.pop();component={seed};pending=[seed]
 while pending:
  v=pending.pop()
  for e in v.link_edges:
   other=e.other_vert(v)
   if other in remaining:remaining.remove(other);component.add(other);pending.append(other)
 components.append(component)
largest=max(components,key=len)
bmesh.ops.delete(bm,geom=[v for group in components if group is not largest for v in group],context='VERTS')
bm.to_mesh(arm.data);bm.free()
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
bm=bmesh.new();bm.from_mesh(body.data)
boundary=[e for e in bm.edges if e.is_boundary and all((body.matrix_world@v.co).z>.6 for v in e.verts)]
remaining=set(boundary);loops=[]
while remaining:
 seed=remaining.pop();edges={seed};pending=[seed]
 while pending:
  edge=pending.pop()
  for v in edge.verts:
   for other in v.link_edges:
    if other in remaining:remaining.remove(other);edges.add(other);pending.append(other)
 loops.append(edges)
large=[loop for loop in loops if len(loop)>20]
assert len(large)==2,[len(x) for x in loops]
small,big=sorted(large,key=len)
difference=len(big)-len(small)
minor={e for loop in loops if len(loop)<20 for e in loop}
result=bmesh.ops.subdivide_edges(bm,edges=sorted(small,key=lambda e:e.calc_length(),reverse=True)[:difference],cuts=1,use_grid_fill=False)
small={e for e in bm.edges if e.is_boundary and e not in big and e not in minor and all((body.matrix_world@v.co).z>.6 for v in e.verts)}
def ordered(edges):
 start=next(iter(edges)).verts[0];out=[start];previous=None;current=start
 while True:
  candidates=[e.other_vert(current) for e in current.link_edges if e in edges and e.other_vert(current)!=previous]
  following=next(v for v in candidates if v!=start) if len(out)==1 else candidates[0]
  if following==start:break
  out.append(following);previous,current=current,following
  assert len(out)<=len(edges)
 return out
a,b=ordered(small),ordered(big)
assert len(a)==len(b),(len(a),len(b))
count=len(a);best=None
for direction in [1,-1]:
 for offset in range(count):
  cost=sum((a[i].co-b[(offset+direction*i)%count].co).length_squared for i in range(count))
  if best is None or cost<best[0]:best=(cost,direction,offset)
_,direction,offset=best
bmesh.ops.weld_verts(bm,targetmap={b[(offset+direction*i)%count]:a[i] for i in range(count)})
for loop in loops:
 if len(loop)<20:bmesh.ops.holes_fill(bm,edges=list(loop),sides=0)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(body.data);bm.free();body.data.update()
print('SHOULDER_WELDED',count,best)


# Remove the protruding pommel and guard only in the two hand volumes.
bm=bmesh.new();bm.from_mesh(body.data)
cut=[]
for v in bm.verts:
 p=body.matrix_world@v.co
 if p.y<-.20 and .47<p.z<.55 and p.x<-.084:cut.append(v)
 elif p.y>.20 and .42<p.z<.51 and p.x<-.049:cut.append(v)
 elif p.y<-.20 and .43<p.z<.48 and p.x>.0:cut.append(v)
 elif p.y>.20 and .385<p.z<.438 and p.x>.035:cut.append(v)
bmesh.ops.delete(bm,geom=cut,context='VERTS')
boundaries=[e for e in bm.edges if e.is_boundary]
new=bmesh.ops.holes_fill(bm,edges=boundaries,sides=0)
uv=bm.loops.layers.uv.active
for f in new['faces']:
 f.material_index=0
 for loop in f.loops:
  neighbors=[other for other in loop.vert.link_loops if other.face not in new['faces']]
  if neighbors:loop[uv].uv=neighbors[0][uv].uv.copy()
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(body.data);bm.free();body.data.update()


import bpy;body=bpy.data.objects['tripo_node_6406d71c'];m=bpy.data.materials.new('M_Archer_ShoulderRepairCloth');m.use_nodes=True;n=m.node_tree.nodes;l=m.node_tree.links;p=n.get('Principled BSDF');p.inputs['Roughness'].default_value=.93;p.inputs['Metallic'].default_value=0;t=n.new('ShaderNodeTexNoise');t.inputs['Scale'].default_value=180;t.inputs['Detail'].default_value=2;r=n.new('ShaderNodeValToRGB');r.color_ramp.elements[0].color=(.10,.096,.088,1);r.color_ramp.elements[1].color=(.21,.20,.18,1);l.new(t.outputs['Fac'],r.inputs['Fac']);l.new(r.outputs['Color'],p.inputs['Base Color']);b=n.new('ShaderNodeBump');b.inputs['Strength'].default_value=.13;b.inputs['Distance'].default_value=.0007;l.new(t.outputs['Fac'],b.inputs['Height']);l.new(b.outputs['Normal'],p.inputs['Normal']);body.data.materials.append(m)
for face in body.data.polygons:
 c=body.matrix_world@face.center
 if .05<c.y<.14 and .63<c.z<.855:face.material_index=len(body.data.materials)-1


import bpy,math,json
from mathutils import Vector
body=bpy.data.objects['tripo_node_6406d71c'];rig=bpy.data.objects['Armature']
for obj in list(bpy.context.scene.objects):
 if obj.type=='MESH' and obj!=body:bpy.data.objects.remove(obj,do_unlink=True)
with bpy.data.libraries.load(str(ROOT/'art_source/TASK-095/ArcheryKit.blend'),link=False) as (source,target):
 target.objects=['SM_Longbow_Practical','SM_Quiver_Practical']
bow,quiver=target.objects
for obj in [bow,quiver]:bpy.context.collection.objects.link(obj);obj.hide_render=False;obj.hide_set(False)
bow.location=(.013,.255,.454);bow.rotation_euler=(math.radians(-30),0,0);bow.scale=(.625,)*3
quiver.location=(-.180,-.05,.53);quiver.rotation_euler=(0,0,math.pi);quiver.scale=(.625,)*3
for obj,bone in [(bow,'tripo::0_Right_Limb_2'),(quiver,'tripo::Spine_2')]:
 bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 obj.vertex_groups.clear();obj.vertex_groups.new(name=bone).add(list(range(len(obj.data.vertices))),1,'REPLACE')
 mod=obj.modifiers.new('ExistingSkeleton','ARMATURE');mod.object=rig


import bpy,math
from mathutils import Vector
from mathutils.bvhtree import BVHTree
body=bpy.data.objects['tripo_node_6406d71c'];rig=bpy.data.objects['Armature'];quiver=bpy.data.objects['SM_Quiver_Practical']
body.data.calc_loop_triangles();points=[body.matrix_world@v.co for v in body.data.vertices]
bvh=BVHTree.FromPolygons(points,[tuple(p.vertices) for p in body.data.loop_triangles],all_triangles=True)
vertices=[];faces=[]
for i in range(31):
 t=i/30
 for side in [-1,1]:
  y=-.095+.15*t+side*.006;z=.84-.26*t+side*.0043
  hit,normal,index,distance=bvh.ray_cast(Vector((1,y,z)),Vector((-1,0,0)),2)
  assert hit is not None,(y,z)
  vertices.append(hit+normal*.002)
 if i:faces.append((2*i-2,2*i,2*i+1,2*i-1))
# Back strap passes through the quiver's two mounting loops.
start=len(vertices)
for x,y,z in [(-.075,-.095,.846),(-.13,-.066,.812),(-.135,-.045,.790),(-.130,-.045,.62),(-.125,-.028,.585)]:
 for side in [-1,1]:vertices.append(Vector((x,y+side*.007,z)))
for i in range(4):faces.append((start+2*i,start+2*i+1,start+2*i+3,start+2*i+2))
data=bpy.data.meshes.new('QuiverSling');data.from_pydata(vertices,[],faces);data.update();obj=bpy.data.objects.new('QuiverSling',data);bpy.context.collection.objects.link(obj)
mat=bpy.data.materials.new('M_Archer_Sling');mat.use_nodes=True;p=mat.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(.055,.024,.009,1);p.inputs['Roughness'].default_value=.89;data.materials.append(mat)
for n in ['tripo::Spine_0','tripo::Spine_1','tripo::Spine_2']:obj.vertex_groups.new(name=n)
for v in data.vertices:
 z=v.co.z
 if z<.664:
  t=max(0,min(1,(z-.559)/(.664-.559)));pairs=[('tripo::Spine_0',1-t),('tripo::Spine_1',t)]
 else:
  t=max(0,min(1,(z-.664)/(.750-.664)));pairs=[('tripo::Spine_1',1-t),('tripo::Spine_2',t)]
 for n,w in pairs:obj.vertex_groups[n].add([v.index],w,'REPLACE')
bpy.context.view_layer.objects.active=obj;solid=obj.modifiers.new('LeatherThickness','SOLIDIFY');solid.thickness=.0015;bpy.ops.object.modifier_apply(modifier=solid.name)
mod=obj.modifiers.new('ExistingSkeleton','ARMATURE');mod.object=rig
for o in list(bpy.data.objects):
 if o.type=='ARMATURE' and o!=rig:bpy.data.objects.remove(o,do_unlink=True)



assert bone_before==[(b.name,list(b.head_local),list(b.tail_local),b.parent.name if b.parent else None) for b in rig.data.bones]
for obj in list(bpy.data.objects):
 if obj.type in {'LIGHT','CAMERA'}:bpy.data.objects.remove(obj,do_unlink=True)
body.name='SK_Archer_Practical'
bpy.context.view_layer.objects.active=body
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/Archer-Repaired.blend'))
print('ARCHER_SOURCE_COMPLETE',len(rig.data.bones),len(body.data.vertices))
