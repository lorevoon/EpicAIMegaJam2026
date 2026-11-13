"""Author identity-aligned dressing sublevels around the single shared Act 1 action scene."""
import unreal as u, math, random, json
from pathlib import Path
root=Path(__file__).resolve().parents[1];base='/Game/DirectorsCut/Levels/';main=base+'L_DC_Act1_Promise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(main)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert not u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Set'),'Already split'
actors=list(aes.get_all_level_actors())
shared_labels={'Mill scenic platform','Tree scenic island','Practical downhill path','Mill acting apron','Crew access ramp','Practical promise tree','Mill-side bench seat','Bench back','Bench leg'}
shared_classes=(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCZombieDirector,u.DCStartMark,u.DCClapperboard)
studio=[]
for a in actors:
 if a.get_attach_parent_actor():continue # Child actors travel with their owning blueprint.
 if isinstance(a,u.WorldSettings) or a.get_class()==u.Brush:continue
 if isinstance(a,(u.ProceduralFoliageVolume,u.ProceduralFoliageBlockingVolume,u.LightmassImportanceVolume)):
  aes.destroy_actor(a);continue
 if isinstance(a,shared_classes) or a.get_actor_label() in shared_labels or 'DC_MillAssembly' in [str(t) for t in a.tags]:
  a.set_folder_path('Act1/Shared action')
  if isinstance(a,u.DCClapperboard) or (isinstance(a,u.DCPerformer) and a.get_editor_property('ruins_shot')):
   a.tags=list(a.tags)+['DC_SetOnly']
 else:studio.append(a)
# Save a dressing-only copy without clipboard-based actor moves (unavailable in commandlets).
studio_names={a.get_name() for a in studio}
assert u.EditorLoadingAndSavingUtils.save_map(world,base+'L_DC_Act1_Set')
assert les.load_level(base+'L_DC_Act1_Set')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in list(aes.get_all_level_actors()):
 if a.get_attach_parent_actor():continue
 if a.get_name() not in studio_names and not isinstance(a,u.WorldSettings) and a.get_class()!=u.Brush:
  aes.destroy_actor(a)
assert u.EditorLoadingAndSavingUtils.save_map(world,base+'L_DC_Act1_Set')
assert les.load_level(main)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in list(aes.get_all_level_actors()):
 if a.get_name() in studio_names:aes.destroy_actor(a)
 if isinstance(a,u.DCClapperboard) or (isinstance(a,u.DCPerformer) and a.get_editor_property('ruins_shot')):a.tags=list(a.tags)+['DC_SetOnly']
set_layer=u.EditorLevelUtils.add_level_to_world(world,base+'L_DC_Act1_Set',u.LevelStreamingDynamic)
assert set_layer
real_layer=u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingDynamic,base+'L_DC_Act1_Real',False)
assert real_layer
u.EditorLevelUtils.make_level_current(real_layer)
def spawn(cls,label,p=(0,0,0),rot=None):
 a=aes.spawn_actor_from_class(cls,u.Vector(*p),rot or u.Rotator());a.set_actor_label(label);a.set_folder_path('Act1/Real environment');a.tags=['DC_RealEnvironment'];return a
def mesh(label,path,p,scale=(1,1,1),yaw=0):
 a=spawn(u.StaticMeshActor,label,p,u.Rotator(yaw=yaw));a.static_mesh_component.set_static_mesh(u.load_asset(path));a.set_actor_scale3d(u.Vector(*scale));return a
# Generate one continuous outdoor ground mesh. Its top stays just below the shared practical
# surfaces, preserving their exact slopes and collision in BOTH versions.
xs=sorted(set(range(-18000,18001,400))|set(range(-3000,3001,100))|{-2800,1600,850,1550,900,1500,700,1800})
ys=sorted(set(range(-18000,18001,400))|set(range(-3000,3001,100))|{-250,2050,200,800,-600,300,-875,-125})
def height(x,y):
 h=-2.
 if -2800<=x<=1600 and -250<=y<=2050:h=19.8
 if 700<=x<=1800 and -875<=y<=-125:h=max(h,9.8)
 if 850<=x<=1550 and 200<=y<=800:h=max(h,59.8)
 if 900<=x<=1500 and -600<=y<=300:h=max(h,11+(y+600)/18-.2)
 # Rounded hills begin far beyond the action, so the set's blocking never changes.
 r=math.hypot(x,y);fade=max(0,min(1,(r-4000)/5000))
 return h+fade*(350+260*math.sin(x/2700)*math.cos(y/2300)+180*math.sin(y/1100+x/3300))
obj=root/'Saved/FilmSetAudit/Act1_RealGround.obj'
lines=['o Act1_RealGround']
for y in ys:
 for x in xs:lines.append(f'v {x} {-y} {height(x,y)}') # OBJ importer flips Y; retain Z-up.
for y in ys:
 for x in xs:lines.append(f'vt {x/1500} {y/1500}')
w=len(xs)
for j in range(len(ys)-1):
 for i in range(w-1):
  a=j*w+i+1;b=a+1;c=a+w;d=c+1
  lines.extend([f'f {a}/{a} {b}/{b} {c}/{c}',f'f {b}/{b} {d}/{d} {c}/{c}'])
obj.write_text('\n'.join(lines))
task=u.AssetImportTask();task.filename=str(obj);task.destination_path='/Game/DirectorsCut/Environment';task.destination_name='SM_Act1_RealGround';task.automated=True;task.replace_existing=True;task.save=True
options=u.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False);options.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
task.options=options;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);assert task.imported_object_paths
terrain=mesh('Outdoor ground - matches shared slopes',task.imported_object_paths[0],(0,0,0))
# A softly mottled ground material is cheap to render and has no grass-card transparency.
mat=u.load_asset('/Game/DirectorsCut/Materials/M_Act1_OutdoorGround') or u.AssetToolsHelpers.get_asset_tools().create_asset('M_Act1_OutdoorGround','/Game/DirectorsCut/Materials',u.Material,u.MaterialFactoryNew())
mat.set_editor_property('two_sided',True)
c=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-200,0);c.constant=u.LinearColor(.12,.19,.045,1);u.MaterialEditingLibrary.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
u.MaterialEditingLibrary.recompile_material(mat);u.EditorAssetLibrary.save_loaded_asset(mat,False);terrain.static_mesh_component.set_material(0,mat)
terrain.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
sun=spawn(u.DirectionalLight,'Real morning sun',(0,0,4000),u.Rotator(pitch=-35,yaw=145));lc=sun.get_component_by_class(u.DirectionalLightComponent);lc.set_mobility(u.ComponentMobility.MOVABLE);lc.set_intensity(1000.);lc.set_editor_property('atmosphere_sun_light',True)
spawn(u.SkyAtmosphere,'Real atmosphere')
sky=spawn(u.SkyLight,'Real sky fill',(0,0,3000));sc=sky.get_component_by_class(u.SkyLightComponent);sc.set_mobility(u.ComponentMobility.MOVABLE);sc.set_intensity(3.);sc.set_editor_property('real_time_capture',True)
fog=spawn(u.ExponentialHeightFog,'Real distant haze',(0,0,-100));fog.get_component_by_class(u.ExponentialHeightFogComponent).set_editor_property('fog_density',.006)
pp=spawn(u.PostProcessVolume,'Real exposure');pp.set_editor_property('unbound',True);pp.set_editor_property('priority',100.);settings=pp.get_editor_property('settings')
for k,v in dict(override_auto_exposure_min_brightness=True,override_auto_exposure_max_brightness=True,auto_exposure_min_brightness=5.,auto_exposure_max_brightness=5.).items():settings.set_editor_property(k,v)
pp.set_editor_property('settings',settings)
rng=random.Random(2026)
for i in range(80):
 angle=i*math.tau/80;r=rng.uniform(4300,11000);x=math.cos(angle)*r;y=math.sin(angle)*r;s=rng.uniform(.8,1.7)
 mesh('Background tree '+str(i),'/Game/WindmillValley/Meshes/SM_Tree_02',(x,y,height(x,y)),(s,s,s),rng.uniform(0,360))
for i in range(14):
 angle=i*math.tau/14;r=13000;x=math.cos(angle)*r;y=math.sin(angle)*r
 mesh('Distant rocky ridge '+str(i),'/Game/WindmillValley/Meshes/SM_Rock_01',(x,y,height(x,y)-100),(12,10,rng.uniform(5,9)),rng.uniform(0,360))
# Both dressing layers use identity transforms and are loaded at startup. Only studio is initially visible.
for layer,visible in [(set_layer,True),(real_layer,False)]:
 layer.set_editor_property('level_transform',u.Transform());layer.set_editor_property('initially_loaded',True);layer.set_editor_property('initially_visible',visible)
 layer.set_editor_property('should_be_loaded',True);layer.set_editor_property('should_be_visible',visible)
 u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
director=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCZombieDirector))
director.set_editor_property('studio_environment_level','L_DC_Act1_Set');director.set_editor_property('real_environment_level','L_DC_Act1_Real')
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert u.EditorLoadingAndSavingUtils.save_map(world,main)
(root/'Saved/FilmSetAudit/dual_scene_build.json').write_text(json.dumps({'persistent':main,'studio':base+'L_DC_Act1_Set','real':base+'L_DC_Act1_Real','shared_geometry':sorted(shared_labels),'studio_actor_count':len(studio),'identity_alignment':True},indent=2))
u.log('DUAL_SCENE_AUTHORED')
