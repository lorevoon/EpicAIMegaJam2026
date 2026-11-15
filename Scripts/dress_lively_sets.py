"""Idempotent set-only dressing; real-scene references retain exact world transforms."""
import unreal as u,json,shutil,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/LivelySet';TAG='DC_LivelySet'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
report={};backup=R/'Saved/LivelySet/Before';backup.mkdir(parents=True,exist_ok=True)
names=['Crew_M01_Brown_Crop','Crew_F01_Auburn_Bun','Crew_F02_Blonde_Bob','Crew_M02_Silver_Swept','Crew_F03_Black_Ponytail']
for act,main in [(1,'Promise'),(2,'BrokenPromise')]:
 setpath=f'{B}/Levels/L_DC_Act{act}_Set'
 src=R/f'Content/DirectorsCut/Levels/L_DC_Act{act}_Set.umap'
 if not (backup/src.name).exists():shutil.copy2(src,backup/src.name)
 # Load the real level directly: hidden streaming actors have unregistered component transforms.
 assert les.load_level(f'{B}/Levels/L_DC_Act{act}_Real')
 wanted={'BP_Barrels_15','BP_Barrels_20','BP_Fence_180','BP_Fence_179','BP_Fence_128','BP_Fence_127','BP_Rocks_168','BP_Fence_178'}
 if act==2:wanted|={'BP_Cart_Wheat5','BP_Barrels_23'}
 refs=[]
 for a in aes.get_all_level_actors():
  if a.get_actor_label() not in wanted:continue
  for i,c in enumerate(a.get_components_by_class(u.StaticMeshComponent)):
   if c.static_mesh:refs.append((a.get_actor_label()+f' / {i}',c.static_mesh.get_path_name(),c.get_world_transform(),[c.get_material(j) for j in range(c.get_num_materials())]))
 assert len(refs)>15
 assert les.load_level(f'{B}/Levels/L_DC_Act{act}_{main}')
 world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
 stage=next(a for a in actors if a.get_actor_label().startswith('Soundstage floor'))
 bc,be=stage.get_actor_bounds(False);floor=bc.z+be.z
 ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
 def terrain(x,y):
  ignored=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
  h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,5000),u.Vector(x,y,-2000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignored,u.DrawDebugTrace.NONE,True).to_tuple()
  return h[4].z if h[0] else -2000
 # Cache support heights before adding meshes, never trace against our own dressing.
 studio=u.GameplayStatics.get_streaming_level(world,f'L_DC_Act{act}_Set');assert studio
 u.EditorLevelUtils.make_level_current(studio)
 for a in actors:
  if a.actor_has_tag(TAG) or ('/L_DC_Act'+str(act)+'_Set.') in a.get_path_name() and a.get_actor_label().startswith('H3D - Light'):
   aes.destroy_actor(a)
 created=[];crew=[];props=[]
 def spawn(cls,label,p=(0,0,0),yaw=0):
  a=aes.spawn_actor_from_class(cls,u.Vector(*p),u.Rotator(yaw=yaw));a.set_actor_label(label);a.tags=[TAG,'DC_SetOnly'];a.set_folder_path(f'Act{act}/Lively set');created.append(a);return a
 def mesh(label,path,p=(0,0,0),yaw=0,scale=(1,1,1)):
  a=spawn(u.StaticMeshActor,label,p,yaw);a.static_mesh_component.set_static_mesh(u.load_asset(path));a.set_actor_scale3d(u.Vector(*scale));a.static_mesh_component.set_collision_profile_name('BlockAll');return a
 def block(label,p,size,mat='M_Studio_Equipment'):
  a=mesh(label,'/Engine/BasicShapes/Cube',p,scale=tuple(v/100 for v in size));a.static_mesh_component.set_material(0,u.load_asset(B+'/Materials/'+mat));return a
 def floorprop(label,path,x,y,yaw=0):
  assert terrain(x,y)<floor+2,(label,'not exposed floor',x,y,terrain(x,y),floor)
  a=mesh(label,path,(x,y,0),yaw);c,e=a.get_actor_bounds(False);a.set_actor_location(u.Vector(x,y,floor-c.z+e.z+.5),False,False);props.append(label);return a
 for label,path,tr,mats in refs:
  a=mesh('Matched reference - '+label,path);a.set_actor_transform(tr,False,False)
  for j,m in enumerate(mats):
   if m:a.static_mesh_component.set_material(j,m)
  assert a.get_actor_transform().equals(tr)
 # Video village: one seated operator and a standing assistant, looking toward screens.
 tx,ty=(-4500,-9660) if act==1 else (-4510,-10000)
 if act==2:
  block('Video village - worktop',(tx,ty,floor+78),(85,220,8))
  for dy in [-90,90]:block('Video village - trestle',(tx,ty+dy,floor+36),(55,10,72))
  for dy in [-55,55]:
   block('Monitor - housing',(tx+12,ty+dy,floor+118),(10,80,52))
   block('Monitor - screen',(tx+6,ty+dy,floor+118),(1,71,42),'M_Studio_Monitor')
   block('Monitor - foot',(tx,ty+dy,floor+87),(25,40,8))
 # Add a low padded crew stool; sitting animation supplies the actual seated pose.
 sx,sy=tx-125,ty+45
 block('Video operator stool - seat',(sx,sy,floor+48),(48,48,8))
 for dx in [-18,18]:
  for dy in [-18,18]:block('Video operator stool - leg',(sx+dx,sy+dy,floor+22),(4,4,44))
 block('Video operator stool - back',(sx-24,sy,floor+72),(5,48,44))
 # Jib bay occupies the exposed black floor, away from the performance route.
 jx,jy=(-4750,-10105) if act==1 else (-4850,-10600)
 if act==2:
  for label,stem,dy in [('Jib','01_jib',0),('Rails','03_rails',-45)]:
   path=B+'/Props/Hyper3D_MovieSet/'+label+'/SM_'+label
   floorprop('Jib bay - '+label,path,jx,jy+dy)
  floorprop('Jib bay - cinema camera',B+'/Props/Hyper3D_MovieSet/CraneCameraV2/SM_CraneCamera_Dark',-4530,-10680,90)
 # Camera cases, battery cart, cable reels and practical stands fill peripheral bays.
 locations=[(-5500,-10290),(-5250,-10300),(-4420,-9180)] if act==1 else [(-5200,-10850),(-4900,-10900),(-4450,-10350)]
 for i,(x,y) in enumerate(locations):
  assert terrain(x,y)<floor+2,('case',x,y,terrain(x,y),floor)
  block('Camera department - flight case',(x,y,floor+36),(110,65,64))
  for dx in [-42,42]:block('Flight case - steel band',(x+dx,y,floor+36),(5,67,65),'M_Studio_Diffusion')
  for dx in [-40,40]:
   for dy in [-22,22]:
    wheel=mesh('Flight case - caster','/Engine/BasicShapes/Cylinder',(x+dx,y+dy,floor+6),scale=(.10,.10,.12));wheel.static_mesh_component.set_material(0,u.load_asset(B+'/Materials/M_Studio_Charcoal'))
  props.append('flight case')
 for x,y in ([(-5700,-10200),(-4450,-9000)] if act==1 else [(-5200,-10650),(-4430,-9700)]):
  floorprop('Grip bay - production light','/Game/Hollywood_Vol3/Meshes/SM_Production_Light_01a',x,y,130)
  for z,r,h in [(8,.46,.08),(26,.20,.32),(44,.46,.08)]:
   a=mesh('Grip bay - cable reel','/Engine/BasicShapes/Cylinder',(x-75,y,floor+z),scale=(r,r,h));a.static_mesh_component.set_material(0,u.load_asset(B+'/Materials/M_Studio_Charcoal'))
 def person(role,activity,x,y,yaw,lines,index,director=False):
  z=max(floor,terrain(x,y))+.8
  a=spawn(u.DCCrewMember,role,(x,y,z),yaw-90)
  sk=u.load_asset(F+('/SK_Director_Game' if director else '/SK_Live'+names[index%len(names)]));assert sk
  a.character.set_skeletal_mesh_asset(sk)
  anim=u.load_asset(F+('/A_DirectorGame_BreathingIdle_mixamo_com' if director else '/A_Crew_'+activity));assert anim
  a.set_editor_property('activity',anim);a.set_editor_property('crew_role',role);a.set_editor_property('lines',lines);a.set_editor_property('director',director)
  a.character.set_animation_mode(u.AnimationMode.ANIMATION_SINGLE_NODE)
  data=a.character.get_editor_property('animation_data');data.anim_to_play=anim;data.saved_looping=True;data.saved_playing=True;a.character.set_editor_property('animation_data',data)
  a.character.play_animation(anim,True);a.character.set_position(.5,False)
  a.set_editor_property('head_height',145 if activity=='Sitting' else 200)
  if director:a.set_editor_property('celebration',u.load_asset(F+'/A_DirectorGame_ThumbsUp'))
  crew.append(dict(role=role,activity=activity,position=[x,y,z],mesh=sk.get_path_name()));return a
 person('Video operator','Sitting',sx,sy,0,["I haven't slept for two days. If I blink, check focus.","That take was sharp. Please tell me we were recording."],0)
 person('Script supervisor','Talking',tx-155,ty-110,25,["Same hand for the cup, please. We have to cut these together.","That was take twelve. The slate still says eleven."],1)
 person('Jib operator','Talking',jx-120,jy-70,25,["Easy on the swing. This camera costs more than my flat.","Give me a clear path and I'll give you the shot."],3)
 person('Camera assistant','Idle',jx-110,jy+90,-45,["Fresh battery, clean card. Now we just need a take.","Nobody touch that focus mark. I mean nobody."],2)
 gx,gy=(-5400,-9750) if act==1 else (-5100,-10300)
 person('Producer','Arguing',gx,gy,90,["You said one more take three takes ago.","We can have the crane or overtime. We cannot have both."],3)
 person('First assistant director','Talking',gx,gy+130,-90,["I'm not rushing the art. I'm trying to get us lunch.","Five minutes means five minutes, not a new lighting setup."],4)
 person('Exhausted runner','Tired',-5800,-9900,0,["I went for coffee and came back with a call sheet for tomorrow.","I've had three dinners today. All of them were biscuits."],2)
 person('Lighting technician','Idle',-4600,-9150,180,["Every time they say natural light, I carry six more lamps.","Don't unplug that. We finally got rid of the flicker."],4)
 person('Director','Idle',tx-310,ty+210,155,["Find the frame first. The feeling will follow.","If this works, we're calling it a bold creative choice."],0,True)
 assert len(crew)==9
 assert not any(a.get_actor_label().startswith('H3D - Light') for a in aes.get_all_level_actors())
 assert u.EditorLoadingAndSavingUtils.save_map(studio.get_loaded_level().get_outer(),setpath)
 report[str(act)]=dict(crew=crew,reference_components=len(refs),new_actors=len(created),floor=floor,floor_props=props)
(R/'Saved/lively_dressing.json').write_text(json.dumps(report,indent=2));print('LIVELY_SETS_SAVED')
