"""Validate saved acting, continuous camera, natural dialogue and final bindings."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/ActingRevision4';BASE='/Game/DirectorsCut'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise');world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(p):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(p.x,p.y,7000),u.Vector(p.x,p.y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple();assert hit[0];return hit[4].z
report={'feet':[],'reaction':{},'cameras':[]}
smith=cast['Mr. Smith'];comp=smith.get_component_by_class(u.SkeletalMeshComponent)
bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())];toes=[b for b in bones if 'toe' in b.lower()];assert len(toes)>=2,toes
for f in range(211):
 smith.evaluate(f/30);gaps=[comp.get_socket_location(b).z-ground(comp.get_socket_location(b)) for b in toes];gap=min(gaps);assert -.2<=gap<=4,(f,gaps);report['feet'].append([f/30,gap])
smith.evaluate(0)
for who in ['Romeo','Juliet']:
 a=cast[who];c=a.get_component_by_class(u.SkeletalMeshComponent);rows=[]
 for time in [10.5,11,11.5,12]:
  a.evaluate(time);head=c.get_socket_location('head_joint');face=c.get_socket_location('mouth_joint')-head;camera=u.Vector(-4550,-8500,1430)-head;dot=(face.x*camera.x+face.y*camera.y)/math.hypot(face.x,face.y)/math.hypot(camera.x,camera.y);assert dot<0,(who,time,dot);rows.append([time,dot])
 report['reaction'][who]=rows;a.evaluate(0)
cams=sorted([a for a in actors if isinstance(a,u.CameraActor) and a.actor_has_tag('DCReferenceShot')],key=lambda a:float(next(str(t).split('=')[1] for t in a.tags if str(t).startswith('DCStart='))))
assert len(cams)==5;assert (cams[3].get_actor_location()-cams[4].get_actor_location()).length()<.01;assert cams[4].actor_has_tag('DCContinueShot')
for a in cams:report['cameras'].append(dict(name=a.get_actor_label(),position=list(a.get_actor_location().to_tuple()),tags=[str(t) for t in a.tags]))
audio=next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue'));sound=audio.get_component_by_class(u.AudioComponent).sound;assert abs(sound.duration-38)<.001
manifest=json.loads((ROOT/'SourceArt/Act2/Audio/timing.json').read_text());final=[l for l in manifest['lines'] if 'promis' in l.get('name','').lower() or 'promis' in l.get('source','').lower()];assert len(final)==1
bye=next(l for l in manifest['lines'] if l['name']=='Goodbye');assert final[0]['start']>=bye['end'] and final[0]['end']<=38
seq=u.load_asset(BASE+'/Act2/LS_DC_Act2_OrcRaid');cuts=[t for t in seq.get_tracks() if isinstance(t,u.MovieSceneCameraCutTrack)];assert len(cuts)==1 and len(cuts[0].get_sections())==4
for track in seq.get_tracks():
 if isinstance(track,u.MovieSceneAudioTrack):assert all(s.get_sound()==sound for s in track.get_sections())
report['sound']=sound.get_path_name();report['final_line']={k:v for k,v in final[0].items() if k!='mouth_gate'};report['passed']=True
(OUT/'validation.json').write_text(json.dumps(report,indent=2));print('ACT2_REVISION4_VALIDATED')
