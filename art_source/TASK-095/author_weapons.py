"""Original metre-scale practical weapons, with grip at origin and point along +Z.

Run in Blender through MCP. Preserves existing source files. Vertex colours
carry material colour and alpha carries metallic; no external textures needed.
"""
from pathlib import Path
import bpy, bmesh, json, math, sys
from mathutils import Vector, Matrix

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir

for obj in list(bpy.data.objects):
    bpy.data.objects.remove(obj, do_unlink=True)
bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=False, do_recursive=True)
PARTS = []
IRON = (.22, .235, .25, .9)
EDGE = (.38, .39, .40, .92)
WOOD = (.20, .075, .025, 0)
LEATHER = (.048, .022, .011, 0)
CORD = (.31, .24, .15, 0)

def finish(obj, color, bevel=0):
    if bevel:
        mod = obj.modifiers.new('Soft forged edges', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bm = bmesh.new(); bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data); bm.free()
    attr = obj.data.color_attributes.new(name='MaterialColor', type='FLOAT_COLOR', domain='CORNER')
    for loop in obj.data.loops:
        p = obj.data.vertices[loop.vertex_index].co
        grain = .94 + .06 * math.sin(p.z * 7 + math.sin(p.x * 600) * 2 + p.y * 390)
        attr.data[loop.index].color = (*[c * grain for c in color[:3]], color[3])
    obj.data.color_attributes.active_color = attr
    PARTS.append(obj)
    return obj

def mesh(name, verts, faces, color, bevel=0):
    data = bpy.data.meshes.new(name); data.from_pydata(verts, [], faces); data.update()
    obj = bpy.data.objects.new(name, data); bpy.context.collection.objects.link(obj)
    return finish(obj, color, bevel)

def tube(name, points, radii, color, sides=12):
    points = [Vector(p) for p in points]; verts=[]; faces=[]
    for i, p in enumerate(points):
        axis = (points[min(i+1,len(points)-1)]-points[max(i-1,0)]).normalized()
        u=axis.cross(Vector((0,1,0)))
        if u.length<.1:u=axis.cross(Vector((1,0,0)))
        u.normalize(); v=axis.cross(u)
        r=radii[i]; rx,ry=r if isinstance(r,tuple) else (r,r)
        verts.extend(p+u*(rx*math.cos(j*math.tau/sides))+v*(ry*math.sin(j*math.tau/sides)) for j in range(sides))
        if i:
            for j in range(sides):
                a=(i-1)*sides+j; b=(i-1)*sides+(j+1)%sides
                faces.append((a,b,b+sides,a+sides))
    faces += [tuple(reversed(range(sides))),tuple(range(len(verts)-sides,len(verts)))]
    return mesh(name,verts,faces,color)

def box(name,center,size,color,bevel=.002):
    bpy.ops.mesh.primitive_cube_add(size=1,location=center)
    obj=bpy.context.object;obj.name=name;obj.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(obj,color,bevel)

def blade(name,base,length,width,thickness):
    verts=[]
    for z,w,t in [(base,width,thickness),(base+length*.65,width*.75,thickness*.8),(base+length*.90,width*.38,thickness*.5)]:
        verts += [(-w/2,0,z),(0,-t/2,z),(w/2,0,z),(0,t/2,z)]
    verts.append((0,0,base+length));faces=[(3,2,1,0)]
    for r in range(2):
        for j in range(4):faces.append((r*4+j,r*4+(j+1)%4,(r+1)*4+(j+1)%4,(r+1)*4+j))
    for j in range(4):faces.append((8+j,8+(j+1)%4,12))
    return mesh(name,verts,faces,EDGE)

def wrap(z0,z1,radius):
    n=round((z1-z0)/.009)
    points=[(radius*math.cos(i*math.tau/12),radius*math.sin(i*math.tau/12),z0+(z1-z0)*i/(n*12)) for i in range(n*12+1)]
    tube('Leather winding',points,[.0015]*len(points),LEATHER,6)

def sword(long=False):
    grip=.17 if long else .115
    tube('Leather grip',[(0,0,-grip/2),(0,0,grip/2)],[(.018,.013),(.016,.012)],LEATHER)
    wrap(-grip/2,grip/2,.017)
    tube('Pommel',[(0,0,-grip/2-.035),(0,0,-grip/2-.017),(0,0,-grip/2)], [.009,.026,.015],IRON)
    box('Crossguard',(0,0,grip/2+.008),(.21 if long else .135,.025,.019),IRON,.006)
    blade('Forged blade',grip/2+.018,.83 if long else .40,.052 if long else .038,.008)

def spear():
    tube('Ash shaft',[(0,0,-.85),(0,0,.82)],[.016,.013],WOOD)
    tube('Butt ferrule',[(0,0,-.90),(0,0,-.84)],[.009,.017],IRON)
    tube('Head socket',[(0,0,.78),(0,0,.90)],[.016,.012],IRON)
    blade('Spearhead',.88,.31,.059,.009)
    wrap(-.065,.065,.017)

def waraxe():
    tube('Ash haft',[(0,0,-.13),(0,0,.48)],[.019,.015],WOOD)
    wrap(-.07,.09,.020)
    tube('Pommel',[(0,0,-.14),(0,0,-.11)],[.022,.022],IRON)
    tube('Head eye',[(0,0,.41),(0,0,.53)],[.028,.024],IRON)
    profile=[(-.045,.43),(-.045,.50),(.025,.525),(.145,.575),(.155,.365),(.045,.41)]
    verts=[(x,sign*(.002 if x>.14 else .012),z) for sign in [-1,1] for x,z in profile]
    faces=[tuple(reversed(range(6))),tuple(range(6,12))]+[(j,(j+1)%6,(j+1)%6+6,j+6) for j in range(6)]
    mesh('Forged bearded axe head',verts,faces,IRON,.0015)

def crossbow():
    # Local +Z is shot direction. +Y is its upper side, +X spans the limbs.
    tube('Walnut tiller',[(0,-.025,-.30),(0,0,-.04),(0,0,.38)],[(.042,.035),(.029,.030),(.022,.023)],WOOD)
    box('Bolt rail',(0,.032,.11),(.018,.009,.50),IRON)
    for x in [-.013,.013]:box('Rail cheek',(x,.035,.11),(.005,.012,.50),WOOD)
    pts=[(.38*t,-.005,.36-.10*abs(t)**1.5) for t in [i/12 for i in range(-12,13)]]
    tube('Steel prod',pts,[(.007,.012*(1-abs(i-12)/24)) for i in range(25)],IRON)
    tube('Hemp string',[(-.38,.015,.26),(0,.038,.02),(.38,.015,.26)],[.0018]*3,CORD,8)
    box('Release nut',(0,.030,.013),(.040,.035,.025),IRON,.004)
    tube('Trigger lever',[(0,-.03,-.035),(0,-.09,-.055),(0,-.09,-.10)],[.006]*3,IRON)
    tube('Trigger guard',[(0,-.025,-.13),(0,-.105,-.13),(0,-.105,-.02),(0,-.025,-.02)],[.004]*4,IRON)
    tube('Spanning stirrup',[(-.028,0,.37),(-.045,0,.46),(.045,0,.46),(.028,0,.37)],[.007]*4,IRON)
    for x in [-.021,.021]:
        for z in [.31,.33,.35]:tube('Prod lashing',[(x,-.022,z),(x,.025,z)],[.003]*2,CORD,6)

scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
report=[];objects=[]
for label,build in [('Shortblade',sword),('Longblade',lambda:sword(True)),('Spear',spear),('Waraxe',waraxe),('Crossbow',crossbow)]:
    PARTS.clear();build();bpy.ops.object.select_all(action='DESELECT')
    for obj in PARTS:obj.select_set(True)
    bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join();obj=bpy.context.object
    obj.name='SM_'+label+'_Practical';scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    mat=bpy.data.materials.new('M_'+label+'_Practical');mat.use_nodes=True
    nodes=mat.node_tree.nodes;bsdf=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
    color=nodes.new('ShaderNodeVertexColor');color.layer_name='MaterialColor'
    mat.node_tree.links.new(color.outputs['Color'],bsdf.inputs['Base Color'])
    mat.node_tree.links.new(color.outputs['Alpha'],bsdf.inputs['Metallic']);bsdf.inputs['Roughness'].default_value=.58
    obj.data.materials.clear();obj.data.materials.append(mat)
    obj.data.calc_loop_triangles();assert len(obj.data.loop_triangles)<20000
    output=task_output_dir('Hearthward','3d_object','TASK-095-'+label.lower(),'weapons-20261008');output.mkdir(parents=True,exist_ok=True)
    fbx=output/(obj.name+'.fbx');source=obj.data;obj.data=source.copy();obj.data.transform(Matrix.Scale(100,4))
    scene.unit_settings.scale_length=.01
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},apply_scale_options='FBX_SCALE_ALL',axis_forward='-Z',axis_up='Y',bake_anim=False,add_leaf_bones=False,colors_type='LINEAR')
    exported=obj.data;obj.data=source;bpy.data.meshes.remove(exported);scene.unit_settings.scale_length=1
    bpy.context.view_layer.update()
    report.append(dict(piece=label,fbx=str(fbx),dimensions_m=list(obj.dimensions),triangles=len(source.loop_triangles),grip_m=[0,0,0],long_axis='+Z',forward='+Z',materials='vertex RGB colour, alpha metallic',source='Original authored geometry for Hearthward'))
    objects.append(obj);obj.hide_set(True)
for i,obj in enumerate(objects):obj.hide_set(False);obj.location=(i*.6,0,.95)
scene.world=bpy.data.worlds.new('WeaponReviewWorld')
scene.world.color=(.12,.12,.12)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/Weapons-Practical.blend'))
out=ROOT/'.agent-local/qa/TASK-095/weapons';out.mkdir(parents=True,exist_ok=True)
(out/'source.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report))
