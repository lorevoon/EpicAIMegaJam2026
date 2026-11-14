"""Act 2: shared performance, copied Act 1 studio, burning village replay layer."""
import unreal as u, json, math, shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut/Levels/';MAIN=BASE+'L_DC_Act2_BrokenPromise';SET=BASE+'L_DC_Act2_Set';REAL=BASE+'L_DC_Act2_Real'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
backup=ROOT/'Saved/Act2/BeforeDualScene.umap'
if not backup.exists():shutil.copy2(ROOT/'Content/DirectorsCut/Levels/L_DC_Act2_BrokenPromise.umap',backup)
assert les.load_level(MAIN)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert not u.GameplayStatics.get_streaming_level(world,'L_DC_Act2_Set'),'Already converted; use refinement scripts.'
actors=list(aes.get_all_level_actors())
# Save nearby architectural pieces that Act 1 kept in its shared performance map.
architecture=[]
for a in actors:
 if a.get_attach_parent_actor():continue
 if a.get_class().get_name().startswith(('BP_Windmill','BP_LargeHouse','BP_LittleHouse','BP_Barrels','BP_WheatStack','BP_Cart','BP_Mailbox')):
  architecture.append((a.get_actor_label(),a.get_class().get_path_name(),a.get_actor_transform()))
# Preserve exactly the existing character, waypoint, beat and sequence actor identities.
for a in actors:
 if isinstance(a,u.DCBeat) and a.get_editor_property('beat_id')=='2.1':
  smith=next(x for x in actors if isinstance(x,u.DCPerformer) and x.get_actor_label()=='Mr. Smith')
  monster=next(x for x in actors if isinstance(x,u.DCPerformer) and x.get_actor_label()=='Armoured monster')
  refs=[]
  for target in [smith,monster]:
   r=u.DCAnchorRef();r.set_editor_property('performer',target);r.set_editor_property('part',u.DCAnchorPart.BODY);r.set_editor_property('must_see_face',False);refs.append(r)
  a.set_editor_property('anchors',refs)
  a.set_editor_property('caption','The village catches fire. Mr. Smith turns as a monster advances through the blaze.')
keep_classes=(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCZombieDirector,u.DCStartMark,u.DCClapperboard,u.LandscapeProxy,u.WorldSettings)
removed=[]
for a in list(aes.get_all_level_actors()):
 label=a.get_actor_label()
 if label.startswith('Fire - ignition'):
  removed.append(label);aes.destroy_actor(a);continue
 if isinstance(a,keep_classes) or a.get_class()==u.Brush:continue
 if str(a.get_folder_path()).startswith('Act2/'):
  continue
 if a.get_attach_parent_actor():continue
 removed.append(label);aes.destroy_actor(a)
# Break only orphaned actor calls in the copied showcase Level Blueprint.
from editor_toolset.toolsets.blueprint import BlueprintTools as B
bp=u.load_object(None,MAIN+'.L_DC_Act2_BrokenPromise:PersistentLevel.L_DC_Act2_BrokenPromise')
for g in B.list_graphs(bp):
 for n in B.find_nodes(g):
  if n.get_class().get_name()!='K2Node_Literal' or n.get_node_title()!='Unknown':continue
  for output in B._get_node_info(n).output_pins:
   for target in output.connected_pins:
    for pin in B._get_node_info(target.node).input_pins:
     if pin.name=='execute':
      for upstream in pin.connected_pins:B.break_pins(upstream,pin.pin_id)
u.BlueprintEditorLibrary.compile_blueprint(bp)
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCClapperboard):a.tags=list(a.tags)+['DC_SetOnly']
 if isinstance(a,u.DCPerformer):a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,MAIN)
# The removed prop must not remain as a broken binding in any authored review cut.
for name in ['LS_DC_Act2_BrokenPromise','LS_DC_Act2_Final38s','LS_DC_Act2_38s']:
 seq=u.load_asset('/Game/DirectorsCut/Act2/'+name)
 if not seq:continue
 for binding in seq.get_bindings():
  if 'Fire' in binding.get_name():binding.remove()
 eal.save_loaded_asset(seq,False)
# Copy the actual current Act 1 environments, without changing their source maps.
for src,dst in [('L_DC_Act1_Set',SET),('L_DC_Act1_Real',REAL)]:
 assert not eal.does_asset_exist(dst),dst
 assert eal.duplicate_asset(BASE+src,dst)
assert les.load_level(SET)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in aes.get_all_level_actors():
 a.set_folder_path(str(a.get_folder_path()).replace('Act1','Act2'))
 a.tags=list(a.tags)+['DC_Act2Studio']
assert u.EditorLoadingAndSavingUtils.save_map(world,SET)
assert les.load_level(REAL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
labels={a.get_actor_label() for a in aes.get_all_level_actors()}
for label,cls,tf in architecture:
 if label in labels:continue
 a=aes.spawn_actor_from_class(u.load_class(None,cls),tf.translation,tf.rotation.rotator());a.set_actor_transform(tf,False,False);a.set_actor_label(label);a.set_folder_path('Act2/Real village')
# Preserve a clear action corridor and clean sightlines around both deaths.
for a in list(aes.get_all_level_actors()):
 if a.get_actor_label() in ['BP_Rocks_158','BP_Rocks_159','BP_Rocks_162']:
  a.set_actor_hidden_in_game(True);a.set_actor_enable_collision(False)
 if isinstance(a,u.AmbientSound):a.set_actor_hidden_in_game(True);a.get_component_by_class(u.AudioComponent).set_auto_activate(False)
# Ground lives in the persistent map, so sample its existing landscape heights once.
assert u.EditorLoadingAndSavingUtils.save_map(world,REAL)
assert les.load_level(MAIN)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
ignore=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,9000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
 return hit[4].z if hit[0] else 1200.
# Huge flames surround the action, never occupying the duel/mourning footprint.
points=[]
for i,(x,y,scale) in enumerate([(-7300,-8500,3.0),(-6700,-7900,3.5),(-5900,-7750,3.2),(-5100,-7950,3.5),(-4300,-8500,3.4),(-4150,-9500,3.0),(-4700,-10400,3.3),(-5600,-10800,3.0),(-6700,-10850,3.0),(-7600,-10200,3.1),(-7900,-9250,3.5),(-6750,-8750,2.2)]):
 points.append((x,y,height(x,y)+10,scale,i in [0,3,6,10]))
# Fire also spreads among the distant village buildings, with fewer smoke systems.
for label,cls,tf in architecture:
 class_name=cls.rsplit('.',1)[-1]
 if 'House' not in class_name and 'Windmill' not in class_name:continue
 p=tf.translation
 if math.hypot(p.x+5900,p.y+9200)<1800:continue
 for dx,dy in [(-200,120),(230,-150)]:points.append((p.x+dx,p.y+dy,height(p.x+dx,p.y+dy)+25,2.8,False))
assert les.load_level(REAL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for i,(x,y,z,s,smoke) in enumerate(points):
 a=aes.spawn_actor_from_class(u.NiagaraActor,u.Vector(x,y,z));a.set_actor_label('Act2 village blaze %02d'%i);a.set_folder_path('Act2/Real fire');a.tags=['DC_Act2RealFire','DC_RealEnvironment'];a.set_actor_scale3d(u.Vector(s,s,s))
 c=a.get_component_by_class(u.NiagaraComponent);c.set_asset(u.load_asset('/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big'+('_Smoke' if smoke else '')));c.set_auto_activate(True);c.set_cast_shadow(False)
for i,(x,y,z,s,smoke) in enumerate(points[:12]):
 if i%3:continue
 a=aes.spawn_actor_from_class(u.PointLight,u.Vector(x,y,z+250));a.set_actor_label('Blaze warm bounce '+str(i));a.set_folder_path('Act2/Real fire');a.tags=['DC_RealEnvironment']
 c=a.get_component_by_class(u.PointLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_light_color(u.LinearColor(1,.23,.025,1));c.set_intensity(18000.);c.set_attenuation_radius(2200.);c.set_cast_shadows(False)
assert u.EditorLoadingAndSavingUtils.save_map(world,REAL)
assert les.load_level(MAIN)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for path,visible in [(SET,True),(REAL,False)]:
 layer=u.EditorLevelUtils.add_level_to_world(world,path,u.LevelStreamingDynamic);assert layer
 layer.set_editor_property('level_transform',u.Transform());layer.set_editor_property('initially_loaded',True);layer.set_editor_property('initially_visible',visible);layer.set_editor_property('should_be_loaded',True);layer.set_editor_property('should_be_visible',visible)
 u.EditorLevelUtils.set_level_visibility(layer.get_loaded_level(),visible,False)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act2_BrokenPromise')
d=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCZombieDirector));d.set_editor_property('studio_environment_level','L_DC_Act2_Set');d.set_editor_property('real_environment_level','L_DC_Act2_Real')
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(-6000,-10000,1900),u.Rotator(pitch=-25,yaw=85))
assert u.EditorLoadingAndSavingUtils.save_map(world,MAIN)
(ROOT/'Saved/Act2/dual_build.json').write_text(json.dumps({'main':MAIN,'studio':SET,'real':REAL,'fire_systems':len(points),'removed_prop_fire':True,'removed_showcase_actors':len(removed),'identity_alignment':True},indent=2));print('ACT2_DUAL_SCENE_BUILT',len(points))
