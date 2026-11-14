"""Copy Act 1 practical buildings, stage the wildfire sky, and survey clear raid lanes."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/Revision2';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools()
les.load_level(BASE+'/Levels/L_DC_Act1_Promise')
buildings=[]
for a in aes.get_all_level_actors():
 if a.get_actor_label() in ['BP_Windmill2','BP_Building9','BP_SmallHouse4']:
  buildings.append((a.get_actor_label(),a.get_class(),a.get_actor_transform()))
assert len(buildings)==3
les.load_level(BASE+'/Levels/L_DC_Act2_Set');labels={a.get_actor_label() for a in aes.get_all_level_actors()}
for label,cls,tf in buildings:
 if label in labels:continue
 a=aes.spawn_actor_from_class(cls,tf.translation,tf.rotation.rotator());a.set_actor_transform(tf,False,False);a.set_actor_label(label);a.set_folder_path('Act2/Studio practical buildings');a.tags=['DC_Act2Studio']
assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),BASE+'/Levels/L_DC_Act2_Set')
les.load_level(BASE+'/Levels/L_DC_Act2_Real');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
# A large, opaque unlit dome replaces the blue sky only in the real layer.
path=F+'/M_WildfireAshSky';mat=u.load_asset(path)
if not mat:
 mat=at.create_asset('M_WildfireAshSky',F,u.Material,u.MaterialFactoryNew());mat.set_editor_property('two_sided',True);mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
 p=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionWorldPosition,0,0);t=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionTime,0,160);custom=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionCustom,280,0)
 inputs=[]
 for name in ['P','T']:
  i=u.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
 custom.set_editor_property('inputs',inputs);custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
 custom.set_editor_property('code','''float3 d=normalize(P-float3(-5800,-9400,1100));
 float2 q=d.xy/(0.35+abs(d.z))*2.4+float2(T*0.006,T*0.002);
 float n=0; float amp=0.55;
 for(int k=0;k<5;k++){float2 i=floor(q),f=frac(q);f=f*f*(3-2*f);
 float a=frac(sin(dot(i,float2(127.1,311.7)))*43758.5453);
 float b=frac(sin(dot(i+float2(1,0),float2(127.1,311.7)))*43758.5453);
 float c=frac(sin(dot(i+float2(0,1),float2(127.1,311.7)))*43758.5453);
 float e=frac(sin(dot(i+1,float2(127.1,311.7)))*43758.5453);
 n+=amp*lerp(lerp(a,b,f.x),lerp(c,e,f.x),f.y);q=q*2.03+7.1;amp*=0.5;}
 float horizon=pow(1-saturate(abs(d.z)),2);
 float smoke=smoothstep(0.32,0.74,n);
 float3 glow=lerp(float3(0.14,0.008,0.003),float3(0.6,0.045,0.006),horizon);
 float3 col=lerp(glow,float3(0.008,0.006,0.007),smoke);
 float2 aq=d.xy/(0.4+abs(d.z))*170+float2(T*0.13,-T*0.23);
 float seed=frac(sin(dot(floor(aq),float2(91.7,47.3)))*23751.34);
 float flake=(1-smoothstep(0.03,0.12,length(frac(aq)-0.5)))*step(0.91,seed);
 return lerp(col,float3(0.16,0.12,0.095),flake*0.7);''')
 u.MaterialEditingLibrary.connect_material_expressions(p,'',custom,'P');u.MaterialEditingLibrary.connect_material_expressions(t,'',custom,'T');u.MaterialEditingLibrary.connect_material_property(custom,'',u.MaterialProperty.MP_EMISSIVE_COLOR);u.MaterialEditingLibrary.recompile_material(mat);assert eal.save_loaded_asset(mat,False)
actors=list(aes.get_all_level_actors())
if not any(a.get_actor_label()=='Act2 red-black ash sky' for a in actors):
 a=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector(-5800,-9400,1100));a.set_actor_label('Act2 red-black ash sky');a.set_folder_path('Act2/Wildfire atmosphere');a.set_actor_scale3d(u.Vector(2000,2000,2000));a.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Sphere'));a.static_mesh_component.set_material(0,mat);a.static_mesh_component.set_cast_shadow(False);a.set_actor_enable_collision(False)
for a in actors:
 if isinstance(a,u.SkyLight):
  c=a.get_component_by_class(u.SkyLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_intensity(0.25);c.set_light_color(u.LinearColor(.6,.13,.07,1))
 if isinstance(a,u.DirectionalLight):
  c=a.get_component_by_class(u.DirectionalLightComponent);c.set_intensity(0.9);c.set_light_color(u.LinearColor(1,.27,.12,1));a.set_actor_rotation(u.Rotator(pitch=-14,yaw=-35),False)
 if isinstance(a,u.AtmosphericFog):a.set_actor_hidden_in_game(True);a.set_is_temporarily_hidden_in_editor(True)
if not any(a.get_actor_label()=='Wildfire smoke haze' for a in actors):
 a=aes.spawn_actor_from_class(u.ExponentialHeightFog,u.Vector(0,0,900));a.set_actor_label('Wildfire smoke haze');a.set_folder_path('Act2/Wildfire atmosphere');c=a.get_component_by_class(u.ExponentialHeightFogComponent);c.set_editor_property('fog_density',.012);c.set_editor_property('fog_height_falloff',.15);c.set_editor_property('fog_inscattering_luminance',u.LinearColor(.09,.018,.009,1));c.set_editor_property('start_distance',1200.)
# Fire curtain sits behind the clear southern approach, never on the walkway.
for a in actors:
 if a.get_actor_label().startswith('Orc entrance fire curtain'):
  index=int(a.get_actor_label().split()[-1]);a.set_actor_location(u.Vector(-6200+index*450,-11100,1250),False,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,BASE+'/Levels/L_DC_Act2_Real')
les.load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for name,vis in [('L_DC_Act2_Set',False),('L_DC_Act2_Real',True)]:u.EditorLevelUtils.set_level_visibility(u.GameplayStatics.get_streaming_level(world,name).get_loaded_level(),vis,False)
actors=list(aes.get_all_level_actors());ignore=[a for a in actors if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.CameraActor,u.LandscapeProxy))];groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
grid=[]
for y in range(-11000,-8799,100):
 for x in range(-6400,-4999,100):
  hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,0),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True)
  if not hit:continue
  z=hit.to_tuple()[4].z
  r=u.SystemLibrary.sphere_trace_single(world,u.Vector(x,y,z+90),u.Vector(x,y,z+100),50,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,u.DrawDebugTrace.NONE,True)
  if r:grid.append({'x':x,'y':y,'hit':str(r.to_tuple()[9])})
(ROOT/'Saved/Act2/Revision2/blocked_grid.json').write_text(json.dumps(grid,indent=2));print('REVISION2_ENVIRONMENT_READY')
