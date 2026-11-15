"""Stage the killer's charge, the couple's stand and two clock-synchronized arrow kills."""
import unreal as u,json,math,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/BowRevision6';OUT=ROOT/'Saved/Act2/BowRevision6';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')}
baseline=json.loads((OUT/'baseline.json').read_text());timing=json.loads((ROOT/'SourceArt/Act2/BowRevision6/Audio/timing.json').read_text())
def load(p):
 a=u.load_asset(p);assert a,p;return a
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-x for x in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True).to_tuple();assert hit[0],(x,y);return hit[4].z+3
cache=OUT/'original_routes.json'
if cache.exists():original=json.loads(cache.read_text())
else:
 original={}
 for who,a in cast.items():
  rows=[]
  for f in range(381):
   a.evaluate(f/10);rows.append([*a.get_actor_location().to_tuple(),a.get_actor_rotation().yaw])
  original[who]=rows;a.evaluate(0)
 cache.write_text(json.dumps(original))
def old(who,t):return original[who][min(380,round(t*10))]
def path(a,rows):
 for w in list(aes.get_all_level_actors()):
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:aes.destroy_actor(w)
 a.set_actor_location_and_rotation(u.Vector(*rows[0][1:4]),u.Rotator(yaw=rows[0][4]),False,False);a.set_editor_property('linear_movement',True);a.set_editor_property('use_waypoint_height',True)
 for t,x,y,z,yaw in rows[1:]:
  w=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,z),u.Rotator(yaw=yaw));w.set_editor_property('performer',a);w.set_editor_property('time',float(t));w.set_actor_label(a.get_actor_label()+f' bow staging {t:.2f}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
def lerp(a,b,w):return [x+(y-x)*w for x,y in zip(a,b)]
targets={'Raid orc 1':(-4950,-9350,22.65,'DeathFront'),'Raid orc 2':(-4680,-9220,25.2,'DeathCrouch')}
routes={}
for who in ['Romeo','Juliet','Armoured monster',*targets]:
 rows=[]
 for f in range(381):
  t=f/10;p=old(who,t);x,y,z,yaw=p
  if who=='Romeo' and t>=13:x,y,yaw=-5050,-8880,-150;z=ground(x,y)+(3 if t>=29 else 0)
  if who=='Juliet' and t>=13:
   if t<18.45:x,y=-4920,-8720
   elif t<20:x,y=lerp([-4920,-8720],[-4890,-8780],smooth((t-18.45)/1.55))
   elif t<22.7:x,y=-4890,-8780
   elif t<23.4:x,y=lerp([-4890,-8780],[-4830,-8810],(t-22.7)/.7)
   elif t<29:x,y=-4830,-8810
   elif t<31:x,y=lerp([-4830,-8810],[-4845,-8730],smooth((t-29)/2))
   else:x,y=-4845,-8730
   yaw=-97 if t>=19 and t<22.8 else (-70 if 23.4<=t<26 else -150)
   z=ground(x,y)
  if who=='Armoured monster' and t>=13:
   x,y=lerp([-5900,-9085],[-5200,-8970],smooth((t-13)/5.8));yaw=math.degrees(math.atan2(115,700)) if t<18.4 else 30;z=ground(x,y)
  if who in targets and t>=13:
   tx,ty,impact,clip=targets[who];x,y=lerp(old(who,13)[:2],[tx,ty],min(1,(t-13)/(impact-13)));yaw=math.degrees(math.atan2(-8780-y,-4890-x));z=ground(x,y)
  rows.append([t,x,y,z,yaw])
 path(cast[who],rows);routes[who]=rows
# Retarget the run once for the actual killer, preserving its skeleton.
runpath=F+'/A_Raider_Run'
if not eal.does_asset_exist(runpath):
 clip=load(BASE+'/Mixamo/SupportClips/A_Support_Run');result=u.IKRetargetBatchOperation.duplicate_and_retarget([u.AssetRegistryHelpers.create_asset_data(clip)],load(BASE+'/Mixamo/SK_XBot'),load(BASE+'/Act2/SK_Act2_OrcComplete'),load(BASE+'/Act2/RTG_Act2_Raider'),'A_Support_','A_Raider_','','',F,False,False,True);assert result
 generated=str(result[0].package_name)
 if generated!=runpath:assert eal.rename_asset(generated,runpath)
clips={n:load(F+'/A_Archer_'+n) for n in ['EquipBow','DrawArrow','WalkLeft']};clips['Run']=load(runpath);clips['Idle']=load(BASE+'/Animations/Swordsman/A_Swordsman_Support_Idle')
for n in ['DeathFront','DeathCrouch']:clips[n]=load(F+'/A_Raider_'+n)
report={'routes':routes,'performances':{},'shots':[]}
for who in ['Romeo','Juliet','Armoured monster',*targets]:
 a=cast[who];source=load(baseline[who]['source']);mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];data={b:[[],[],[]] for b in bones}
 def poses(clip,t):return list(u.AnimationLibrary.get_bone_poses_for_time(clip,bones,max(0,min(t,clip.get_play_length())),False,mesh))
 for f in range(1141):
  t=f/30;pose=poses(source,t);replacement=None;w=1;death=False
  if who=='Romeo' and 13<=t<17.5:
   replacement=poses(clips['Idle'],(t-13)%clips['Idle'].get_play_length());w=smooth((t-13)/.25)*smooth((17.5-t)/.25)
  if who=='Armoured monster' and 13<=t<18.8:
   replacement=poses(clips['Run'],(t-13)*.85%clips['Run'].get_play_length());w=smooth((t-13)/.25)*smooth((18.8-t)/.2)
  if who=='Juliet' and 13<=t<29:
   if t<18.45:replacement=poses(source,12.8);w=smooth((t-13)/.2)
   else:
    if t<20:clip=clips['EquipBow'];phase=(t-18.45)/1.55
    elif t<22.35:clip=clips['DrawArrow'];phase=(t-20)/2.35
    elif t<22.7:clip=clips['DrawArrow'];phase=1
    elif t<23.4:clip=clips['WalkLeft'];phase=(t-22.7)/.7
    elif t<24.9:clip=clips['DrawArrow'];phase=(t-23.4)/1.5
    else:clip=clips['DrawArrow'];phase=1
    replacement=poses(clip,min(1,phase)*clip.get_play_length());w=smooth((t-18.45)/.2)*smooth((29-t)/.4)
    # A small release recoil at each loose; the supplied clip ends at full draw.
    for release in [22.35,24.9]:
     if release<=t<release+.3:
      recoil=math.sin((t-release)/.3*math.pi)
      for b in ['R_wrist_joint','R_elbow_joint']:
       if b in bones:
        i=bones.index(b);replacement[i].rotation=replacement[i].rotation*u.Rotator(roll=12*recoil).quaternion()
  if who in targets and t>=targets[who][2]-.12:
   hit=targets[who][2];clip=clips[targets[who][3]];replacement=poses(clip,max(0,t-hit));w=smooth((t-hit+.12)/.12);death=True
  if replacement:
   # Remove planar source travel. Keep death fall height and original facial acting.
   for i,b in enumerate(bones):
    if any(s in b.lower() for s in ['mouth','eye','brow','jaw','lip']):continue
    tr=replacement[i]
    if i==0:
     tr.translation=u.Vector(pose[i].translation.x,pose[i].translation.y,tr.translation.z if death else pose[i].translation.z)
    pose[i]=blend(pose[i],tr,w)
  if who in ['Romeo','Juliet']:
   i=bones.index('mouth_joint');g=0;opening=0
   for line in timing['lines']:
    if line['speaker']==who and line['start']<=t<line['end']:
     gate=line['mouth_gate'][f];g=max(g,gate);opening=max(opening,gate*(.5-.5*math.cos((t-line['start'])*math.pi*7.2)))
   pose[i].scale3d=u.Vector(1-.08*opening,max(.12,(1-g)*.75+g*(.18+1.65*opening)),1)
  for b,tr in zip(bones,pose):
   data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Stand_'+who.replace(' ','').replace('.','');anim=u.load_asset(dest) or eal.duplicate_asset(source.get_path_name(),dest);c=anim.get_editor_property('controller');c.open_bracket('Hold ground and fight together',False)
 for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False),b
 c.close_bracket(False);assert eal.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);report['performances'][who]=dest
j=cast['Juliet'];j.set_editor_property('carried_mesh',load(F+'/SM_BowBasic'));j.set_editor_property('carry_bone','L_wrist_joint');j.set_editor_property('carry_transform',u.Transform(rotation=u.Rotator(yaw=90),scale=u.Vector(.65,.65,.65)));j.set_editor_property('prop_visible_from',18.6);j.set_editor_property('prop_visible_until',29)
task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/BowRevision6/Audio/Act2_Soundtrack.wav');task.destination_path=F;task.destination_name='SW_StandTogether';task.automated=True;task.save=True;task.replace_existing=True;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=load(F+'/SW_StandTogether');sound.set_editor_property('sound_asset_compression_type',u.SoundAssetCompressionType.PCM);sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE);eal.save_loaded_asset(sound,False)
ambient=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue'));ambient.get_component_by_class(u.AudioComponent).set_sound(sound)
for who,a in cast.items():a.set_actor_location_and_rotation(u.Vector(*original[who][0][:3]),u.Rotator(yaw=original[who][0][3]),False,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(OUT/'staging.json').write_text(json.dumps(report,indent=2));print('STAND_TOGETHER_STAGED')
