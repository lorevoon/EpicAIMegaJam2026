import unreal as u,json,math,shutil
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/KissRevision12';O=R/'Saved/KissRevision12';O.mkdir(parents=True,exist_ok=True);e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
backup=O/'before.umap'
if not backup.exists():shutil.copy2(R/'Content/DirectorsCut/Levels/L_DC_Act1_Promise.umap',backup)
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-v for v in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
mix=u.load_asset(B+'/Mixamo/SK_XBot')
for who,role,sex in [('Juliet','Archer','Male'),('Romeo','Swordsman','Female')]:
 source=u.load_asset(B+'/Animations/PerformanceRevision11/A_Mixamo_'+sex+'Kiss');dest=F+'/A_'+role+'_'+sex+'Kiss';mesh=u.load_asset(B+'/Characters/'+role+'/SK_'+role);rtg=u.load_asset(B+'/Mixamo/SupportRetarget/RTG_Support_'+role)
 if not e.does_asset_exist(dest):
  result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(source)],mix,mesh,rtg,'A_Mixamo_','A_'+role+'_','','',F,False,False,True);assert len(result)==1
  if str(result[0].package_name)!=dest:assert e.rename_asset(str(result[0].package_name),dest)
 assert e.save_loaded_asset(u.load_asset(dest),False)
assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};report={}
for who,role,sex in [('Juliet','Archer','Male'),('Romeo','Swordsman','Female')]:
 a=cast[who];source=u.load_asset(B+'/Animations/PerformanceRevision11/A_Act1_'+who);mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];kiss=u.load_asset(F+'/A_'+role+'_'+sex+'Kiss');data={b:[[],[],[]] for b in bones}
 for f in range(601):
  t=f/30;original=list(u.AnimationLibrary.get_bone_poses_for_time(source,bones,t,False,mesh));p=original
  if t>16:
   raw=list(u.AnimationLibrary.get_bone_poses_for_time(kiss,bones,t-16,False,mesh));raw[0].translation=u.Vector(original[0].translation.x,original[0].translation.y,raw[0].translation.z);w=smooth((t-16)/.35);p=[blend(x,y,w) for x,y in zip(original,raw)]
   for i,b in enumerate(bones):
    if b in ['mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint']:p[i]=original[i]
  for b,tr in zip(bones,p):data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Act1_'+who;anim=u.load_asset(dest) or e.duplicate_asset(source.get_path_name(),dest);c=anim.get_editor_property('controller');c.open_bracket('Swapped kiss roles without forced spine fitting',False)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
 c.close_bracket(False);e.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);report[who]={'clip':kiss.get_path_name()}
# Move final marks apart by 8 cm each, easing the adjustment before the kiss.
routefile=O/'original_marks.json'
if not routefile.exists():routefile.write_text(json.dumps({w.get_actor_label():list(w.get_actor_location().to_tuple()) for w in actors if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer') in [cast['Romeo'],cast['Juliet']]},indent=2))
routes=json.loads(routefile.read_text());ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
for w in actors:
 if isinstance(w,u.DCWaypoint) and w.get_actor_label() in routes:
  t=w.get_editor_property('time');p=u.Vector(*routes[w.get_actor_label()]);p.y+=(8 if w.get_editor_property('performer')==cast['Juliet'] else -8)*smooth((t-14)/2)
  if t>14:
   h=u.SystemLibrary.line_trace_single(world,u.Vector(p.x,p.y,6000),u.Vector(p.x,p.y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True)
   if h:p.z=h.to_tuple()[4].z+2
  w.set_actor_location(p,False,False)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise')
assert les.load_level(B+'/Levels/L_DC_Act2_BrokenPromise');assert les.load_level(B+'/Levels/L_DC_Act1_Promise');cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)};report['samples']=[]
for t in [16,16.5,17,17.5,18,19,20]:
 row={'time':t};points=[]
 for who in ['Romeo','Juliet']:
  a=cast[who];a.evaluate(t);c=a.get_component_by_class(u.SkeletalMeshComponent);row[who]={b:list(c.get_socket_location(b).to_tuple()) for b in ['mouth_joint','head_joint','pelvis_joint','L_ankle_joint','R_ankle_joint']};points.append(c.get_socket_location('mouth_joint'))
 row['mouth_distance_cm']=(points[0]-points[1]).length();report['samples'].append(row)
(O/'report.json').write_text(json.dumps(report,indent=2));print('KISS12_SAVED',[(r['time'],r['mouth_distance_cm']) for r in report['samples']])
