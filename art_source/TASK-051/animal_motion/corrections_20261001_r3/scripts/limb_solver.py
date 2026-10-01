"""Fixed-length three-link limbs, upright distal shafts and medial hinges."""
import bpy, math
from mathutils import Vector, Quaternion
from pose_tools import set_world

def unwrap(value, previous):
    if previous is not None:
        while value-previous>math.pi:value-=2*math.pi
        while value-previous<-math.pi:value+=2*math.pi
    return value

def solve_limb(arm, ns, key, old, lengths, centre_y, amplitude, continuity, foot_matrix, species=None):
    """Keep the cannon/shank upright; solve the proximal anatomical hinge."""
    hip, old_elbow, old_hock, foot = old
    l0, l1, l2 = lengths
    side = 1 if key.endswith('L') else -1
    axis = (hip-foot).normalized(); distance = (hip-foot).length
    normal = Vector((0,1,-axis.y/axis.z)).normalized()
    inset = min(lengths)*math.sin(math.radians(3))
    inset = min(inset, max(.0003, abs(hip.y-centre_y)*.20))
    radius = math.sqrt(l2*l2-inset*inset)
    up = Vector((0,0,1))-normal*normal.z; up.normalize()
    lean = math.radians(-6 if key.startswith('F') else -18)
    if species=='hare':
        rest=arm.data.bones[ns[2]].head_local-arm.data.bones[ns[3]].head_local
        lean=max(math.radians(-30),min(math.radians(30),math.atan2(rest.x,rest.z)))
    preferred = (up*math.cos(lean)+Vector((1,0,0))*math.sin(lean)).normalized()
    bend = normal.cross(axis).normalized()
    signed = math.atan2(preferred.dot(bend), preferred.dot(axis))
    low = abs(l0-l1)+.000002; high = l0+l1-.000002
    cmin = (distance*distance+l2*l2-high*high)/(2*distance*radius)
    cmax = (distance*distance+l2*l2-low*low)/(2*distance*radius)
    if cmin>1:
        inset=0.;radius=l2
        cmin=(distance*distance+l2*l2-high*high)/(2*distance*l2)
        cmax=(distance*distance+l2*l2-low*low)/(2*distance*l2)
    if cmin>1.0001:raise RuntimeError('Unreachable foot '+key)
    tmin=math.acos(max(-1,min(1,cmax)));tmax=math.acos(max(-1,min(1,cmin)))
    theta=max(tmin+.00001,min(tmax-.00001,abs(signed)))
    signed=theta*(1 if signed>=0 else -1)
    if species=='hare' and not key.startswith('F'):
        # The hind tarsus remains behind the hip-to-toe line. Switching
        # cone branches when the preferred axis enters the excluded inner
        # reach cone creates a knee snap despite constant segment lengths.
        signed=-theta
    desired=((axis*math.cos(signed)+bend*math.sin(signed))*radius-normal*side*inset).normalized()
    original=(old_hock-foot).normalized()
    old_radial=original-axis*original.dot(axis);new_radial=desired-axis*desired.dot(axis)
    if old_radial.length<1e-7:old_radial=new_radial.copy()
    old_radial.normalize();new_radial.normalize()
    azimuth=unwrap(math.atan2(axis.dot(old_radial.cross(new_radial)),old_radial.dot(new_radial)),continuity.get(key+'_distal'))
    continuity[key+'_distal']=azimuth
    angle0=math.acos(max(-1,min(1,original.dot(axis))));angle1=math.acos(max(-1,min(1,desired.dot(axis))))
    # The torso has less compression than the old loop. Clamp the old distal
    # direction to the new reach cap before interpolating the transition.
    angle0=max(tmin,min(tmax,angle0))
    angle=angle0*(1-amplitude)+angle1*amplitude
    direction=axis*math.cos(angle)+(Quaternion(axis,azimuth*amplitude)@old_radial)*math.sin(angle)
    hock=foot+direction*l2
    delta=hock-hip; d=delta.length; hinge_axis=delta.normalized()
    if not low-1e-5<=d<=high+1e-5:raise RuntimeError('Unreachable proximal chain '+key)
    along=(d*d+l0*l0-l1*l1)/(2*d);height=math.sqrt(max(0,l0*l0-along*along))
    circle=hip+hinge_axis*along
    transverse=normal-hinge_axis*normal.dot(hinge_axis);transverse.normalize()
    other=hinge_axis.cross(transverse).normalized()
    desired_x=-1 if key.startswith('F') else 1
    if species=='hare' and key.startswith('F'):desired_x=1
    sign=1 if other.x*desired_x>0 else -1
    if species=='hare' and key.startswith('F'):
        # A lifted rabbit wrist can pass shoulder height. Keep the same
        # scapular/elbow branch through that crossing instead of switching
        # to the other side of the hinge circle in a single frame.
        sign=1
    amount=(-side*inset*.8-normal.dot(circle-hip))/max(1e-9,height*normal.dot(transverse))
    amount=max(-1,min(1,amount))
    new_perp=transverse*amount+other*math.sqrt(max(0,1-amount*amount))*sign
    old_perp=old_elbow-circle;old_perp-=hinge_axis*old_perp.dot(hinge_axis)
    if old_perp.length<1e-7:old_perp=new_perp.copy()
    old_perp.normalize()
    angle=unwrap(math.atan2(hinge_axis.dot(old_perp.cross(new_perp)),old_perp.dot(new_perp)),continuity.get(key+'_proximal'))
    continuity[key+'_proximal']=angle
    elbow=circle+(Quaternion(hinge_axis,angle*amplitude)@old_perp)*height
    joints=[hip,elbow,hock,foot]
    old_frames=[arm.pose.bones[n].matrix.to_quaternion() for n in ns[:3]]
    for i in range(3):
        set_world(arm,ns[i],joints[i],joints[i+1])
        if amplitude<.999999:
            # Preserve the original axial twist at the idle endpoint and
            # ease it into the stable hinge frame. Joint positions alone
            # cannot prevent a sudden 100-degree change in shaft roll.
            p=arm.pose.bones[ns[i]];q=p.matrix.to_quaternion();axis=(joints[i+1]-joints[i]).normalized()
            old_z=old_frames[i]@Vector((0,0,1));old_z-=axis*old_z.dot(axis)
            new_z=q@Vector((0,0,1));new_z-=axis*new_z.dot(axis)
            if old_z.length>1e-7 and new_z.length>1e-7:
                old_z.normalize();new_z.normalize()
                twist=math.atan2(axis.dot(new_z.cross(old_z)),new_z.dot(old_z))
                twist=unwrap(twist,continuity.get(ns[i]+'_twist'));continuity[ns[i]+'_twist']=twist
                q=Quaternion(axis,twist*(1-amplitude))@q
                matrix=q.to_matrix().to_4x4();matrix.translation=joints[i];p.matrix=matrix;bpy.context.view_layer.update()
    arm.pose.bones[ns[3]].matrix=foot_matrix
    bpy.context.view_layer.update()
    return joints
