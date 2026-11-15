import unreal as u,json,math,shutil
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/Animations/Act1Revision14';O=R/'Saved/Act1Revision14';e=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
if not (O/'before.umap').exists():shutil.copy2(R/'Content/DirectorsCut/Levels/L_DC_Act1_Promise.umap',O/'before.umap')
# Shared blend helper, without running earlier authoring.
s=(R/'Scripts/author_kiss13.py').read_text();exec(s[s.index('def smooth'):s.index('assert les.load_level')])
assert les.load_level(B+'/Levels/L_DC_Act1_Promise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)};inspection=json.loads((O/'inspection.json').read_text());timing=json.loads((R/'SourceArt/Act1Dialogue/timing.json').read_text());line=next(x for x in timing['lines'] if x['name']=='LetMeHelp');report={}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True);assert h,(x,y);return h.to_tuple()[4].z+2
for who,role,sex in [('Romeo','Swordsman','Male'),('Juliet','Archer','Female')]:
 a=cast[who];mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];source=u.load_asset(B+'/Animations/KissRevision13/A_Act1_'+who);walk=u.load_asset(B+'/Animations/PerformanceRevision11/A_'+role+'_'+sex+'Walk');idle=u.load_asset(B+'/Animations/'+role+'/A_'+role+'_BreathingIdle');assert source and walk and idle;data={b:[[],[],[]] for b in bones};smiles=[]
 def pose(clip,t):return list(u.AnimationLibrary.get_bone_poses_for_time(clip,bones,min(clip.get_play_length(),max(0,t)),False,mesh))
 for f in range(601):
  t=f/30;old=pose(source,t);p=old;w=0
  if who=='Juliet' and t<5.9:
   phase=(t*1.12)%walk.get_play_length();p=pose(walk,phase);w=1-smooth((t-5.45)/.45)
   if phase>walk.get_play_length()-.10:p=[blend(x,y,smooth((phase-walk.get_play_length()+.1)/.1)) for x,y in zip(p,pose(walk,0))]
  elif 12.0<=t<14.8:
   p=pose(idle,(t-12)%idle.get_play_length());w=smooth((t-12)/.3)
  elif 14.8<=t<16.25:
   # A soft partial walking step carries the actor to the kiss mark.
   p=pose(walk,min(.48,(t-14.8)*.4));rest=pose(idle,(t-12)%idle.get_play_length());p=[blend(x,y,.65) for x,y in zip(rest,p)];w=smooth((t-14.8)/.22)*(1-smooth((t-15.95)/.3))
  if w:
   p[0].translation=u.Vector(old[0].translation.x,old[0].translation.y,p[0].translation.z);p=[blend(x,y,w) for x,y in zip(old,p)]
   for i,b in enumerate(bones):
    if b in ['mouth_joint','eyes_joint','L_eyebrow_joint','R_eyebrow_joint']:p[i]=old[i]
  if who=='Romeo' and t<2.1:
   gate=line['mouth_gate'][f];fade=min(1,max(0,(t-line['start'])/.07),max(0,(line['end']-t)/.09));opening=(.5-.5*math.cos((t-line['start'])*2*math.pi*3.6))*gate*fade;p[bones.index('mouth_joint')].scale3d=u.Vector(1-.05*opening,.28+2.3*opening if gate*fade>.01 else 1,1)
  smiles.append(.9 if who=='Romeo' and t<2.1 else (.65 if t<16 else 0))
  for b,tr in zip(bones,p):data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Act1_'+who;anim=u.load_asset(dest) or e.duplicate_asset(source.get_path_name(),dest);controller=anim.get_editor_property('controller');controller.open_bracket('Entrance dialogue and stepping approach',False)
 for b,keys in data.items():assert controller.set_bone_track_keys(b,*keys,False)
 controller.close_bracket(False)
 if u.AnimationLibrary.does_curve_exist(anim,'FaceSmile',u.RawCurveTrackTypes.RCT_FLOAT):u.AnimationLibrary.remove_curve(anim,'FaceSmile')
 u.AnimationLibrary.add_curve(anim,'FaceSmile');u.AnimationLibrary.add_float_curve_keys(anim,'FaceSmile',[f/30 for f in range(601)],smiles);u.AnimationLibrary.add_curve_meta_data(mesh.skeleton,'FaceSmile',False);u.AnimationLibrary.set_curve_meta_data_material(mesh.skeleton,'FaceSmile',True);e.save_loaded_asset(mesh.skeleton,False);assert e.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);report[who]={'asset':dest}
 # Replace only the entrance and approach keys; keep the rest of the staging.
 for wpt in list(actors):
  if isinstance(wpt,u.DCWaypoint) and wpt.get_editor_property('performer')==a:
   t=wpt.get_editor_property('time')
   if (who=='Juliet' and t<5.9) or t>=12:aes.destroy_actor(wpt)
 rows=[]
 if who=='Juliet':
  start=(-5150.,-8250.);end=(-5900.,-8765.);yaw=math.degrees(math.atan2(end[1]-start[1],end[0]-start[0]));a.set_actor_location_and_rotation(u.Vector(*start,ground(*start)),u.Rotator(yaw=yaw),False,False)
  for f in range(1,178):
   t=f/30;alpha=min(1,t/5.6);x=start[0]+(end[0]-start[0])*alpha;y=start[1]+(end[1]-start[1])*alpha;rows.append((t,x,y,yaw))
 initial_y=-8850 if who=='Romeo' else -8765;final_y=-8832.5 if who=='Romeo' else -8782.5;final_yaw=90 if who=='Romeo' else -90
 for f in range(360,601):
  t=f/30;w=smooth((t-14.8)/1.2);rows.append((t,-5000,initial_y+(final_y-initial_y)*w,final_yaw*smooth((t-12)/.7)))
 for t,x,y,yaw in rows:
  wpt=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,ground(x,y)),u.Rotator(yaw=yaw));wpt.set_editor_property('performer',a);wpt.set_editor_property('time',t);wpt.set_actor_label(f'{who} natural staging {t:.3f}');wpt.set_folder_path('Act1/Blocking');wpt.set_is_temporarily_hidden_in_editor(True)
# Persistent invisible preparation mark at the studio clapperboard.
board=next(a for a in actors if isinstance(a,u.DCClapperboard));p=board.get_actor_location()+u.Vector(250,0,0);p.z=board.get_actor_location().z;mark=next((a for a in actors if isinstance(a,u.DCStartMark)),None) or aes.spawn_actor_from_class(u.DCStartMark,p,u.Rotator(yaw=180));mark.set_actor_location_and_rotation(p,u.Rotator(yaw=180),False,False);mark.set_actor_label('Preparation spawn - hidden in gameplay');mark.set_actor_hidden_in_game(True);mark.set_actor_enable_collision(False);report['spawn']=list(p.to_tuple())
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,B+'/Levels/L_DC_Act1_Promise');(O/'authoring.json').write_text(json.dumps(report,indent=2));print('ACT1_14_AUTHORED',report)
