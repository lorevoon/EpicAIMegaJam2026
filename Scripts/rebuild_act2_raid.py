"""Re-import full cast meshes and stage the continuous 38-second orc raid."""
import unreal as u,json,math,warnings
from pathlib import Path
warnings.filterwarnings('ignore',category=DeprecationWarning)
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';FOLDER=BASE+'/Act2';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FPS=30
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
def load(p):
 a=u.load_asset(p);assert a,p;return a
def props(o,**kw):
 for k,v in kw.items():o.set_editor_property(k,v)
def imp(file,name,sk,mesh=False):
 path=FOLDER+'/'+name
 if eal.does_asset_exist(path):return load(path)
 task=u.AssetImportTask();props(task,filename=str(ROOT/'SourceArt/Act2'/file),destination_path=FOLDER,destination_name=name,automated=True,save=True,replace_existing=False)
 ui=u.FbxImportUI();props(ui,import_mesh=mesh,import_as_skeletal=True,import_animations=not mesh,import_materials=False,import_textures=False,create_physics_asset=False,skeleton=sk,automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if mesh else u.FBXImportType.FBXIT_ANIMATION)
 ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task]);return load(path)
orc_old=load(FOLDER+'/SK_Act2_Raider');romeo_old=load(BASE+'/Characters/SwordsmanAct1/SK_SwordsmanAct1');juliet_mesh=load(BASE+'/Characters/ArcherAct1/SK_ArcherAct1')
orc_mesh=imp('Orc_Complete_Raid.fbx','SK_Act2_OrcComplete',orc_old.skeleton,True);romeo_mesh=imp('Romeo_SourceSword.fbx','SK_Act2_RomeoArmed',romeo_old.skeleton,True)
for mesh,role,old in [(orc_mesh,'Orc',orc_old),(romeo_mesh,'Hero',romeo_old)]:
 oldslots={str(s.material_slot_name):s.material_interface for s in old.materials};slots=mesh.materials
 for i,slot in enumerate(slots):
  n=str(slot.material_slot_name);kind='Body' if 'Body' in n else 'Objects'
  mat=oldslots.get(n) or load(BASE+'/Materials/MI_Polysplit_'+role+'_'+kind)
  if role=='Orc':
   path=FOLDER+'/MI_Act2_OrcOpaque_'+kind
   mat=load(path) if eal.does_asset_exist(path) else eal.duplicate_asset(mat.get_path_name(),path)
   override=u.MaterialInstanceBasePropertyOverrides();props(override,override_blend_mode=True,blend_mode=u.BlendMode.BLEND_OPAQUE,override_two_sided=True,two_sided=True);mat.set_editor_property('base_property_overrides',override)
   # UE5.8 instances need their own persisted skeletal shader usage after base overrides.
   usage=getattr(u.MaterialUsage,next(n for n in dir(u.MaterialUsage) if 'SKELETAL' in n))
   u.MaterialEditingLibrary.set_material_usage_override(mat,usage,True,True);u.MaterialEditingLibrary.update_material_instance(mat)
   assert eal.save_loaded_asset(mat,False)
  slot.material_interface=mat;slots[i]=slot
 mesh.materials=slots;assert mesh.skeleton;assert eal.save_loaded_asset(mesh,False)
romeo_gasp=imp('A_Romeo_Gasp.fbx','A_Raid_RomeoGasp',romeo_mesh.skeleton);juliet_gasp=imp('A_Juliet_Gasp.fbx','A_Raid_JulietGasp',juliet_mesh.skeleton)
# Retarget the same supported death motion to the complete orc's compatible rig.
deathpath=FOLDER+'/A_Raid_OrcDeath'
if not eal.does_asset_exist(deathpath):
 result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(load(BASE+'/Mixamo/SupportClips/A_Support_DeathBackward'))],load(BASE+'/Mixamo/SK_XBot'),orc_mesh,load(FOLDER+'/RTG_Act2_Raider'),'A_Support_','A_Raid_','','',FOLDER,False,False,True)
 assert len(result)==1
 obj=load(str(result[0].package_name));assert eal.rename_asset(obj.get_path_name(),deathpath)
orc_death=load(deathpath)
# Read the actual end of Act 1, including the practical tree shared by set and real versions.
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');oldactors=list(aes.get_all_level_actors());treeinfo=None
for a in oldactors:
 if a.get_actor_label()=='Practical promise tree' and isinstance(a,u.StaticMeshActor):treeinfo=(a.static_mesh_component.static_mesh,a.get_actor_transform(),[a.static_mesh_component.get_material(i) for i in range(a.static_mesh_component.get_num_materials())])
continuity=json.loads((ROOT/'Saved/Act2/act1_continuity.json').read_text())['cast']
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());by={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
assert set(by)=={'Mr. Smith','Romeo','Juliet','Armoured monster'},set(by)
# Remove only obsolete custom wrist blocks and routes from this Act 2 staging.
for a in actors:
 if isinstance(a,u.DCWaypoint) or a.get_actor_label() in [who+' '+part for who in ['Romeo','Armoured monster'] for part in ['Blade','Guard','Grip']]:aes.destroy_actor(a)
ignore=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple();assert h[0],(x,y);return h[4].z+3

def pos(x,y):return u.Vector(x,y,height(x,y))
def cue(t,a,loop=True,rate=1,offset=0):
 q=u.DCAnimationCue();props(q,start_time=float(t),animation=a,loop=loop,play_rate=float(rate),start_offset=float(offset));return q
S=lambda n:load(BASE+'/Animations/Swordsman/A_Swordsman_'+n)
J=lambda n:load(BASE+'/Animations/Archer/A_Archer_'+n)
O=lambda n:load(FOLDER+'/A_Raider_'+n)
def config(a,mesh,p,yaw,cues):
 a.set_actor_location(p,False,False);a.set_actor_rotation(u.Rotator(yaw=yaw),False);props(a,character_mesh=mesh,mesh_offset=u.Transform(rotation=u.Rotator(yaw=-90)),head_bone='head_joint',body_bone='chest_joint',use_waypoint_height=True,linear_movement=True,animation_cues=cues)
 c=a.get_component_by_class(u.SkeletalMeshComponent);c.set_editor_property('override_materials',[])
def route(a,points):
 # Frequent terrain samples prevent characters floating across the sloping lane.
 expanded=[];previous=(0,a.get_actor_location().x,a.get_actor_location().y,a.get_actor_rotation().yaw)
 for end in points:
  start=previous;dist=math.hypot(end[1]-start[1],end[2]-start[2]);steps=max(1,math.ceil(dist/100))
  for i in range(1,steps+1):
   f=i/steps;dyaw=(end[3]-start[3]+180)%360-180;expanded.append((start[0]+(end[0]-start[0])*f,start[1]+(end[1]-start[1])*f,start[2]+(end[2]-start[2])*f,start[3]+dyaw*f))
  previous=end
 for t,x,y,yaw in expanded:
  w=aes.spawn_actor_from_class(u.DCWaypoint,pos(x,y),u.Rotator(yaw=yaw));w.set_actor_label(a.get_actor_label()+' @ '+str(round(t,3)));w.set_folder_path('Act2/Blocking');props(w,performer=a,time=float(t));w.set_is_temporarily_hidden_in_editor(True)
smith=by['Mr. Smith'];romeo=by['Romeo'];juliet=by['Juliet'];orc=by['Armoured monster']
config(smith,smith.get_editor_property('character_mesh'),pos(-5750,-9000),210,[cue(0,S('PromiseLook')),cue(5,S('Support_LookAround')),cue(7.2,S('Support_DeathBackward'),False,1.6)])
config(romeo,romeo_mesh,u.Vector(*continuity['Romeo']['position']),90,[cue(0,S('PromiseLook')),cue(10,romeo_gasp,False),cue(13,S('Support_Run')),cue(17.5,S('Support_DrawWeapon'),False),cue(19,S('Support_MeleeSlash'),False),cue(21,S('Support_BlockHit'),False),cue(23,S('Support_MeleeBackhand'),False),cue(24.4,S('Support_BlockHit'),False),cue(25.1,S('Support_MeleeDownward'),False),cue(26.3,S('Support_DeathBackward'),False,1.5)])
config(juliet,juliet_mesh,u.Vector(*continuity['Juliet']['position']),-90,[cue(0,J('PromiseLook')),cue(10,juliet_gasp,False),cue(13,J('Support_Run')),cue(17,J('Support_LookAround')),cue(29,J('Support_Run')),cue(31,load(FOLDER+'/A_Juliet_Mourning'),False)])
config(orc,orc_mesh,pos(-6900,-9800),32,[cue(0,O('Support_Walk'),True,1.2),cue(6.3,O('Support_MeleeDownward'),False),cue(8.5,O('Support_Idle')),cue(16.5,O('Support_Walk')),cue(19,O('Support_Block'),False),cue(20.8,O('Support_MeleeSlash'),False),cue(23,O('Support_BlockHit'),False),cue(24.3,O('Support_MeleeDownward'),False),cue(25.8,orc_death,False,1.6)])
route(smith,[(38,-5750,-9000,210)])
route(orc,[(4,-6400,-9500,32),(6.2,-5900,-9085,30),(16.5,-5900,-9085,30),(18.5,-5940,-9300,30),(38,-5940,-9300,30)])
route(romeo,[(6,-5000,-8824.5,90),(9.5,-5050,-8880,210),(13,-5050,-8880,210),(17.5,-5790,-9210,210),(38,-5790,-9210,210)])
route(juliet,[(6,-5000,-8790.5,-90),(9.5,-4920,-8720,210),(13,-4920,-8720,210),(17,-5370,-8950,210),(29,-5370,-8950,210),(31,-5585,-9090,210),(38,-5585,-9090,210)])
# Four raiders flank their leader, then break toward the rest of the village.
troop=[]
for i,(start,end,raid) in enumerate([((-7200,-9960),(-6500,-9660),(-6800,-9250)),((-7140,-9720),(-6560,-9420),(-7000,-9000)),((-6920,-10070),(-6220,-9790),(-5350,-10100)),((-7370,-10060),(-6670,-9800),(-5900,-10200))]):
 a=aes.spawn_actor_from_class(u.DCPerformer,pos(*start),u.Rotator(yaw=32));a.set_actor_label('Raid orc '+str(i+1));a.set_folder_path('Act2/Raid troop');a.tags=['DC_Act2','DC_RaidTroop'];config(a,orc_mesh,pos(*start),32,[cue(0,O('Support_Walk'),True,1.0+i*.05,offset=i*.17),cue(12,O('Support_Walk'),True,1.1),cue(20,O('Support_Idle'))]);route(a,[(6,*end,32),(9,*end,32),(20,*raid,30 if i<2 else -30),(38,*raid,30 if i<2 else -30)]);troop.append(a)
# Familiar tree remains in both sets, at the exact Act 1 transform.
if treeinfo and not any(a.get_actor_label()=='Practical promise tree' for a in aes.get_all_level_actors()):
 mesh,tf,mats=treeinfo;a=aes.spawn_actor_from_class(u.StaticMeshActor,tf.translation,tf.rotation.rotator());a.set_actor_transform(tf,False,False);a.set_actor_label('Practical promise tree');a.set_folder_path('Act2/Set');a.static_mesh_component.set_static_mesh(mesh)
 for i,m in enumerate(mats):a.static_mesh_component.set_material(i,m)
# Reframe the five storyboard beats around the revised causal sequence.
beats=sorted([a for a in aes.get_all_level_actors() if isinstance(a,u.DCBeat)],key=lambda a:str(a.get_editor_property('beat_id')))
data=[('The orc raid',0,6,3,1,[orc,troop[0],troop[3]],'A troop of orcs emerges through towering fire and marches into the village.',(-6000,-9300)),('Mr. Smith falls',6,10,8.5,1,[orc,smith],'One raider reaches Mr. Smith and kills him.',(-5550,-9650)),('They saw everything',10,19,11,1,[romeo,juliet],'Under their promise tree, Romeo and Juliet gasp. Romeo rushes to Mr. Smith.',(-4550,-9250)),('A fatal victory',19,29,27.5,1,[romeo,orc],'Romeo fights the killer. His final strike kills the orc, but Romeo dies of his wounds.',(-5550,-9800)),('You promised',29,38,34.5,2,[juliet,romeo],'Juliet reaches Romeo, kneels over him, and sobs.',(-5300,-9500))]
for b,(title,start,end,ready,hold,targets,caption,spot) in zip(beats,data):
 refs=[]
 for a in targets:
  r=u.DCAnchorRef();props(r,performer=a,part=u.DCAnchorPart.HEAD if start>=29 else u.DCAnchorPart.BODY,must_see_face=False);refs.append(r)
 props(b,display_name=title,caption=caption,window_start=float(start),window_end=float(end),pose_ready_time=float(ready),required_hold=float(hold),anchors=refs,show_camera_spot=True,camera_spot=b.get_actor_transform().inverse_transform_location(pos(*spot)))
 b.set_actor_label(str(b.get_editor_property('beat_id'))+' '+title)
# Save routes BEFORE any evaluation, then reload to rebuild all runtime key caches.
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)]
# Bake smooth cue transitions once, retaining deterministic forward/reverse playback.
code=(ROOT/'Scripts/polish_act2_stage.py').read_text();code=code[code.index('def smooth'):code.index('# Bake actor motion')].replace("name='A_Act2_'","name='A_Raid_'").replace("(a.get_actor_label()=='Romeo' and t>25) or (a.get_actor_label()=='Mr. Smith' and t>8.5)","(a.get_actor_label()=='Romeo' and t>27.8) or (a.get_actor_label()=='Mr. Smith' and t>8.7) or (a.get_actor_label()=='Armoured monster' and t>27.3)")
exec(code)
for a in cast:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(ROOT/'Saved/Act2/raid_build.json').write_text(json.dumps({'duration':38,'troop_count':5,'cast_count':len(cast),'act1_start_positions':continuity,'tree_copied':bool(treeinfo),'mutual_death':True,'source_sword_bind':True},indent=2));print('ACT2_RAID_REBUILT')
