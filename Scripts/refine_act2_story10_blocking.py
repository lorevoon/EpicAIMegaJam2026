"""Back-to-back vigilance, shoot-run-shoot, and return to Romeo."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/StoryRevision10';OUT=ROOT/'Saved/Act2/StoryRevision10';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise'
eal=u.EditorAssetLibrary;les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('prop_only')}
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


j=cast['Juliet'];timeline=json.loads((OUT/'staging.json').read_text())['timeline']
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,7000),u.Vector(x,y,-3000),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,u.DrawDebugTrace.NONE,True);assert h;return h.to_tuple()[4].z+3
J=(-4870.,-8850.);ADV=(-4660.,-9190.);MID=(-4550.,-8940.);M=(-4770.,-8810.);rows=[]
for f in range(381):
 t=f/10;x,y=J
 if 18.1<t<19.1:x,y=mix(J,ADV,t-18.1)
 elif 19.1<=t<26.5:x,y=ADV
 elif 26.5<=t<28.3:
  v=(t-26.5)/1.8;x,y=[(1-v)**2*ADV[i]+2*v*(1-v)*MID[i]+v*v*M[i] for i in range(2)]
 elif t>=28.3:x,y=M
 if t<15.433333:yaw=-150
 elif t<16.1:yaw=yawblend(-150,-99,smooth((t-15.433333)/.666667))
 elif t<17.95:yaw=-99
 elif t<18.35:yaw=yawblend(-99,-55,smooth((t-17.95)/.4))
 elif t<18.9:yaw=-55
 elif t<19.25:yaw=yawblend(-55,-37,smooth((t-18.9)/.35))
 elif t<25.8:yaw=-37
 elif t<26.5:yaw=yawblend(-37,35,smooth((t-25.8)/.7))
 elif t<28.3:
  v=(t-26.5)/1.8;dx=(1-v)*(MID[0]-ADV[0])+v*(M[0]-MID[0]);dy=(1-v)*(MID[1]-ADV[1])+v*(M[1]-MID[1]);yaw=math.degrees(math.atan2(dy,dx))
 else:yaw=yawblend(149.4,135,smooth((t-28.3)/.3))
 rows.append([t,x,y,ground(x,y),yaw])
for w in list(actors):
 if isinstance(w,u.DCWaypoint) and w.get_editor_property('performer')==j:aes.destroy_actor(w)
j.set_actor_location_and_rotation(u.Vector(*rows[0][1:4]),u.Rotator(yaw=rows[0][4]),False,False)
for t,x,y,z,yaw in rows[1:]:
 w=aes.spawn_actor_from_class(u.DCWaypoint,u.Vector(x,y,z),u.Rotator(yaw=yaw));w.set_editor_property('performer',j);w.set_editor_property('time',t);w.set_actor_label(f'Juliet story @ {t:.1f}');w.set_folder_path('Act2/Blocking');w.set_is_temporarily_hidden_in_editor(True)
smith=cast['Mr. Smith'];smith.tags=list(smith.tags)+['RaidSmithFocus'] if not smith.actor_has_tag('RaidSmithFocus') else smith.tags
# Replace the dedicated Smith beat with Juliet's action, keeping five scored scenes.
beats={str(a.get_editor_property('beat_id')):a for a in actors if isinstance(a,u.DCBeat)}
for id,start,end,title,names in [('2.1',0,10,'Orc raid',['Armoured monster','Raid orc 1','Raid orc 4','Mr. Smith']),('2.2',10,15.433333,'Stand together',['Romeo','Juliet']),('2.3',15.433333,19,'Juliet strikes first',['Juliet','Raid orc 1']),('2.4',19,29,'Sword fight',['Romeo','Armoured monster']),('2.5',29,38,'A final goodbye',['Romeo','Juliet'])]:
 b=beats[id];b.set_editor_property('window_start',start);b.set_editor_property('window_end',end);b.set_editor_property('pose_ready_time',0);b.set_editor_property('required_hold',1.2);b.set_editor_property('display_name',title);b.set_editor_property('caption',title);refs=[]
 for name in names:
  ref=u.DCAnchorRef();ref.set_editor_property('performer',cast[name]);ref.set_editor_property('part',u.DCAnchorPart.HEAD);refs.append(ref)
 b.set_editor_property('anchors',refs)
task=u.AssetImportTask();task.filename=str(ROOT/'SourceArt/Act2/StoryRevision10/Audio/Act2_Soundtrack.wav');task.destination_path=F;task.destination_name='SW_Story';task.automated=True;task.replace_existing=True;task.save=True;u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=u.load_asset(F+'/SW_Story');sound.set_editor_property('sound_asset_compression_type',u.SoundAssetCompressionType.PCM);sound.set_editor_property('loading_behavior',u.SoundWaveLoadingBehavior.FORCE_INLINE);eal.save_loaded_asset(sound,False)
next(a for a in actors if isinstance(a,u.AmbientSound) and a.actor_has_tag('DCSceneDialogue')).get_component_by_class(u.AudioComponent).set_sound(sound)
for a in cast.values():
 if a!=j:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
assert les.load_level(BASE+'/Levels/L_DC_Act1_Promise');assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast={a.get_actor_label():a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)};j=cast['Juliet'];j.evaluate(17.75);v=cast['Raid orc 1'];v.evaluate(18.1);wrist=j.get_component_by_class(u.SkeletalMeshComponent).get_socket_transform('L_wrist_joint');hit=v.get_component_by_class(u.SkeletalMeshComponent).get_socket_location('head_joint');aim=u.MathLibrary.find_look_at_rotation(wrist.translation,hit);q=wrist.rotation;desired=u.Rotator(pitch=90,yaw=aim.yaw+90).quaternion();grip=u.Transform(rotation=(u.Quat(-q.x,-q.y,-q.z,q.w)*desired).rotator(),scale=u.Vector(.65,.65,.65));grip.translation=-grip.transform_location(u.Vector(0,-36,0));j.set_editor_property('carry_transform',grip)
for a in cast.values():a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
(OUT/'staging.json').write_text(json.dumps({'timeline':timeline,'juliet':rows},indent=2));print('STORY10_STAGED')
