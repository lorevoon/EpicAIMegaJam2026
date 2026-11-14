"""Create the independent 38-second final stage. Run in Unreal Python commandlet."""
import unreal as u,json,math,warnings
from pathlib import Path
warnings.filterwarnings('ignore',category=DeprecationWarning)
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';DEST=BASE+'/Levels/L_DC_Act2_BrokenPromise'
at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary;aes=u.get_editor_subsystem(u.EditorActorSubsystem);les=u.get_editor_subsystem(u.LevelEditorSubsystem)
def load(path):
 a=u.load_asset(path);assert a,path;return a
def props(a,**kw):
 for k,v in kw.items():a.set_editor_property(k,v)
def imp(file,name,sk=None,mesh=False):
 folder=BASE+'/Act2';path=folder+'/'+name
 if eal.does_asset_exist(path):
  existing=load(path)
  if existing.get_editor_property('skeleton'):return existing
  assert eal.delete_asset(path) # Only a broken asset generated under /Act2 by this script.
 task=u.AssetImportTask();props(task,filename=str((ROOT/'SourceArt/Act2'/file).resolve()),destination_path=folder,destination_name=name,automated=True,save=True,replace_existing=False)
 ui=u.FbxImportUI();props(ui,import_mesh=mesh,import_as_skeletal=True,import_animations=not mesh,import_materials=False,import_textures=False,create_physics_asset=False,skeleton=sk,automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if mesh else u.FBXImportType.FBXIT_ANIMATION)
 ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task])
 for p in task.imported_object_paths:
  obj=load(p)
  if isinstance(obj,u.SkeletalMesh if mesh else u.AnimSequence):
   if mesh and obj.skeleton:assert eal.save_loaded_asset(obj.skeleton,False)
   if obj.get_name()!=name:assert eal.rename_asset(obj.get_path_name(),path)
   return load(path)
 raise RuntimeError(file)
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
meshes={r:load(BASE+'/Characters/'+r+'/SK_'+r) for r in ['SwordsmanAct1','ArcherAct1','OldMan','OrcArmored']}
if not meshes['OrcArmored'].skeleton:
 oldmesh=meshes['OrcArmored'];meshes['OrcArmored']=imp('../Polysplit/OrcArmored_Rigged.fbx','SK_Act2_Raider',mesh=True);meshes['OrcArmored'].materials=oldmesh.materials;eal.save_loaded_asset(meshes['OrcArmored'],False)
mourn=imp('A_Juliet_Mourning.fbx','A_Juliet_Mourning',meshes['ArcherAct1'].skeleton)
firemesh=imp('SK_Act2_Fire.fbx','SK_Act2_Flames',mesh=True);fireanim=imp('SK_Act2_Fire.fbx','A_Act2_Flames',firemesh.skeleton)
def material(name,color,emissive=False):
 path=BASE+'/Act2/'+name
 if eal.does_asset_exist(path):return load(path)
 m=at.create_asset(name,BASE+'/Act2',u.Material,u.MaterialFactoryNew());c=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(*color,1))
 u.MaterialEditingLibrary.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR if emissive else u.MaterialProperty.MP_BASE_COLOR)
 if emissive:m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
 u.MaterialEditingLibrary.recompile_material(m);eal.save_loaded_asset(m,False);return m
mats={'FlameOrange':material('M_FlameOrange',(5,.32,.008),True),'FlameGold':material('M_FlameGold',(7,2,.03),True),'Smoke':material('M_Smoke',(.055,.045,.04))}
slots=firemesh.materials
for i,s in enumerate(slots):s.material_interface=mats.get(str(s.material_slot_name),mats['FlameOrange']);slots[i]=s
firemesh.materials=slots;eal.save_loaded_asset(firemesh,False)
# Rebuild only this task's dedicated map from the untouched source environment.
assert les.load_level('/Game/WindmillValley/Levels/L_Showcase')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,DEST)
assert les.load_level(DEST);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for a in list(aes.get_all_level_actors()):
 if isinstance(a,(u.DCPerformer,u.DCWaypoint,u.DCBeat,u.DCZombieDirector,u.DCStartMark,u.DCClapperboard,u.PlayerStart)) or 'DC_Act1' in [str(t) for t in a.tags]:aes.destroy_actor(a)
actors=list(aes.get_all_level_actors());ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
 assert hit[0],(x,y);return hit[4].z+3
# Lane just downhill from the familiar original mill, away from its walls.
OX,OY=-5750,-9000
# Local positions are centimetres; terrain samples keep every route grounded.
def v(x,y,z=None):return u.Vector(OX+x,OY+y,height(OX+x,OY+y) if z is None else z)
def spawn(cls,label,p,yaw=0,folder='Act2/Action'):
 a=aes.spawn_actor_from_class(cls,p,u.Rotator(yaw=yaw));assert a;a.set_actor_label(label);a.set_folder_path(folder);a.tags=['DC_Act2'];return a
def static(label,path,p,scale=(1,1,1),mat=None):
 a=spawn(u.StaticMeshActor,label,p,folder='Act2/Set');a.static_mesh_component.set_static_mesh(load(path));a.set_actor_scale3d(u.Vector(*scale))
 if mat:a.static_mesh_component.set_material(0,mat)
 return a
# Re-retarget the source combat clips onto the new saved monster skeleton.
rigpath=BASE+'/Act2/IK_Act2_Raider';rtgpath=BASE+'/Act2/RTG_Act2_Raider'
rig=u.load_asset(rigpath) if eal.does_asset_exist(rigpath) else at.create_asset('IK_Act2_Raider',BASE+'/Act2',u.IKRigDefinition,u.IKRigDefinitionFactory())
rc=u.IKRigController.get_controller(rig);rc.set_skeletal_mesh(meshes['OrcArmored']);rc.set_retarget_root('pelvis_joint')
chains={'Spine':('waist_joint','chest_joint'),'Neck':('neck_joint','neck_joint'),'Head':('head_joint','head_joint'),'LeftClavicle':('L_clavicle_joint','L_clavicle_joint'),'RightClavicle':('R_clavicle_joint','R_clavicle_joint'),'LeftArm':('L_shoulder_joint','L_wrist_joint'),'RightArm':('R_shoulder_joint','R_wrist_joint'),'LeftLeg':('L_thigh_joint','L_ball_joint'),'RightLeg':('R_thigh_joint','R_ball_joint')}
for name,(start,end) in chains.items():rc.add_retarget_chain(name,start,end,'')

rtg=u.load_asset(rtgpath) if eal.does_asset_exist(rtgpath) else at.create_asset('RTG_Act2_Raider',BASE+'/Act2',u.IKRetargeter,u.IKRetargetFactory())
rr=u.IKRetargeterController.get_controller(rtg);src=load(BASE+'/Mixamo/Retarget/IK_Mixamo');rr.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,src);rr.set_ik_rig(u.RetargetSourceOrTarget.TARGET,rig);rr.add_default_ops();rr.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.SOURCE,src);rr.assign_ik_rig_to_all_ops(u.RetargetSourceOrTarget.TARGET,rig);rr.auto_map_chains(u.AutoMapChainType.EXACT,True)

for obj in [rig,rtg]:eal.save_loaded_asset(obj,False)
def anim(role,name):
 if role!='OrcArmored':return load(BASE+'/Animations/'+role+'/A_'+role+'_'+name)
 path=BASE+'/Act2/A_Raider_'+name
 if eal.does_asset_exist(path):return load(path)
 source=load(BASE+'/Mixamo/SupportClips/A_'+name)
 result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(source)],load(BASE+'/Mixamo/SK_XBot'),meshes['OrcArmored'],rtg,'A_Support_','A_Raider_Support_','','',BASE+'/Act2',False,False,True)
 assert len(result)==1,name
 obj=load(str(result[0].package_name));eal.save_loaded_asset(obj,False);return obj

def cue(t,a,loop=True,rate=1):
 c=u.DCAnimationCue();props(c,start_time=t,animation=a,loop=loop,play_rate=rate);return c
def performer(label,role,x,y,yaw,cues):
 a=spawn(u.DCPerformer,label,v(x,y),yaw);props(a,display_name=label,character_mesh=meshes[role],mesh_offset=u.Transform(rotation=u.Rotator(yaw=-90)),head_bone='head_joint',body_bone='chest_joint',use_waypoint_height=True,linear_movement=True,animation_cues=cues);return a
def route(a,points):
 for t,x,y,yaw in points:
  w=spawn(u.DCWaypoint,a.get_actor_label()+' @ '+str(t),v(x,y),yaw,'Act2/Blocking');props(w,performer=a,time=float(t))
S=lambda n:anim('Swordsman',n);O=lambda n:anim('OrcArmored',n);A=lambda n:anim('Archer',n)
smith=performer('Mr. Smith','OldMan',0,0,0,[cue(0,S('PromiseLook')),cue(1,S('Support_LookAround')),cue(7,S('Support_DeathBackward'),False,1.6)])
romeo=performer('Romeo','SwordsmanAct1',-550,-200,0,[cue(0,S('Unarmed_Idle')),cue(8,S('Support_Run')),cue(11,S('Support_DrawWeapon'),False),cue(12,S('Support_MeleeSlash'),False),cue(14.4,S('Support_Block'),False),cue(16.3,S('Support_MeleeBackhand'),False),cue(18.6,S('Support_BlockHit'),False),cue(20.5,S('Support_Idle')),cue(22,S('Support_MeleeDownward'),False),cue(24,S('Support_DeathBackward'),False,1.5)])
juliet=performer('Juliet','ArcherAct1',-640,-320,0,[cue(0,A('PromiseLook')),cue(8,A('Support_Run')),cue(11,A('Support_LookAround')),cue(27.5,A('Support_Run')),cue(29,mourn,False)])
monster=performer('Armoured monster','OrcArmored',450,40,180,[cue(0,O('Support_Walk')),cue(5.5,O('Support_MeleeDownward'),False),cue(8,O('Support_Idle')),cue(12,O('Support_Block'),False),cue(14.3,O('Support_MeleeSlash'),False),cue(16.3,O('Support_BlockHit'),False),cue(18.5,O('Support_MeleeBackhand'),False),cue(20.8,O('Support_Idle')),cue(23,O('Support_MeleeDownward'),False),cue(25.3,O('Support_Idle')),cue(27.2,O('Support_Walk'))])
route(monster,[(5,140,0,180),(8,140,0,180),(11,140,-180,180),(22,140,-180,180),(27.2,140,-180,180),(28,210,-150,25),(34,750,180,25),(38,1000,350,25)])
route(romeo,[(8,-550,-200,0),(11,-20,-180,0),(24,-20,-180,0),(38,-20,-180,0)])
route(juliet,[(8,-640,-320,0),(11,-200,-350,20),(27.5,-200,-350,20),(29,-180,-240,0),(38,-180,-240,0)])
route(smith,[(38,0,0,0)])
wood=load('/Game/Stylized_Village/Assets/Architecture/Wood/_Materials/MI_Wood_PiecesPlanks');cube='/Engine/BasicShapes/Cube'
z=height(OX,OY+100)
static('Mr. Smith - familiar bench',cube,v(0,100,z+46),(1.7,.48,.12),wood)
for x in [-60,60]:static('Bench leg',cube,v(x,100,z+22),(.12,.4,.44),wood)
static('Mr. Smith - marked box','/Game/Stylized_Village/Assets/Props/Furniture/SM_Crate_01',v(-90,40),(.4,.4,.4))
fire=spawn(u.DCPerformer,'Fire - ignition 0.6 seconds',v(60,180),folder='Act2/Effects');props(fire,display_name='Fire',character_mesh=firemesh,animation_cues=[cue(0,fireanim,False)],head_bone='fx_0',body_bone='fx_0',use_waypoint_height=True)
# Reference smoke lies behind the action; animated smoke is part of the fire rig.
fire.get_component_by_class(u.SkeletalMeshComponent).set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
# Hand-held swords are components attached to animated wrists, not floating world props.
metal=material('M_Blade',(.44,.49,.56));hilt=material('M_Hilt',(.10,.045,.015))
for a,length in [(romeo,90),(monster,115)]:
 c=a.get_component_by_class(u.SkeletalMeshComponent)
 for label,scale,offset,mat in [('Blade',(.065,.035,length/100),(0,0,length*.5),metal),('Guard',(.32,.06,.06),(0,0,0),hilt),('Grip',(.065,.065,.18),(0,0,-10),hilt)]:
  weapon=static(a.get_actor_label()+' '+label,cube,u.Vector(),scale,mat)
  weapon.static_mesh_component.set_mobility(u.ComponentMobility.MOVABLE)
  weapon.attach_to_component(c,'R_wrist_joint',u.AttachmentRule.SNAP_TO_TARGET,u.AttachmentRule.SNAP_TO_TARGET,u.AttachmentRule.KEEP_WORLD,False)
  weapon.set_actor_relative_location(u.Vector(*offset),False,False)
  weapon.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
cast=[smith,romeo,juliet,monster,fire]
for a in cast:a.evaluate(0)
beatdata=[('2.1','Peace catches fire',0,5,3,1,[fire,smith]),('2.2','The hand falls',5,12,9,1,[monster,smith]),('2.3','Face the killer',12,22,16,1,[romeo,monster]),('2.4','The broken promise',22,29,26,1,[romeo,monster]),('2.5','You promised',29,38,34,2,[juliet,romeo])]
captions=['Fire starts beside Mr. Smith. A monster emerges through the smoke.','The monster kills Mr. Smith beside his box. Romeo arrives too late.','Romeo confronts the killer while Juliet watches.','The monster delivers a fatal blow. Romeo falls and never rises.','Juliet cradles Romeo and sobs: You promised...']
for i,(id,title,start,end,ready,hold,targets) in enumerate(beatdata):
 b=spawn(u.DCBeat,id+' '+title,v(-250,-500),folder='Act2/Story beats');refs=[]
 for a in targets:
  ref=u.DCAnchorRef();props(ref,performer=a,part=u.DCAnchorPart.HEAD if i==4 else u.DCAnchorPart.BODY,must_see_face=False);refs.append(ref)
 props(b,beat_id=id,display_name=title,caption=captions[i],window_start=float(start),window_end=float(end),pose_ready_time=float(ready),required_hold=float(hold),max_distance=3000.,anchors=refs)
d=spawn(u.DCZombieDirector,'Stage 2 - The Raid / Broken Promise - 38 seconds',v(-400,-500),folder='Act2/Direction');props(d,take_duration=38.,movie_title='The Raid / Broken Promise - Stage 2',max_cameras=0)
spawn(u.DCStartMark,'Stage 2 camera operator',v(-350,-850,height(OX-350,OY-850)+100),55,'Act2/Direction')
spawn(u.DCClapperboard,'Stage 2 clapperboard',v(-300,-700),0,'Act2/Direction')
world.get_world_settings().set_editor_property('default_game_mode',u.DCZombieGameMode)
# Sample final sockets to align the grieving couple without moving the corpse.
for a in cast:a.evaluate(35)
report={'level':DEST,'duration':38,'beats':[{ 'id':b[0],'start':b[2],'end':b[3],'ready':b[4],'hold':b[5]} for b in beatdata],'cast':{}}
for a in cast:
 c=a.get_component_by_class(u.SkeletalMeshComponent)
 report['cast'][a.get_actor_label()]={'location':list(a.get_actor_location().to_tuple()),'head':list(c.get_socket_location(a.get_editor_property('head_bone')).to_tuple()),'bones':{n:list(c.get_socket_location(n).to_tuple()) for n in ['head_joint','L_wrist_joint','R_wrist_joint','L_ankle_joint','R_ankle_joint']}}
for a in cast:a.evaluate(0)
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(v(-450,-1100,height(OX,OY)+480),u.Rotator(pitch=-18,yaw=65))
assert u.EditorLoadingAndSavingUtils.save_map(world,DEST)
(ROOT/'Saved/Act2').mkdir(exist_ok=True);(ROOT/'Saved/Act2/build_report.json').write_text(json.dumps(report,indent=2))
print('ACT2_BUILD_COMPLETE',DEST)

