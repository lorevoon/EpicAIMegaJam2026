"""Revision 7: a grounded stand, deliberate sword work and a stationary kneel."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/SmoothRevision7';V6=BASE+'/Act2/BowRevision6';OUT=ROOT/'Saved/Act2/SmoothRevision7';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')};baseline=json.loads((OUT/'baseline.json').read_text())['cast']
def load(p):
 a=u.load_asset(p);assert a,p;return a
def smooth(x):
 x=max(0,min(1,x));return x*x*(3-2*x)
def blend(a,b,w):
 q=list(a.rotation.to_tuple());r=list(b.rotation.to_tuple())
 if sum(x*y for x,y in zip(q,r))<0:r=[-x for x in r]
 v=[x+(y-x)*w for x,y in zip(q,r)];n=math.sqrt(sum(x*x for x in v));tr=u.Transform();tr.translation=a.translation+(b.translation-a.translation)*w;tr.rotation=u.Quat(*[x/n for x in v]);tr.scale3d=a.scale3d+(b.scale3d-a.scale3d)*w;return tr
def mix(a,b,w):return [x+(y-x)*w for x,y in zip(a,b)]
def yawblend(a,b,w):return a+((b-a+180)%360-180)*w
groundignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,groundignore,u.DrawDebugTrace.NONE,True);assert h,(x,y);return h.to_tuple()[4].z+3
def old(who,t):return baseline[who]['rows'][max(0,min(380,round(t*10)))]
def interpolate(keys,t):
 for a,b in zip(keys,keys[1:]):
  if t<=b[0]:return mix(a[1:],b[1:],max(0,(t-a[0])/(b[0]-a[0])))
 return keys[-1][1:]
R=(-5000.,-8824.5);J=(-4890.,-8734.5)
leaderkeys=[(0,-5750,-10280),(4,-5750,-9600),(6.2,-5900,-9085),(13,-5900,-9085),(18.4,R[0]-150,R[1]-90),(38,R[0]-150,R[1]-90)]
route_data={}
for who,a in cast.items():
 if who=='Mr. Smith':continue
 rows=[]
 for f in range(381):
  t=f/10
  if who=='Romeo':x,y=R;yaw=yawblend(-167,-150,smooth((t-17)/1.6))
  elif who=='Juliet':
   x,y=J
   if t<18:yaw=-150
   elif t<20:yaw=yawblend(-150,-96,smooth((t-18)/2))
   elif t<22.8:yaw=-96
   elif t<23.6:yaw=yawblend(-96,-65,smooth((t-22.8)/.8))
   elif t<26.2:yaw=-65
   else:yaw=yawblend(-65,30,smooth((t-26.2)/2.8))
  elif who=='Armoured monster':
   x,y=interpolate(leaderkeys,t);p=interpolate(leaderkeys,max(0,t-.05));q=interpolate(leaderkeys,min(38,t+.05));yaw=math.degrees(math.atan2(q[1]-p[1],q[0]-p[0])) if math.dist(p,q)>.01 else 30
   if 6.2<=t<6.6:yaw=yawblend(106.24,30,smooth((t-6.2)/.4))
   if 18.4<=t<18.8:yaw=yawblend(math.degrees(math.atan2(R[1]-90+9085,R[0]-150+5900)),30,smooth((t-18.4)/.4))
  else:
   i=int(who[-1]);side={1:-230,2:230,3:-460,4:460}[i];lag=150 if i<=2 else 420
   lead=interpolate(leaderkeys,min(t,5.2));formation=(lead[0]+side,lead[1]-lag)
   at6=interpolate(leaderkeys,5.2);start=(at6[0]+side,at6[1]-lag)
   if t<=6:x,y=formation
   elif i<=2:
    end=(-4950,-9350) if i==1 else (-4680,-9220);x,y=mix(start,end,smooth((t-6)/9))
   else:
    end=(-8600,-10050) if i==3 else (-8500,-10650);x,y=mix(start,end,smooth((t-6)/11))
   if i<=2 and t>=15:yaw=math.degrees(math.atan2(J[1]-y,J[0]-x))
   else:
    prev=rows[-1] if rows else None;yaw=math.degrees(math.atan2(y-prev[2],x-prev[1])) if prev and math.hypot(y-prev[2],x-prev[1])>.01 else (90 if t<6 else 180)
  z=ground(x,y);rows.append([t,x,y,z,yaw])
 for w in list(aes.get_all_level_actors()):
  if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==a:aes.destroy_actor(w)
 a.set_actor_location_and_rotation(u.Vector(*rows[0][1:4]),u.Rotator(yaw=rows[0][4]),False,False);a.set_editor_property('linear_movement',True);a.set_editor_property('use_waypoint_height',True)
 for t,x,y,z,yaw in rows[1:]:
  w=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,z),u.Rotator(yaw=yaw));w.set_editor_property('performer',a);w.set_editor_property('time',t);w.set_actor_label(who+f' grounded @ {t:.1f}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
 route_data[who]=rows
# The four raiders remain separated throughout their opening formation.
minimum=min(math.dist(route_data['Raid orc '+str(i)][f][1:3],route_data['Raid orc '+str(j)][f][1:3]) for f in range(61) for i in range(1,5) for j in range(i+1,5));assert minimum>=220,minimum
clips={}
def clip(key,path):clips[key]=load(path)
for role in ['Swordsman','Raider']:
 for name in ['DrawSword','Guard','Dodge']:clip(role+name,F+'/A_'+role+'_'+name)
for role in ['Swordsman','Archer']:
 clip(role+'Idle',BASE+'/Animations/'+role+'/A_'+role+'_Support_Idle')
for name in ['MeleeSlash','MeleeBackhand','MeleeDownward']:
 clip('Swordsman'+name,BASE+'/Animations/Swordsman/A_Swordsman_Support_'+name);clip('Raider'+name,BASE+'/Act2/A_Raider_Support_'+name)
clip('EquipBow',V6+'/A_Archer_EquipBow');clip('DrawArrow',V6+'/A_Archer_DrawArrow');clip('BowReady',BASE+'/Animations/Archer/A_Archer_Bow_Idle');clip('BowAim',BASE+'/Animations/Archer/A_Archer_Support_BowAim');clip('BowRelease',BASE+'/Animations/Archer/A_Archer_Support_BowRelease');clip('RaiderWalk',BASE+'/Act2/A_Raider_Support_Walk')
report={'minimum_troop_spacing_cm':minimum,'routes':route_data,'performances':{}}
for who,a in cast.items():
 if who=='Mr. Smith':continue
 source=load(baseline[who]['source']);mesh=a.get_editor_property('character_mesh');comp=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];face={i for i,b in enumerate(bones) if any(s in b.lower() for s in ['mouth','eye','brow','jaw','lip'])};data={b:[[],[],[]] for b in bones}
 def pose(anim,t):return list(u.AnimationLibrary.get_bone_poses_for_time(anim,bones,max(0,min(t,anim.get_play_length())),False,mesh))
 rootref=pose(source,0)[0]
 # Timeline entries: start, key, clip start fraction, end fraction, transition seconds.
 if who=='Romeo':
  timeline=[(0,'SwordsmanIdle',0,None,0),(10,'SOURCE',10,11.8,.3),(11.8,'SwordsmanDrawSword',0,1,.3),(13.5,'SwordsmanGuard',0,None,.35),(18.8,'SwordsmanMeleeSlash',.08,.72,.25),(20.2,'SwordsmanDodge',.10,.74,.32),(21.65,'SwordsmanGuard',0,None,.3),(23.15,'SwordsmanDodge',.10,.72,.3),(24.6,'SwordsmanMeleeDownward',.08,.85,.3),(26,'SOURCE',26,38,.25)]
 elif who=='Juliet':
  timeline=[(0,'ArcherIdle',0,None,0),(10,'SOURCE',10,11.8,.3),(11.8,'EquipBow',0,1,.3),(13.2,'BowReady',0,None,.35),(20.4,'DrawArrow',0,1,.3),(22.35,'BowRelease',0,.6,.10),(22.85,'BowReady',0,None,.3),(23.45,'DrawArrow',0,1,.25),(24.9,'BowRelease',0,.6,.10),(25.4,'BowReady',0,None,.3),(28.6,'SOURCE',31,38,.65)]
 elif who=='Armoured monster':
  timeline=[(0,'SOURCE',0,13,0),(13,'SOURCE',13,18.5,.3),(18.5,'RaiderGuard',0,None,.35),(20.2,'RaiderMeleeBackhand',.10,.7,.3),(21.65,'RaiderMeleeSlash',.08,.78,.3),(23.15,'RaiderMeleeBackhand',.12,.7,.3),(24.6,'RaiderMeleeDownward',.08,.85,.3),(26,'SOURCE',26,38,.25)]
 elif who in ['Raid orc 1','Raid orc 2']:
  impact=22.65 if who.endswith('1') else 25.2;timeline=[(0,'RaiderWalk',0,None,0),(15,'RaiderDrawSword',0,1,.3),(16.7,'RaiderGuard',0,None,.35),(impact,'SOURCE',impact,38,.12)]
 else:timeline=[(0,'RaiderWalk',0,None,0),(17,'RaiderGuard',0,None,.5)]
 def sample(index,t):
  start,key,lo,hi,transition=timeline[index];end=timeline[index+1][0] if index+1<len(timeline) else 38
  if key=='SOURCE':return pose(source,lo+(hi-lo)*max(0,min(1,(t-start)/max(.001,end-start))))
  anim=clips[key];length=anim.get_play_length()
  phase=(t-start)%length if hi is None else length*(lo+(hi-lo)*max(0,min(1,(t-start)/max(.001,end-start))))
  p=pose(anim,phase)
  p[0].translation=u.Vector(rootref.translation.x,rootref.translation.y,p[0].translation.z)
  return p
 for f in range(1141):
  t=f/30;index=max(i for i,q in enumerate(timeline) if q[0]<=t+.00001);new=sample(index,t);boundary=timeline[index][0];transition=timeline[index][4]
  if index and t-boundary<transition:
   previous=sample(index-1,boundary);w=smooth((t-boundary)/transition);new=[blend(x,y,w) for x,y in zip(previous,new)]
  original=pose(source,t)
  for i in face:new[i]=original[i]
  for b,tr in zip(bones,new):data[b][0].append(tr.translation);data[b][1].append(tr.rotation);data[b][2].append(tr.scale3d)
 dest=F+'/A_Grounded_'+who.replace(' ','').replace('.','');anim=u.load_asset(dest) or eal.duplicate_asset(source.get_path_name(),dest);controller=anim.get_editor_property('controller');controller.open_bracket('Grounded acting with blended defense and draw',False)
 for b,keys in data.items():assert controller.set_bone_track_keys(b,*keys,False)
 controller.close_bracket(False);assert eal.save_loaded_asset(anim,False);cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;a.set_editor_property('animation_cues',[cue]);report['performances'][who]={'asset':dest,'timeline':timeline}
j=cast['Juliet'];j.set_editor_property('prop_visible_from',12.05);j.set_editor_property('prop_visible_until',28.6)
# Fit the bow string behind its grip using the actual aiming wrist.
j=cast['Juliet'];j.evaluate(22.3);victim=cast['Raid orc 1'];victim.evaluate(22.65);wrist=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_transform('L_wrist_joint');hit=victim.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');aim=u.MathLibrary.find_look_at_rotation(wrist.translation,hit);desired=u.Rotator(pitch=90,yaw=aim.yaw+90).quaternion();q=wrist.rotation;relative=u.Quat(-q.x,-q.y,-q.z,q.w)*desired;grip=u.Transform(rotation=relative.rotator(),scale=u.Vector(.65,.65,.65));grip.translation=-grip.transform_location(u.Vector(0,-36,0));j.set_editor_property('carry_transform',grip)
# Reset placements directly, rather than evaluating against cached old routes.
for who,a in cast.items():
 if who in route_data:
  row=route_data[who][0];a.set_actor_location_and_rotation(u.Vector(*row[1:4]),u.Rotator(yaw=row[4]),False,False)
 else:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL);(OUT/'staging.json').write_text(json.dumps(report,indent=2));print('SMOOTH7_STAGED',minimum)
