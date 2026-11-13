"""Build the editable, silent 40-second Promise scene in a saved L_Showcase copy."""
import unreal as u, json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/DirectorsCut';LEVEL=BASE+'/Levels/L_DC_Act1_Promise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
current=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if 'L_DC_Act1_Promise' not in current.get_path_name():
    les.load_level('/Game/WindmillValley/Levels/L_Showcase')
else:
    for previous in list(aes.get_all_level_actors()):
        if 'DC_Act1' in [str(t) for t in previous.tags]:aes.destroy_actor(previous)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
original=list(aes.get_all_level_actors())
# Clear existing boulders only where they intersect the performance deck in this copy.
cleared=[]
for a in original:
    if isinstance(a,u.StaticMeshActor) and 'rock' in a.get_actor_label().lower():
        center,extent=a.get_actor_bounds(False)
        if max(extent.x,extent.y)<1000 and abs(center.x-3700)<extent.x+180 and abs(center.y-3300)<extent.y+160:
            cleared.append(a.get_actor_label());aes.destroy_actor(a)
original=list(aes.get_all_level_actors())
ignore=[a for a in original if not isinstance(a,u.LandscapeProxy)]
def ground(x,y):
    h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,9000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
    assert h[0],(x,y)
    return h[4].z
def v(x,y,z=None):return u.Vector(x,y,ground(x,y)+3 if z is None else z)
def cls(name):return u.load_class(None,'/Script/EpicAIMegaJam2026.'+name)
def asset(p):
    a=u.load_asset(p);assert a,p
    return a
def spawn(c,label,loc,yaw=0,folder='Act1/Set'):
    a=aes.spawn_actor_from_class(c,loc,u.Rotator(yaw=yaw));assert a,label
    a.set_actor_label(label);a.set_folder_path(folder);a.tags=['DC_Act1']
    return a
def props(a,**kwargs):
    for k,value in kwargs.items():a.set_editor_property(k,value)
def static(label,mesh,loc,scale=(1,1,1),yaw=0,mat=None):
    a=spawn(u.StaticMeshActor,label,loc,yaw)
    c=a.static_mesh_component;c.set_static_mesh(asset(mesh));a.set_actor_scale3d(u.Vector(*scale))
    if mat:c.set_material(0,asset(mat))
    return a
def clip(who,name):return asset(BASE+'/Animations/'+who+'/A_'+who+'_'+name)
def cue(time,who,name,loop=True,rate=1):
    c=u.DCAnimationCue();props(c,start_time=time,animation=clip(who,name),loop=loop,play_rate=rate)
    return c
def actor(role,label,loc,yaw,cues,crew=False):
    a=spawn(cls('DCPerformer'),label,loc,yaw,'Act1/Cast' if not crew else 'Act1/Crew')
    props(a,display_name=label,character_mesh=asset(BASE+'/Characters/'+role+'/SK_'+role),
          mesh_offset=u.Transform(rotation=u.Rotator(yaw=-90)),head_bone='head_joint',body_bone='chest_joint',
          use_waypoint_height=True,linear_movement=True,animation_cues=cues,ruins_shot=crew)
    return a
def key(a,t,x,y,yaw,z=None):
    w=spawn(cls('DCWaypoint'),a.get_actor_label()+' @ '+str(t),v(x,y,z),yaw,'Act1/Blocking')
    props(w,performer=a,time=t)
def route(a,points):
    loc=a.get_actor_location();prev=(0,loc.x,loc.y,a.get_actor_rotation().yaw,None)
    for t,x,y,yaw,z in points:
        pt,px,py,pyaw,pz=prev
        if z is None and pz is None and math.hypot(x-px,y-py)>50:
            for j in range(1,max(1,math.ceil((t-pt)*4))):
                alpha=j/max(1,math.ceil((t-pt)*4))
                turn=(yaw-pyaw+180)%360-180
                key(a,pt+(t-pt)*alpha,px+(x-px)*alpha,py+(y-py)*alpha,pyaw+turn*alpha)
        key(a,t,x,y,yaw,z);prev=(t,x,y,yaw,z)
def transform(loc,yaw=0,scale=(1,1,1)):return u.Transform(location=loc,rotation=u.Rotator(yaw=yaw),scale=u.Vector(*scale))

# A mill crossing bench and the village pack's recognisable crate.
wood='/Game/Stylized_Village/Assets/Architecture/Wood/_Materials/MI_Wood_PiecesPlanks'
cube='/Engine/BasicShapes/Cube'
benchz=ground(4100,2010)
static('Mill-side bench seat',cube,v(4100,2010,benchz+47),(1.7,.48,.12),0,wood)
for x in [4040,4160]:static('Bench leg',cube,v(x,2010,benchz+21),(.12,.4,.42),0,wood)
static('Bench back',cube,v(4100,2036,benchz+77),(1.7,.08,.48),0,wood)
crate='/Game/Stylized_Village/Assets/Props/Furniture/SM_Crate_01'

hero=actor('SwordsmanAct1','Hero - BasicHero_M_Swordsman',v(3850,1420),62,[
    cue(0,'Swordsman','CarryWalk'),cue(4.8,'Swordsman','SetDown',False),cue(6.8,'Swordsman','PromiseLook'),
    cue(14,'Swordsman','Unarmed_WalkForward'),cue(23,'Swordsman','PromiseLook'),cue(31,'Swordsman','KissLean',False),cue(34,'Swordsman','ForeheadHold')])
props(hero,has_prop=True,carried_mesh=asset(crate),carry_transform=transform(u.Vector(40,0,112),0,(.35,.35,.35)),
      set_down_time=6.2,set_down_transform=transform(v(4100,1975,benchz+53+21.5),0,(.35,.35,.35)))
old=actor('OldMan','Old villager - Mage',v(4136,1940),225,[cue(0,'Swordsman','PromiseLook'),cue(7,'Swordsman','ShoulderPat',False),cue(10,'Swordsman','PromiseLook'),cue(13,'Swordsman','Wave',False),cue(16,'Swordsman','PromiseLook')])
heroine=actor('ArcherAct1','Heroine - BasicHero_F_Archer',v(4550,1620),145,[cue(0,'Archer','PromiseLook'),cue(10,'Archer','Unarmed_WalkForward'),cue(12.6,'Archer','PromiseLook'),cue(14,'Archer','Unarmed_WalkForward'),cue(23,'Archer','PromiseLook'),cue(31,'Archer','KissLean',False),cue(34,'Archer','ForeheadHold')])

# Compact uphill blocking: no teleport between the kindness and promise beats.
lookz=max(ground(3600,3220),ground(3800,3220),ground(3600,3380),ground(3800,3380))+12
static('Lookout timber deck',cube,v(3700,3300,lookz-10),(3.2,2.6,.2),0,wood)
for x in [3570,3830]:
    for y in [3190,3410]:
        g=ground(x,y);static('Lookout support',cube,v(x,y,(g+lookz)/2),(.16,.16,max(.1,(lookz-g)/100)),0,wood)
points=[(14,4100,1900,90,None),(16,4100,2200,100,None),(18,4020,2500,90,None),(23,3660,3300,0,lookz)]
route(hero,[(4.8,4100,1900,90,None),(6.8,4100,1900,90,None),(13.9,4100,1900,90,None)]+points+[(30.8,3660,3300,0,lookz),(34,3683,3300,0,lookz),(40,3683,3300,0,lookz)])
route(heroine,[(10,4550,1620,145,None),(12.6,4170,1860,180,None),(13.9,4170,1860,90,None)]+[(t,x+80,y, yaw,z) for t,x,y,yaw,z in points[:-1]]+[(23,3740,3300,180,lookz),(30.8,3740,3300,180,lookz),(34,3717,3303,180,lookz),(40,3717,3303,180,lookz)])

# A walkable board ramp bridges the last rise to the level lookout deck.
start=v(3820,2800);end=v(3700,3200,lookz)
delta=end-start;length=delta.length();mid=(start+end)*.5
ramp=static('Lookout access ramp',cube,mid,(length/100,1.8,.14),math.degrees(math.atan2(delta.y,delta.x)),wood)
ramp.set_actor_rotation(u.Rotator(pitch=math.degrees(math.atan2(delta.z,math.hypot(delta.x,delta.y))),yaw=math.degrees(math.atan2(delta.y,delta.x))),False)
# Sample the ramp rather than letting feet pass through it.
for a,dx in [(hero,0),(heroine,80)]:
    key(a,20,3820+dx,2800,90,start.z)
    key(a,21,3760+dx,3000,90,(start.z+lookz)/2)
    key(a,22,3700+dx,3200,90,lookz)

crew1=actor('Crew','Crew - sound assistant',v(4500,2050),180,[cue(0,'Swordsman','Unarmed_WalkForward'),cue(10,'Swordsman','Unarmed_Idle'),cue(15,'Swordsman','Unarmed_WalkForward'),cue(25,'Swordsman','Unarmed_Idle')],True)
route(crew1,[(5,4250,1860,200,None),(10,3820,1740,210,None),(15,3820,1740,35,None),(25,4500,2300,35,None),(40,4500,2300,180,None)])
crew2=actor('Crew','Crew - hillside runner',v(3500,2850),70,[cue(0,'Swordsman','Unarmed_Idle'),cue(18,'Swordsman','Unarmed_WalkForward'),cue(28,'Swordsman','Unarmed_Idle'),cue(33,'Swordsman','Unarmed_WalkForward')],True)
route(crew2,[(18,3500,2850,70,None),(24,3500,3200,35,None),(28,3650,3520,30,None),(33,3650,3520,220,None),(40,3500,2850,220,None)])

def beat(id,title,caption,start,end,ready,actors,loc,body=False):
    b=spawn(cls('DCBeat'),id+' '+title,loc,0,'Act1/Story beats')
    refs=[]
    for a in actors:
        ref=u.DCAnchorRef();props(ref,performer=a,part=u.DCAnchorPart.BODY if body else u.DCAnchorPart.HEAD,must_see_face=False);refs.append(ref)
    props(b,beat_id=id,display_name=title,caption=caption,mandatory=True,window_start=start,window_end=end,pose_ready_time=ready,required_hold=2.,max_distance=2500.,anchors=refs)
    return b
beat('1.1','A small kindness','The box is down; the old man pats his shoulder.',2,10,7.6,[hero,old],v(4100,1900),True)
beat('1.2','She sees his kindness','The couple leave together as the villager waves.',11,19,14,[hero,heroine],v(4100,2200),True)
beat('1.3','Always together','They exchange a quiet promise at the lookout.',22,29,24,[hero,heroine],v(3700,3300,lookz))
beat('1.4','The kiss','They lean together and hold forehead to forehead.',31,38,33.5,[hero,heroine],v(3700,3300,lookz))
director=spawn(cls('DCZombieDirector'),'Stage 1 - The Promise (40 seconds)',v(4200,1700),0,'Act1/Direction')
props(director,take_duration=40.,movie_title='The Promise - Stage 1',max_cameras=0)
spawn(cls('DCStartMark'),'Camera operator start',v(4550,1700,ground(4550,1700)+100),150,'Act1/Direction')
spawn(cls('DCClapperboard'),'Stage 1 clapperboard',v(4450,1780),0,'Act1/Direction')
world.get_world_settings().set_editor_property('default_game_mode',cls('DCZombieGameMode'))
for label,pos in [('Mill-side sunset bounce',v(4350,1800,2050)),('Lookout sunset bounce',v(3950,3150,lookz+420))]:
    light=spawn(u.PointLight,label,pos,0,'Act1/Lighting')
    component=light.get_component_by_class(u.PointLightComponent)
    component.set_mobility(u.ComponentMobility.MOVABLE)
    component.set_intensity(120.)
    component.set_light_color(u.LinearColor(1.,.82,.65,1.))
    component.set_editor_property('attenuation_radius',1500.)
    component.set_cast_shadows(False)
for a in [hero,heroine,old,crew1,crew2]:a.evaluate(0.)
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(v(4750,1400,2000),u.Rotator(pitch=-12,yaw=145))
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
report={'level':LEVEL,'duration':40,'cast':[a.get_path_name() for a in [hero,heroine,old,crew1,crew2]],'lookout_z':lookz,'crate':crate,'speech':False}
(ROOT/'Saved/Stage1Audit/staging_report.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_STAGING_COMPLETE '+str(report))
