"""Bake deterministic Act 1 facial acting on the existing rigs and align the kiss.
Original clips and source meshes remain untouched. Re-running reads the saved cue audit.
"""
import unreal as u, json, math, warnings
from pathlib import Path
warnings.filterwarnings('ignore',category=DeprecationWarning)
root=Path(__file__).resolve().parents[1];base='/Game/DirectorsCut';fps=30;frames=601
audit=json.loads((root/'SourceArt/Act1Faces/blocking_source.json').read_text())
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert 'L_DC_Act1_Promise' in str(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name())
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
ground=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Ground')
ground_visible=ground.get_editor_property('should_be_visible')
u.EditorLevelUtils.set_level_visibility(ground.get_loaded_level(),True,False)
cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label() in audit}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True)
 assert hit,(x,y)
 return hit.to_tuple()[4].z+2
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def keyed(t,keys):
 for (ta,a),(tb,b) in zip(keys,keys[1:]):
  if t<=tb:return a+(b-a)*smooth((t-ta)/(tb-ta))
 return keys[-1][1]
def expressions(who,t):
 if who=='Mr. Smith':
  smile=keyed(t,[(0,.15),(2,.25),(3.5,.9),(5,.65),(6.5,1),(8.2,1),(9,.7),(20,.7)])
  shut=.28*smile;brow=.65*smile;kiss=0
 else:
  smile=keyed(t,[(0,.3 if who=='Romeo' else .2),(2.4,.2),(4,.8),(6,1),(7.5,.55),(12.5,.65),(15,.8),(16,.5),(17,0),(20,0)])
  kiss=smooth((t-16.2)/1.1);shut=max(.14*smile,kiss);brow=.4*smile
 blink_times=[1.7,5.4,9.6,14.2] if who=='Romeo' else [2.2,5.9,10.3,14.8] if who=='Juliet' else [1.2,4.8,10.8,15.5]
 for bt in blink_times:shut=max(shut,max(0,1-abs(t-bt)/.12))
 return smile,shut,brow,kiss

def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v))
 tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
root_cache={}
def raw(cue,at,bones,mesh):
 asset=cue['asset'];poses=u.AnimationLibrary.get_bone_poses_for_time(asset,bones,at,False,mesh)
 if asset.get_editor_property('force_root_lock'):
  key=asset.get_path_name()
  if key not in root_cache:root_cache[key]=u.AnimationLibrary.get_bone_poses_for_time(asset,[bones[0]],0,False,mesh)[0]
  poses[0]=root_cache[key]
 return poses
def sample(cue,t,bones,mesh):
 length=cue['asset'].get_play_length();at=max(0,cue['offset']+(t-cue['time'])*cue['rate'])
 at=at%length if cue['loop'] else min(length,at)
 poses=raw(cue,at,bones,mesh)
 # Close imperfect source loop seams continuously without accumulating root travel.
 if cue['loop'] and at>length-.18:
  end=raw(cue,0,bones,mesh);w=smooth((at-(length-.18))/.18)
  poses=[blend(a,b,w) for a,b in zip(poses,end)]
 return poses
clips={};tracks={};report={}
for who,performer in cast.items():
 mesh=performer.get_editor_property('character_mesh');bones=audit[who]['bones'];name='A_Act1_'+who.replace(' ','').replace('.','')+'_FacePerformance'
 factory=u.AnimSequenceFactory();factory.target_skeleton=mesh.skeleton;factory.preview_skeletal_mesh=mesh
 path=base+'/Animations/Act1Faces/'+name
 anim=u.load_asset(path) or u.AssetToolsHelpers.get_asset_tools().create_asset(name,base+'/Animations/Act1Faces',u.AnimSequence,factory)
 controller=anim.get_editor_property('controller');controller.open_bracket('Act 1 facial performance',False)
 controller.set_frame_rate(u.FrameRate(fps,1),False);controller.set_number_of_frames(u.FrameNumber(frames-1),False)
 controller.remove_all_bone_tracks(False)
 for bone in bones:controller.add_bone_track(bone,False)
 data={bone:[[],[],[]] for bone in bones};smiles=[]
 cues=[dict(c,asset=u.load_asset(c['animation'])) for c in audit[who]['cues']]
 for c in cues:
  if 'WalkForward' in c['animation']:
   c['rate']=1.75 if c['time']==7 else 1.3
   if who=='Juliet':
    c['animation']=base+'/Animations/Archer/A_Archer_FemaleWalk'
    c['asset']=u.load_asset(c['animation']);assert c['asset']
    c['rate']=1.25 if c['time']==7 else 1.0
  if c['time'] in [12,12.5]:c['time']=11.6
  if who=='Juliet' and c['time']==6:c['time']=5.6
 for frame in range(frames):
  t=frame/fps;cue=max([c for c in cues if c['time']<=t+1e-5],key=lambda c:c['time'])
  length=cue['asset'].get_play_length();at=max(0,cue['offset']+(t-cue['time'])*cue['rate']);at=at%length if cue['loop'] else min(length,at)
  poses=sample(cue,t,bones,mesh)
  index=cues.index(cue)
  if index and t-cue['time']<.4:
   old=sample(cues[index-1],t,bones,mesh)
   poses=[blend(a,b,smooth((t-cue['time'])/.4)) for a,b in zip(old,poses)]
  smile,closed,brow,kiss=expressions(who,t);smiles.append(smile)
  for bone,tr in zip(bones,poses):
   if bone=='eyes_joint':tr.scale3d=u.Vector(1,max(.055,1-closed*.945),1)
   if bone=='mouth_joint':tr.scale3d=u.Vector(1-.18*kiss,1-.15*kiss,1)
   if bone in ['L_eyebrow_joint','R_eyebrow_joint']:
    # Reference facial parent X is vertical; a subtle lift softens the face.
    p=tr.translation;p.x+=.7*brow;tr.translation=p
   data[bone][0].append(tr.translation);data[bone][1].append(tr.rotation);data[bone][2].append(tr.scale3d)
 for bone,(positions,rotations,scales) in data.items():assert controller.set_bone_track_keys(bone,positions,rotations,scales,False),bone
 controller.close_bracket(False)
 if u.AnimationLibrary.does_curve_exist(anim,'FaceSmile',u.RawCurveTrackTypes.RCT_FLOAT):u.AnimationLibrary.remove_curve(anim,'FaceSmile')
 u.AnimationLibrary.add_curve(anim,'FaceSmile');u.AnimationLibrary.add_float_curve_keys(anim,'FaceSmile',[f/fps for f in range(frames)],smiles)
 u.AnimationLibrary.add_curve_meta_data(mesh.skeleton,'FaceSmile',False);u.AnimationLibrary.set_curve_meta_data_material(mesh.skeleton,'FaceSmile',True)
 cue=u.DCAnimationCue();cue.start_time=0;cue.animation=anim;cue.play_rate=1;cue.loop=False
 performer.set_editor_property('animation_cues',[cue]);clips[who]=anim;tracks[who]=data
 assert u.EditorAssetLibrary.save_loaded_asset(anim,False)
 assert u.EditorAssetLibrary.save_loaded_asset(mesh.skeleton,False)
 report[who]={'animation':path,'frames':frames}
 u.log('FACE_PERFORMANCE_BAKED '+who)
# Both actors keep planted feet. Small head translations bring the rigged mouth
# anchors to one shared contact, including the terrain height difference.
max_correction=0
for frame in range(16*fps,frames):
 t=frame/fps
 for who in ['Romeo','Juliet']:cast[who].evaluate(t)
 comps={who:cast[who].get_component_by_class(u.SkeletalMeshComponent) for who in ['Romeo','Juliet']}
 points={who:c.get_socket_location('mouth_joint') for who,c in comps.items()};target=(points['Romeo']+points['Juliet'])*.5
 weight=smooth((t-16)/1.5)
 for who,c in comps.items():
  delta=(target-points[who])*weight;max_correction=max(max_correction,delta.length())
  parent=c.get_parent_bone('head_joint');tr=c.get_socket_transform(parent,u.RelativeTransformSpace.RTS_WORLD)
  local=tr.inverse_transform_direction(delta)
  tracks[who]['head_joint'][0][frame]+=local
for who in ['Romeo','Juliet']:
 controller=clips[who].get_editor_property('controller');controller.open_bracket('Paired lip contact',False)
 assert controller.set_bone_track_keys('head_joint',*tracks[who]['head_joint'],False)
 controller.close_bracket(False);assert u.EditorAssetLibrary.save_loaded_asset(clips[who],False)
for performer in cast.values():performer.evaluate(0)
u.EditorLevelUtils.set_level_visibility(ground.get_loaded_level(),ground_visible,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,base+'/Levels/L_DC_Act1_Promise')
report['max_head_correction_cm']=max_correction
(root/'Saved/FilmSetAudit/face_authoring_report.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_FACE_CONTACT_SAVED')

