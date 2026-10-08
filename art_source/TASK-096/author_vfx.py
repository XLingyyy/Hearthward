"""UE editor first-article authoring; Epic template copies, original flame shader."""
import unreal,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'.agent-local/qa/TASK-096/vfx-author.json'
lib=unreal.MaterialEditingLibrary;assets=unreal.EditorAssetLibrary;folder='/Game/Hearthward/Assets/TASK-096/Fire/'
r={'passed':False,'systems':[]}
def put(o,k,v):o.set_editor_property(k,v)
def dist(o,k,values,mode):
 d=o.get_editor_property(k)
 mode_name='UniformConstant' if mode==3 else 'NonUniformConstant'
 scalar=str(values[0]) if len(values)==1 else '('+','.join(axis+'='+str(value) for axis,value in zip('XYZ',values))+')'
 assert d.import_text('(Mode='+mode_name+',ChannelConstantsAndRanges=('+','.join(map(str,values))+'),Min='+scalar+',Max='+scalar+')')
 put(o,k,d)
def node(m,cls,**kw):
 n=lib.create_material_expression(m,cls)
 for k,v in kw.items():put(n,k,v)
 return n
def link(a,output,b,pin):assert lib.connect_material_expressions(a,output,b,pin)
def material(name,smoke,texture):
 m=unreal.load_asset(folder+name) if assets.does_asset_exist(folder+name) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder.rstrip('/'),unreal.Material,unreal.MaterialFactoryNew())
 assert m is not None
 lib.delete_all_material_expressions(m)
 put(m,'blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT if smoke else unreal.BlendMode.BLEND_ADDITIVE)
 put(m,'shading_model',unreal.MaterialShadingModel.MSM_UNLIT);put(m,'two_sided',True)
 lib.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES)
 uv=node(m,unreal.MaterialExpressionTextureCoordinate);color=node(m,unreal.MaterialExpressionParticleColor)
 custom=node(m,unreal.MaterialExpressionCustom,output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT4)
 names=['UV','Age'] if smoke else ['UV','Age','Time']
 if smoke:names+=['Tex']
 inputs=[]
 for n in names:
  item=unreal.CustomInput();put(item,'InputName',n);inputs.append(item)
 put(custom,'inputs',inputs)
 # Lightweight emitters expose Color, while NormalizedAge is internal to their modules.
 link(uv,'',custom,'UV');link(color,'A',custom,'Age')
 if smoke:
  tex=node(m,unreal.MaterialExpressionTextureObject,texture=texture);link(tex,'',custom,'Tex')
  code='float4 s=Texture2DSample(Tex,TexSampler,UV); float a=max(s.r,max(s.g,s.b)); float edge=pow(saturate(1-dot(UV*2-1,UV*2-1)),2); a*=edge*Age*0.32; return float4(0.09,0.08,0.075,a);'
 else:
  tim=node(m,unreal.MaterialExpressionTime);link(tim,'',custom,'Time')
  code="""struct NoiseHelper {
 float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
 float noise(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
}; NoiseHelper n;
float y=1-UV.y;
float turbulence=n.noise(float2(UV.x*5,y*4-Time*1.5))+0.45*n.noise(float2(UV.x*11,y*9-Time*3));
float bend=(n.noise(float2(y*3,Time*0.9))-0.5)*0.27*y;
float width=0.42*pow(saturate(1-y),0.7);
float body=saturate((width-abs(UV.x-0.5+bend))*9-(turbulence-0.5)*1.8*y);
body*=smoothstep(0,0.08,y)*(1-smoothstep(0.8,1,y));
float fade=Age;
float hot=saturate(body*1.1-y*0.65);
float3 color=lerp(float3(1,0.035,0.003),float3(1,0.42,0.07),hot)*1.2;
return float4(color,body*fade*0.28);"""
 put(custom,'code',code)
 rgb=node(m,unreal.MaterialExpressionComponentMask,r=True,g=True,b=True);alpha=node(m,unreal.MaterialExpressionComponentMask,r=False,g=False,b=False,a=True)
 link(custom,'',rgb,'');link(custom,'',alpha,'')
 assert lib.connect_material_property(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 assert lib.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
 lib.recompile_material(m);assets.save_loaded_asset(m,False);return m
try:
 texture=unreal.load_asset(folder+'T_SoftSmoke') if assets.does_asset_exist(folder+'T_SoftSmoke') else assets.duplicate_asset('/Engine/Tutorial/SubEditors/TutorialAssets/T_soft_smoke',folder+'T_SoftSmoke');assert texture
 assets.save_loaded_asset(texture,False)
 for smoke in [False,True]:
  name='NS_HearthSmoke' if smoke else 'NS_HearthFire';mat=material('M_HearthSmoke' if smoke else 'M_HearthFlame',smoke,texture)
  system=unreal.load_asset(folder+name) if assets.does_asset_exist(folder+name) else assets.duplicate_asset('/Niagara/DefaultAssets/Templates/Systems/FountainLightweight',folder+name);assert system
  emitter=next(o for o in unreal.ObjectIterator(unreal.load_class(None,'/Script/Niagara.NiagaraStatelessEmitter')) if o.get_path_name().startswith(system.get_path_name()+':'))
  modules={o.get_class().get_name().removeprefix('NiagaraStatelessModule_'):o for o in emitter.get_editor_property('Modules')}
  enabled={'InitializeParticle','ShapeLocation','AddVelocity','SolveVelocitiesAndForces','ApplyOwnerScaleToAttributes','ScaleColor'}
  for key,obj in modules.items():put(obj,'bModuleEnabled',key in enabled)
  init=modules['InitializeParticle'];dist(init,'LifetimeDistribution',[3.2 if smoke else 1.2],3)
  fade=modules['ScaleColor'].get_editor_property('ScaleDistribution')
  assert fade.import_text('(Mode=NonUniformCurve,ValuesTimeRange=(X=0,Y=1),Values=((R=1,G=1,B=1,A=0),(R=1,G=1,B=1,A=1),(R=1,G=1,B=1,A=0.85),(R=1,G=1,B=1,A=0)))')
  put(modules['ScaleColor'],'ScaleDistribution',fade)
  dist(init,'SpriteSizeDistribution',[100,100] if smoke else [65,120],4)
  dist(init,'SpriteRotationDistribution',[0],3)
  shape=modules['ShapeLocation'];dist(shape,'SphereRadius',[18 if smoke else 24],3)
  vel=modules['AddVelocity'];dist(vel,'ConeVelocityDistribution',[65 if smoke else 38],3);put(vel,'ConeAngle',8.0)
  infos=list(emitter.get_editor_property('SpawnInfos'));dist(infos[0],'Rate',[4 if smoke else 10],3);put(emitter,'SpawnInfos',infos)
  renderer=emitter.get_editor_property('RendererProperties')[0];put(renderer,'Material',mat)
  put(emitter,'FixedBounds',unreal.Box(min=unreal.Vector(-250,-250,-150),max=unreal.Vector(250,250,500)))
  assets.save_loaded_asset(system,False)
  r['systems'].append({'path':system.get_path_name(),'material':mat.get_path_name(),'lifetime':3.2 if smoke else 1.2,'spawn_rate':4 if smoke else 10,'fade':'ScaleColor alpha; no material NormalizedAge dependency'})
 r['passed']=True
except Exception:r['error']=traceback.format_exc()
out.write_text(json.dumps(r,indent=2),encoding='utf-8')
