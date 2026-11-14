"""Expand Act 1 around the user's complete mill group and author a clear 20-second take."""
import unreal as u, math, json
from pathlib import Path
root=Path(__file__).resolve().parents[1];level='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(level)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors());by={a.get_actor_label():a for a in actors}
assert 'BP_Building9' in by and 'BP_Windmill2' in by,'Need user-added complete mill group'
def place(a,p,yaw=None):
 a.set_actor_location(u.Vector(*p),False,False)
 if yaw is not None:a.set_actor_rotation(u.Rotator(yaw=yaw),False)
def props(a,**kw):
 for k,v in kw.items():a.set_editor_property(k,v)
def spawn(cls,label,p):
 a=aes.spawn_actor_from_class(cls,u.Vector(*p));a.set_actor_label(label);a.set_folder_path('Act1/Studio');a.tags=['DC_Studio'];return a
def block(label,p,size,mat):
 a=by.get(label)
 if not a:a=spawn(u.StaticMeshActor,label,p)
 place(a,p);a.set_actor_rotation(u.Rotator(),False);a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
 a.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,u.load_asset('/Game/DirectorsCut/Materials/'+mat))
 a.static_mesh_component.set_collision_profile_name('BlockAll');return a
# Keep complete original assemblies (and their child actors) at original scale.
group=['BP_Building9','BP_SmallHouse4','BP_Cart_Wheat5','BP_WheatStack_15','BP_Barrels_23','BP_Windmill2','BP_Mailbox5']
shift=u.Vector(6800,10000,0)
for label in group:
 a=by[label];p=a.get_actor_location()+shift
 a.set_actor_location(p,False,False)
 c,e=a.get_actor_bounds(False,True)
 p=a.get_actor_location();p.z+=20-(c.z-e.z);a.set_actor_location(p,False,False)
 a.set_folder_path('Act1/Complete mill group');a.tags=['DC_Studio','DC_MillAssembly']
for a in actors:
 if a.get_actor_label() in ['Practical windmill','Practical village house'] or a.get_actor_label().startswith('Practical wheat bundle') or isinstance(a,u.DCWaypoint):aes.destroy_actor(a)
# 56 x 44 m enclosed stage; only vertical walls are chroma green.
block('Soundstage floor - 30m x 24m',(0,0,-15),(5600,4400,30),'M_Studio_Charcoal').set_actor_label('Soundstage floor - 56m x 44m')
for label,p,size in [('Green screen - rear',(0,2200,1400),(5640,40,2800)),('Green screen - left',(-2800,0,1400),(40,4400,2800)),('Green screen - right',(2800,0,1400),(40,4400,2800)),('Green screen - front',(0,-2200,1400),(5640,40,2800))]:block(label,p,size,'M_Studio_ChromaGreen')
block('Studio roof',(0,0,2820),(5640,4440,40),'M_Studio_Charcoal')
block('Studio rear shell',(0,2240,1400),(5640,30,2800),'M_Studio_Charcoal')
block('Green floor return',(0,1950,0),(5500,450,4),'M_Studio_Charcoal').set_actor_label('Neutral rear floor return')
# Neutral earth under every house and prop, no chroma or grass floor.
block('Mill scenic platform',(-600,900,10),(4400,2300,20),'M_Studio_PracticalEarth')
block('Tree scenic island',(1250,-500,5),(1100,750,10),'M_Studio_PracticalEarth')
# Main acting apron, followed by a 9 m shallow ramp with ample space for two people.
block('Mill acting apron',(1200,500,30),(700,600,60),'M_Studio_PracticalEarth')
ramp=block('Practical downhill path',(1200,-150,30),(600,math.hypot(900,50),12),'M_Studio_PracticalEarth')
ramp.set_actor_rotation(u.Rotator(roll=math.degrees(math.atan2(50,900))),False)
# Walkable 1:10 access ramps for player and staff, avoiding vertical platform edges.
r=block('Crew access ramp',(1100,1000,40),(650,math.hypot(400,40),12),'M_Studio_PracticalEarth');r.set_actor_rotation(u.Rotator(roll=-math.degrees(math.atan2(40,400))),False)
# Tree near the end of the shortened walk, with trunk outside both acting lanes.
tree=by['Practical promise tree'];p=tree.get_actor_location();place(tree,(p.x+600,p.y-990,p.z))
# Properly sized bench; keep it outside all character routes.
block('Mill-side bench seat',(1530,430,105),(180,55,14),'M_Studio_PracticalEarth')
block('Bench back',(1530,458,140),(180,12,70),'M_Studio_PracticalEarth')
legs=[a for a in actors if a.get_actor_label()=='Bench leg']
for a,x in zip(legs,[1460,1600]):place(a,(x,430,80));a.set_actor_scale3d(u.Vector(.12,.45,.4))
# Relocate filming equipment to perimeter lanes, outside all action and crew routes.
for a in list(aes.get_all_level_actors()):
 label=a.get_actor_label()
 if label.startswith('Tracking marker') or label=='Screen support upright':
  p=a.get_actor_location();p.y+=1020;place(a,(p.x,p.y,p.z+400))
 if label=='Overhead lighting bar':
  p=a.get_actor_location();place(a,(p.x,p.y,2600));a.set_actor_scale3d(u.Vector(53,.14,.14))
 if label in ['Lighting stand','Lighting stand base','Softbox housing','Softbox diffusion','Studio soft light']:
  p=a.get_actor_location();p.x= -2450 if p.x<0 else 2450 if p.x>0 else 0;p.y=-1000 if p.y<0 else 2000;place(a,(p.x,p.y,p.z))
 if label in ['Boom stand','Boom pole','Boom microphone']:
  p=a.get_actor_location();place(a,(p.x+1450,p.y-500,p.z))
 if label in ['Monitor housing','Monitor screen','Video village table','Table leg','Equipment case']:
  p=a.get_actor_location();place(a,(p.x,p.y-700,p.z))
 if label in ['Dolly rail','Rail cross tie']:
  p=a.get_actor_location();place(a,(p.x,p.y-700,p.z))
 if label=='Actor floor tape':aes.destroy_actor(a)
# Broad overhead fixtures light the fully closed room.
for x in [-1600,0,1600]:
 for y in [-1300,900]:
  a=spawn(u.RectLight,'Ceiling soft light',(x,y,2500));a.set_actor_rotation(u.Rotator(pitch=-90),False)
  c=a.get_component_by_class(u.RectLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_intensity(18000);props(c,attenuation_radius=6000.,source_width=1000.,source_height=800.,cast_shadows=False,affect_translucent_lighting=False)
for a in aes.get_all_level_actors():
 if isinstance(a,u.PostProcessVolume):
  settings=a.get_editor_property('settings');props(settings,override_auto_exposure_min_brightness=True,override_auto_exposure_max_brightness=True,auto_exposure_min_brightness=5.,auto_exposure_max_brightness=5.);a.set_editor_property('settings',settings)
# Explicit blocking: separated approach lanes, no crossing through Smith or the bench.
cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
hero=cast['Romeo'];juliet=cast['Juliet'];old=cast['Mr. Smith']
def key(a,t,p,yaw):
 w=spawn(u.DCWaypoint,a.get_actor_label()+' @ '+str(t),p);w.set_folder_path('Act1/Blocking');w.set_actor_rotation(u.Rotator(yaw=yaw),False);props(w,performer=a,time=float(t))
def route(a,keys):
 place(a,keys[0][1],keys[0][2]);props(a,use_waypoint_height=True,linear_movement=True)
 for t,p,y in keys:key(a,t,p,y)
route(hero,[(0,(1080,150,62),90),(2.4,(1100,300,62),45),(7,(1100,300,62),-90),(12,(1100,-600,12),0),(16,(1100,-600,12),0),(17.5,(1120,-600,12),0),(20,(1120,-600,12),0)])
route(juliet,[(0,(1450,50,62),140),(4,(1450,50,62),140),(6,(1300,300,62),180),(7,(1300,300,62),-90),(11.5,(1300,-510,17),-90),(12.5,(1200,-600,12),180),(16,(1200,-600,12),180),(17.5,(1190,-600,12),180),(20,(1190,-600,12),180)])
route(old,[(0,(1240,470,62),-115),(20,(1240,470,62),-115)])
# Crew stay in the open front/side lanes. One takes the gentle access ramp to the apron.
crew=[a for n,a in cast.items() if n not in ['Romeo','Juliet','Mr. Smith']]
route(crew[0],[(0,(-600,-800,2),0),(5,(-300,-800,2),0),(10,(100,-800,2),0),(15,(400,-800,2),180),(20,(100,-800,2),180)])
route(crew[1],[(0,(1850,1000,22),180),(5,(1100,1200,22),-90),(9,(1100,800,62),-90),(12,(1000,550,62),180),(16,(1100,800,62),90),(20,(1100,1200,22),90)])
for a,times in [(hero,[0,2.4,3.4,7,12,16,17.5]),(juliet,[0,4,6,7,12.5,16,17.5]),(old,[0,3.5,5,6.5,8])]:
 cues=list(a.get_editor_property('animation_cues'))
 for c,t in zip(cues,times):props(c,start_time=float(t),play_rate=1.)
 props(a,animation_cues=cues)
for a in crew:
 cues=list(a.get_editor_property('animation_cues'))
 for c in cues:props(c,start_time=c.get_editor_property('start_time')*20/15,play_rate=1.)
 props(a,animation_cues=cues)
props(hero,set_down_time=3.4,set_down_transform=u.Transform(location=u.Vector(1530,420,145),rotation=u.Rotator(),scale=u.Vector(.35,.35,.35)))
windows={'1.1':(1,5,3.5),'1.2':(6,10.5,7),'1.3':(12.5,16,12.5),'1.4':(17,20,17.5)}
for a in actors:
 if isinstance(a,u.DCBeat):
  ident=str(a.get_editor_property('beat_id'));s,e,r=windows[ident];props(a,window_start=float(s),window_end=float(e),pose_ready_time=float(r),required_hold=1.)
  place(a,(1150,-600,12) if ident in ['1.3','1.4'] else (1200,300,62))
 if isinstance(a,u.DCZombieDirector):props(a,take_duration=20.);a.set_actor_label('Act 1 - The Promise - enclosed studio (20 seconds)')
 if isinstance(a,u.DCStartMark):place(a,(900,-1400,100),90)
 if isinstance(a,u.DCClapperboard):place(a,(650,-1350,0),90)
for a in cast.values():a.evaluate(0.)
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
u.log('EXPANDED_STUDIO_SAVED')
