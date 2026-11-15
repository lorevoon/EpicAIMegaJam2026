"""Reload saved Act 2; check bindings, motion continuity and deterministic seeking."""
import unreal as u, json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Saved/Act2/Flow'
BASE='/Game/DirectorsCut'
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise')
cast={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.DCPerformer)}
baseline=json.loads((OUT/'baseline.json').read_text())
report={'motion':{},'seek':{},'bindings':[]}
def angle(a,b):
 d=min(1.,abs(sum(x*y for x,y in zip(a.to_tuple(),b.to_tuple()))))
 return math.degrees(2*math.acos(d))
for who,a in cast.items():
 cue=a.get_editor_property('animation_cues')[0]
 anim=cue.animation
 assert abs(anim.get_play_length()-38)<.001
 if who in ['Romeo','Juliet','Armoured monster','Mr. Smith']:
  assert '/FlowRevision5/' in anim.get_path_name(),who
 else:assert anim.get_path_name()==baseline[who]
 comp=a.get_component_by_class(u.SkeletalMeshComponent)
 bones=['pelvis_joint','chest_joint','L_shoulder_joint','R_shoulder_joint','L_elbow_joint','R_elbow_joint']
 mesh=a.get_editor_property('character_mesh')
 if who in ['Romeo','Armoured monster']:
  scores={}
  for label,clip in [('before',u.load_asset(baseline[who])),('after',anim)]:
   poses=[u.AnimationLibrary.get_bone_poses_for_time(clip,bones,f/30,False,mesh) for f in range(564,781)]
   delta=[max(angle(x.rotation,y.rotation) for x,y in zip(p,q)) for p,q in zip(poses,poses[1:])]
   longest=run=0
   for v in delta:
    run=run+1 if v<.25 else 0;longest=max(longest,run)
   scores[label]={'longest_quiet_seconds':longest/30,'peak_frame_rotation_deg':max(delta),'mean_frame_rotation_deg':sum(delta)/len(delta)}
  report['motion'][who]=scores
  assert scores['after']['longest_quiet_seconds']<.5,(who,scores)
  assert scores['after']['longest_quiet_seconds']<=scores['before']['longest_quiet_seconds'],(who,scores)
 # Runtime sampling must be independent of previous sample/replay order.
 samples={}
 for t in [3,7.5,11,14,19.6,21,22.6,24,25.55,27,30,32,35,38]:
  a.evaluate(t);samples[t]=list(comp.get_socket_location('R_wrist_joint').to_tuple())
 for t in reversed(list(samples)):
  a.evaluate(t);actual=list(comp.get_socket_location('R_wrist_joint').to_tuple())
  assert math.dist(samples[t],actual)<.01,(who,t)
 report['seek'][who]='passed'
 a.evaluate(0)
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid')
for b in seq.get_bindings():
 parent=b.get_parent()
 who=parent.get_name() if parent and parent.is_valid() else b.get_name()
 for track in b.get_tracks():
  if isinstance(track,u.MovieSceneSkeletalAnimationTrack):
   assert who in cast,who
   for section in track.get_sections():assert section.params.animation==cast[who].get_editor_property('animation_cues')[0].animation,who
   report['bindings'].append(who)
assert len(report['bindings'])==8
report['passed']=True
(OUT/'validation.json').write_text(json.dumps(report,indent=2))
print('ACT2_FLOW_VALIDATED',json.dumps(report))
