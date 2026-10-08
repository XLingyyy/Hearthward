"""Author bow articulation and hand-constrained poses on the repaired source.

Run with Archer-Repaired.blend open. Body bones retain their reference pose;
the independent archer skeleton adds bow controls without editing Guard.
"""
from pathlib import Path
import bpy,bmesh,json,math
from mathutils import Matrix,Vector,Quaternion

ROOT=Path(__file__).resolve().parents[2]
rig=bpy.data.objects['Armature']
rig.data.pose_position='POSE'
assert Path(bpy.data.filepath).name=='Archer-Repaired.blend'
for obj in list(bpy.data.objects):
 if obj.type=='ARMATURE' and obj!=rig:bpy.data.objects.remove(obj,do_unlink=True)
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
transforms={o:o.matrix_world.copy() for o in meshes}
world=rig.matrix_world.copy()
for obj in meshes:
 obj.data.transform(transforms[obj]);obj.parent=None;obj.matrix_world=Matrix.Identity(4)
rig.data.transform(world);rig.matrix_world=Matrix.Identity(4)
original={b.name:b.matrix_local.copy() for b in rig.data.bones}
bow=bpy.data.objects['SM_Longbow_Practical']
grip=Vector((.013,.255,.454))
rest_bow=Matrix.Translation(grip)@Matrix.Rotation(math.radians(-30),4,'X')
to_bow=rest_bow.inverted()
# Replace the original two-endpoint string with a string that can bend at nock.
bm=bmesh.new();bm.from_mesh(bow.data)
string=[v for v in bm.verts if abs((to_bow@v.co).x+.09375)<.0007 and abs((to_bow@v.co).y)<.0007]
assert len(string)==12,len(string)
uv_layer=bm.loops.layers.uv.active
string_uv=next(l[uv_layer].uv.copy() for v in string for l in v.link_loops)
bmesh.ops.delete(bm,geom=string,context='VERTS');bm.to_mesh(bow.data);bm.free()
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT')
specs=[('BowGrip',grip,'tripo::0_Right_Limb_2'),
       ('BowUpper',rest_bow@Vector((-.09375,0,.46875)),'BowGrip'),
       ('BowLower',rest_bow@Vector((-.09375,0,-.46875)),'BowGrip'),
       ('BowNock',rest_bow@Vector((-.09375,0,0)),'BowGrip'),
       ('NockedArrow',rest_bow@Vector((.40375,0,0)),'BowNock')]
for name,head,parent in specs:
 b=rig.data.edit_bones.new(name);b.head=head;b.tail=head+Vector((0,0,.04));b.parent=rig.data.edit_bones[parent]
bpy.ops.object.mode_set(mode='OBJECT')
bow.vertex_groups.clear()
for name in ['BowGrip','BowUpper','BowLower']:bow.vertex_groups.new(name=name)
for v in bow.data.vertices:
 z=(to_bow@v.co).z;weight=min(1,(abs(z)/.46875)**2)
 bow.vertex_groups['BowUpper' if z>=0 else 'BowLower'].add([v.index],weight,'REPLACE')
 bow.vertex_groups['BowGrip'].add([v.index],1-weight,'REPLACE')
verts=[];faces=[];weights=[]
for half,tip in [(-1,'BowLower'),(1,'BowUpper')]:
 start=len(verts)
 for i in range(13):
  t=i/12;point=rest_bow@Vector((-.09375,0,half*.46875*t))
  for j in range(6):
   v=point+rest_bow.to_3x3()@Vector((math.cos(j*math.tau/6)*.0006,math.sin(j*math.tau/6)*.0006,0))
   verts.append(v);weights.append((tip,t))
  if i:
   for j in range(6):
    a=start+(i-1)*6+j;b=start+(i-1)*6+(j+1)%6;faces.append((a,b,b+6,a+6))
data=bpy.data.meshes.new('ArticulatedBowstring');data.from_pydata(verts,[],faces);data.update()
obj=bpy.data.objects.new('ArticulatedBowstring',data);bpy.context.collection.objects.link(obj)
data.materials.append(bow.data.materials[0]);uv=data.uv_layers.new()
for loop in uv.data:loop.uv=string_uv
for name in ['BowLower','BowUpper','BowNock']:obj.vertex_groups.new(name=name)
for i,(name,t) in enumerate(weights):
 obj.vertex_groups[name].add([i],t,'REPLACE');obj.vertex_groups['BowNock'].add([i],1-t,'REPLACE')
mod=obj.modifiers.new('BowRig','ARMATURE');mod.object=rig
with bpy.data.libraries.load(str(ROOT/'art_source/TASK-095/ArcheryKit.blend'),link=False) as (source,target):target.objects=['SM_Arrow_Practical']
arrow=target.objects[0];bpy.context.collection.objects.link(arrow);arrow.name='NockedArrowMesh'
arrow.matrix_world=Matrix.Translation(specs[-1][1])@Matrix.Scale(.625,4)
arrow.data.transform(arrow.matrix_world);arrow.matrix_world=Matrix.Identity(4)
arrow.vertex_groups.clear();arrow.vertex_groups.new(name='NockedArrow').add(list(range(len(arrow.data.vertices))),1,'REPLACE')
mod=arrow.modifiers.new('BowRig','ARMATURE');mod.object=rig
for name,matrix in original.items():
 assert max(abs(rig.data.bones[name].matrix_local[i][j]-matrix[i][j]) for i in range(4) for j in range(4))<1e-5,name
REST={b.name:b.matrix_local.copy() for b in rig.data.bones}

def reset():
 for p in rig.pose.bones:p.matrix_basis=Matrix.Identity(4);p.rotation_mode='QUATERNION'
 bpy.context.view_layer.update()

def bone_between(name,head,tail,twist=0):
 b=rig.data.bones[name];direction=(tail-head).normalized()
 q=(b.tail_local-b.head_local).normalized().rotation_difference(direction)
 if twist:q=Quaternion(direction,twist)@q
 m=(q.to_matrix()@REST[name].to_3x3()).to_4x4();m.translation=head
 rig.pose.bones[name].matrix=m
 bpy.context.view_layer.update()

def arm(prefix,hand,pole,grip_point,twist=0):
 upper=prefix+'_1';lower=prefix+'_2'
 shoulder=rig.pose.bones[upper].head.copy();a=rig.data.bones[upper].length;b=rig.data.bones[lower].length
 wrist=Vector(hand);offset=Vector(grip_point)-rig.data.bones[lower].tail_local
 for _ in range(5):
  d=wrist-shoulder;length=min(a+b-.0005,max(abs(a-b)+.0005,d.length));axis=d.normalized()
  normal=(Vector(pole)-shoulder);normal=(normal-axis*normal.dot(axis)).normalized()
  along=(a*a+length*length-b*b)/(2*length);height=math.sqrt(max(0,a*a-along*along))
  elbow=shoulder+axis*along+normal*height;end=shoulder+axis*length
  bone_between(upper,shoulder,elbow);bone_between(lower,elbow,end,twist)
  rotation=rig.pose.bones[lower].matrix.to_3x3()@REST[lower].to_3x3().inverted()
  wrist=Vector(hand)-rotation@offset
 return rig.pose.bones[lower].matrix@REST[lower].inverted()@Vector(grip_point)

def pose(draw=0,raise_bow=1,recoil=0):
 # Hands are positioned in metres on the one-metre source character.
 spine=rig.pose.bones['tripo::Spine_1'];pivot=spine.head.copy()
 turn=Matrix.Translation(pivot)@Matrix.Rotation(math.radians(-30)*raise_bow,4,'Z')@Matrix.Translation(-pivot)
 spine.matrix=turn@spine.matrix;bpy.context.view_layer.update()
 head=rig.pose.bones['tripo::Head_0'];pivot=head.head.copy()
 head.matrix=Matrix.Translation(pivot)@Matrix.Rotation(math.radians(30)*raise_bow,4,'Z')@Matrix.Translation(-pivot)@head.matrix
 bpy.context.view_layer.update()
 hand_rest=Vector((.013,.255,.454));hand_aim=Vector((.305,-.035,.80))
 desired=hand_rest.lerp(hand_aim,raise_bow)
 actual=arm('tripo::0_Right_Limb',desired,(.06,.40,.73),hand_rest)
 rotation=Quaternion((1,0,0),math.radians(-30)*(1-raise_bow))
 bow_matrix=Matrix.Translation(actual)@rotation.to_matrix().to_4x4()
 delta=bow_matrix@rest_bow.inverted()
 rig.pose.bones['BowGrip'].matrix=delta@REST['BowGrip'];bpy.context.view_layer.update()
 # Bending moves tips toward the archer and slightly shortens their separation.
 for name,sign in [('BowUpper',1),('BowLower',-1)]:
  m=delta@REST[name];m.translation=bow_matrix@Vector((-.09375-.070*draw,0,sign*(.46875-.025*draw)))
  rig.pose.bones[name].matrix=m
 nock=bow_matrix@Vector((-.09375-.19*draw,0,.014))
 m=delta@REST['BowNock'];m.translation=nock;rig.pose.bones['BowNock'].matrix=m
 bpy.context.view_layer.update()
 draw_rest=Vector((-.05,-.245,.50))
 draw_target=nock+Vector((-.012*recoil,-.009-.035*recoil,0))
 hand=draw_rest.lerp(draw_target,raise_bow)
 achieved=arm('tripo::1_Left_Limb',hand,(-.25,-.25,.79),draw_rest)
 m=delta@REST['NockedArrow'];m.translation=nock+rotation@Vector((.4975,0,0));rig.pose.bones['NockedArrow'].matrix=m
 bpy.context.view_layer.update()
 return {'bow_grip_error':(desired-actual).length,'draw_hand_error':(hand-achieved).length,'nock':list(nock)}

reset();print('POSE_QA',json.dumps(pose(1,1)))
bpy.context.scene['archer_motion_source']='Original bow/upper-body authoring; existing Guard lower-body animation'
print('ARCHER_RIG_READY',len(rig.data.bones))
