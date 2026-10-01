"""Local pose construction with explicit limb planes, no stretchy IK."""
import bpy,math
from mathutils import Vector,Matrix
def set_world(arm,name,point,end):
 b=arm.data.bones[name];rest=b.tail_local-b.head_local
 # A fixed transverse axis gives a continuous frame even when a folded
 # cannon points almost opposite to its rest direction. A shortest-vector
 # rotation alone has an ambiguous twist at 180 degrees.
 def frame(direction):
  y=direction.normalized();z=Vector((0,1,0))-y*y.y;z.normalize();x=y.cross(z).normalized()
  return Matrix((x,y,z)).transposed().to_quaternion()
 q=frame(end-point) @ frame(rest).inverted() @ b.matrix_local.to_quaternion()
 m=q.to_matrix().to_4x4();m.translation=point
 arm.pose.bones[name].matrix=m
 bpy.context.view_layer.update()
def solve_three(arm,ns,hip,foot,upper_direction,plane_normal,standing,knee_y_bounds=None):
 lengths=[(standing[ns[i+1]]-standing[ns[i]]).length for i in range(3)]
 elbow=hip+upper_direction.normalized()*lengths[0]
 delta=foot-elbow;distance=delta.length;a,b=lengths[1:]
 if not abs(a-b)+.00001<distance<a+b-.00001:
  raise ValueError(f'Unreachable limb {ns[0]} lower reach {distance} limits {abs(a-b)} {a+b}')
 axis=delta.normalized();perp=plane_normal.cross(axis).normalized()
 along=(distance*distance+a*a-b*b)/(2*distance);height=math.sqrt(max(0,a*a-along*along))
 centre=elbow+axis*along
 knee=centre+perp*height
 if knee_y_bounds is not None and height>1e-7:
  target_y=max(knee_y_bounds[0],min(knee_y_bounds[1],knee.y))
  if abs(target_y-knee.y)>1e-7:
   lateral=Vector((0,1,0))-axis*axis.y
   if lateral.length>1e-6:
    lateral.normalize();other=axis.cross(lateral).normalized()
    amount=max(-1,min(1,(target_y-centre.y)/(height*lateral.y)))
    sign=1 if perp.dot(other)>=0 else -1
    perp=lateral*amount+other*math.sqrt(max(0,1-amount*amount))*sign
    knee=centre+perp*height
 joints=[hip,elbow,knee,foot]
 for i in range(3):set_world(arm,ns[i],joints[i],joints[i+1])
 eff=ns[3];m=arm.data.bones[eff].matrix_local.copy();m.translation=foot;arm.pose.bones[eff].matrix=m
 bpy.context.view_layer.update()
 return joints

