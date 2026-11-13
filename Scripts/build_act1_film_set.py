"""Convert Act 1 to a 30 x 24 m practical green-screen stage. Run with the map unloaded."""
import unreal as u, json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
LEVEL='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level('/Game/Levels/L_DC_Zombie')
reference=[]
for a in aes.get_all_level_actors():
    if a.get_actor_label()=='Ground':
        c,e=a.get_actor_bounds(False);reference.append({'label':'Ground','size_cm':[e.x*2,e.y*2,e.z*2]})
assert les.load_level(LEVEL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors())
assert not any('DC_Studio' in [str(t) for t in a.tags] for a in actors),'Already converted; restore the pre-studio map to rebuild.'
cast={a.get_actor_label():a for a in actors if isinstance(a,u.DCPerformer)}
assert {'Romeo','Juliet','Mr. Smith'}<=cast.keys()
beats=[a for a in actors if isinstance(a,u.DCBeat)]
director=next(a for a in actors if isinstance(a,u.DCZombieDirector))
start=next(a for a in actors if isinstance(a,u.DCStartMark))
clapper=next(a for a in actors if isinstance(a,u.DCClapperboard))
practical={label:next(a for a in actors if a.get_actor_label()==label) for label in ['BP_Windmill2','BP_SmallHouse4','BP_WheatStack_15']}
bench=[a for a in actors if a.get_actor_label() in ['Mill-side bench seat','Bench leg','Bench back']]
keep=set(cast.values())|set(beats)|{director,start,clapper}|set(practical.values())|set(bench)
removed=0
for a in actors:
    if a in keep or isinstance(a,(u.WorldSettings,u.Brush)):continue
    if aes.destroy_actor(a):removed+=1
def place(a,xyz,yaw=0):
    a.set_actor_location_and_rotation(u.Vector(*xyz),u.Rotator(yaw=yaw),False,False)
def spawn(cls,label,xyz,yaw=0,folder='Act1/Studio'):
    a=aes.spawn_actor_from_class(cls,u.Vector(*xyz),u.Rotator(yaw=yaw));assert a
    a.set_actor_label(label);a.set_folder_path(folder);a.tags=['DC_Studio'];return a
def material(name,color,unlit=False):
    path='/Game/DirectorsCut/Materials/'+name
    m=u.load_asset(path)
    if not m:m=u.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/DirectorsCut/Materials',u.Material,u.MaterialFactoryNew())
    u.MaterialEditingLibrary.delete_all_material_expressions(m)
    c=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector,-300,0)
    c.set_editor_property('constant',u.LinearColor(*color,1))
    if unlit:
        m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
        u.MaterialEditingLibrary.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        u.MaterialEditingLibrary.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
        r=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant,-300,150);r.set_editor_property('r',.85)
        u.MaterialEditingLibrary.connect_material_property(r,'',u.MaterialProperty.MP_ROUGHNESS)
    u.MaterialEditingLibrary.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m,False);return m
green=material('M_Studio_ChromaGreen',(.025,.42,.065),True)
black=material('M_Studio_Charcoal',(.018,.023,.03))
metal=material('M_Studio_Equipment',(.07,.085,.10))
dirt=material('M_Studio_PracticalEarth',(.24,.15,.085))
grass=material('M_Studio_PracticalGrass',(.16,.22,.07))
white=material('M_Studio_Diffusion',(.85,.88,.9),True)
mark=material('M_Studio_TrackingMark',(.035,.24,.055),True)
screen=material('M_Studio_Monitor',(.025,.10,.12),True)
def block(label,xyz,size,mat,yaw=0,mesh='/Engine/BasicShapes/Cube',folder='Act1/Studio'):
    a=spawn(u.StaticMeshActor,label,xyz,yaw,folder);a.static_mesh_component.set_static_mesh(u.load_asset(mesh))
    a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
    if mat:a.static_mesh_component.set_material(0,mat)
    return a
block('Soundstage floor - 30m x 24m',(0,0,-12),(3000,2400,24),black)
# Three green walls, green floor return, and exposed frames make the set boundary obvious.
for label,pos,size in [('Green screen - rear',(0,1180,750),(3000,30,1500)),('Green screen - left',(-1480,0,750),(30,2400,1500)),('Green screen - right',(1480,0,750),(30,2400,1500))]:block(label,pos,size,green)
block('Green floor return',(0,940,2),(2920,450,4),green)
block('Studio rear shell',(0,1225,800),(3100,40,1600),black)
block('Studio roof',(0,0,1640),(3100,2500,35),black)
for x in [-1450,-725,0,725,1450]:
    block('Screen support upright',(x,1210,775),(12,12,1550),metal)
    for z in [400,800,1200]:
        block('Tracking marker horizontal',(x,1162,z),(24,2,4),mark)
        block('Tracking marker vertical',(x,1161,z),(4,2,24),mark)
for y in [-900,100,1100]:block('Overhead lighting bar',(0,y,1480),(2900,14,14),metal)
# Practical scenic apron and shallow descending path. Front edges stay visible as built scenery.
block('Mill scenic platform',(-950,250,30),(1000,1050,60),dirt,folder='Act1/Practical scenery')
block('Tree scenic island',(850,300,5),(650,750,10),grass,folder='Act1/Practical scenery')
rise=50;length=1300;pitch=math.degrees(math.atan2(-rise,length))
ramp=block('Practical downhill path',(200,0,30),(math.hypot(length,rise),340,12),dirt,folder='Act1/Practical scenery')
ramp.set_actor_rotation(u.Rotator(pitch=pitch),False)
def fit(a,label,xy,height,base):
    a.set_actor_scale3d(u.Vector(1,1,1));place(a,(0,0,0))
    c,e=a.get_actor_bounds(False);s=height/(2*e.z);a.set_actor_scale3d(u.Vector(s,s,s))
    c,e=a.get_actor_bounds(False);place(a,(xy[0]-c.x,xy[1]-c.y,base-c.z+e.z))
    a.set_actor_label(label);a.set_folder_path('Act1/Practical scenery');a.tags=['DC_Studio']
fit(practical['BP_Windmill2'],'Practical windmill',(-950,500),1100,60)
fit(practical['BP_SmallHouse4'],'Practical village house',(-50,700),560,10)
aes.destroy_actor(practical['BP_WheatStack_15'])
for i,(x,y) in enumerate([(-650,340),(-510,390),(-570,270)]):
    wheat=spawn(u.StaticMeshActor,'Practical wheat bundle '+str(i),(x,y,60),folder='Act1/Practical scenery')
    wheat.static_mesh_component.set_static_mesh(u.load_asset('/Game/WindmillValley/Meshes/SM_Wheat_01'))
    fit(wheat,'Practical wheat bundle '+str(i),(x,y),170,60)
tree=spawn(u.StaticMeshActor,'Practical promise tree',(850,450,10),folder='Act1/Practical scenery')
tree.static_mesh_component.set_static_mesh(u.load_asset('/Game/WindmillValley/Meshes/SM_Tree_02'))
fit(tree,'Practical promise tree',(950,500),750,10)
for a in bench:
    label=a.get_actor_label()
    if label=='Mill-side bench seat':p=(-580,40,107)
    elif label=='Bench back':p=(-580,66,137)
    else:p=(-640 if a.get_actor_location().y>-8550 else -520,40,81)
    place(a,p);a.set_folder_path('Act1/Practical scenery')
# Retain original character assets and animation clips; author only compact blocking.
def props(a,**kw):
    for k,v in kw.items():a.set_editor_property(k,v)
def key(a,t,x,y,yaw,z):
    w=spawn(u.DCWaypoint,a.get_actor_label()+' @ '+str(t),(x,y,z),yaw,'Act1/Blocking');props(w,performer=a,time=t)
def height(x):return 62 if x<=-450 else 12 if x>=850 else 62-(x+450)/26
hero=cast['Romeo'];juliet=cast['Juliet'];old=cast['Mr. Smith']
place(hero,(-850,-180,62),30);place(old,(-610,-130,62),135);place(juliet,(-250,-160,height(-250)),180)
for a,base in [(hero,[0,4.8,6.8,14,23,31,34]),(juliet,[0,10,12.6,14,23,31,34]),(old,[0,7,10,13,16])]:
    cues=list(a.get_editor_property('animation_cues'))
    for c,t in zip(cues,base):c.set_editor_property('start_time',t)
    props(a,animation_cues=cues,use_waypoint_height=True,linear_movement=True)
for t in [4.8,6.8,13.9,14]:key(hero,t,-650,-100,0,62)
key(juliet,10,-250,-160,180,height(-250))
for t in [12.6,13.9,14]:key(juliet,t,-650,-20,0,62)
for i in range(1,36):
    t=14+i*.25;f=(t-14)/9;x=-650+1500*f;y=-100+200*f
    for a,dy in [(hero,0),(juliet,80)]:key(a,t,x,y+dy,7.6,height(x))
for a,dx,yaw in [(hero,-40,0),(juliet,40,180)]:
    for t in [23,30.8]:key(a,t,850+dx,100,yaw,12)
    for t in [34,40]:key(a,t,850+(-17 if a==hero else 17),100+(3 if a==juliet else 0),yaw,12)
props(hero,set_down_transform=u.Transform(location=u.Vector(-580,5,135),rotation=u.Rotator(),scale=u.Vector(.35,.35,.35)))
crews=[a for name,a in cast.items() if name not in ['Romeo','Juliet','Mr. Smith']]
for i,a in enumerate(crews):
    place(a,(-250+i*650,-500,2),90)
    for t,x,y in [(10,-250+i*650,-500),(20,-200+i*650,-200),(30,50+i*650,-400),(40,-250+i*650,-500)]:key(a,t,x,y,90,2)
    cues=list(a.get_editor_property('animation_cues'))
    for c,t in zip(cues,[0,10,15,25] if i==0 else [0,18,28,33]):c.set_editor_property('start_time',t)
    props(a,animation_cues=cues)
for b in beats:
    id=str(b.get_editor_property('beat_id'));last=id in ['1.3','1.4'];place(b,(850,100,12) if last else (-650,-100,62))
    if id=='1.3':props(b,window_start=22.,window_end=29.,pose_ready_time=24.)
    if id=='1.4':props(b,window_start=31.,window_end=38.,pose_ready_time=33.5)
props(director,take_duration=40.);director.set_actor_label('Act 1 - The Promise - practical studio (40 seconds)');place(director,(0,-1100,0))
place(start,(0,-850,100),90);place(clapper,(-180,-780,0),90)
# Off-camera equipment: softboxes, visible stands, boom, short dolly rails, and video village.
for x,y,yaw in [(-1250,-650,30),(1250,-650,150),(0,900,-90)]:
    block('Lighting stand',(x,y,280),(12,12,560),metal)
    block('Lighting stand base',(x,y,10),(110,90,20),metal)
    block('Softbox housing',(x,y,575),(180,35,110),black,yaw)
    block('Softbox diffusion',(x,y-20,575),(165,4,95),white,yaw)
    a=spawn(u.RectLight,'Studio soft light',(x,y-40,550),yaw)
    a.set_actor_rotation(u.Rotator(pitch=-35,yaw=yaw),False)
    c=a.get_component_by_class(u.RectLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_intensity(25000);c.set_editor_property('attenuation_radius',3000.)
    c.set_editor_property('source_width',160.);c.set_editor_property('source_height',100.)
fill=spawn(u.DirectionalLight,'Studio broad fill',(0,0,1400));fill.set_actor_rotation(u.Rotator(pitch=-60,yaw=-25),False)
fill.get_component_by_class(u.DirectionalLightComponent).set_intensity(3.)
for x in [-70,70]:block('Dolly rail',(x,-650,6),(6,700,12),metal)
for y in [-950,-800,-650,-500,-350]:block('Rail cross tie',(0,y,3),(180,12,6),black)
block('Video village table',(-1000,-950,90),(280,100,12),metal)
for x in [-1120,-880]:block('Table leg',(x,-950,45),(10,75,90),metal)
for x in [-1065,-935]:
    block('Monitor housing',(x,-950,145),(100,18,70),black)
    block('Monitor screen',(x,-961,145),(88,2,58),screen)
block('Boom stand',(650,-650,240),(9,9,480),metal)
boom=block('Boom pole',(650,-400,510),(8,560,8),metal);boom.set_actor_rotation(u.Rotator(roll=12),False)
block('Boom microphone',(650,-130,565),(14,70,14),black)
for x in [-1300,1300]:block('Equipment case',(x,-1000,45),(150,100,90),black)
for x,y in [(-650,-100),(850,100)]:
    block('Actor floor tape',(x,y-45,height(x)+1),(30,4,1),white)
world.get_world_settings().set_editor_property('default_game_mode',u.load_class(None,'/Script/EpicAIMegaJam2026.DCZombieGameMode'))
for a in cast.values():a.evaluate(0.)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# Validate real playback, 4 scored beats, rewind, and absence of the valley/foliage systems.
def sample(t):
    rows={}
    for name,a in cast.items():
        a.evaluate(t);p=a.get_actor_location();assert abs(p.x)<1450 and abs(p.y)<1180
        rows[name]=[p.x,p.y,p.z]
    return rows
initial=sample(0);samples={str(t):sample(t) for t in [8,15,20,24,33.5,36,40]};rewind=sample(0)
assert max(abs(x-y) for name in initial for x,y in zip(initial[name],rewind[name]))<.01
for b in beats:assert max(b.get_editor_property('window_start'),b.get_editor_property('pose_ready_time'))+b.get_editor_property('required_hold')<=b.get_editor_property('window_end')<=40
assert len(beats)==4
assert not any(isinstance(a,(u.LandscapeProxy,u.InstancedFoliageActor)) for a in aes.get_all_level_actors())
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
report={'reference':reference,'studio_floor_cm':[3000,2400],'duration':40,'removed_environment_actors':removed,'cast_samples':samples,'capture_beats':4,'rewind_passed':True,'green_screen_replacement':'not implemented as requested','map':LEVEL}
(ROOT/'Saved/FilmSetAudit/build_report.json').write_text(json.dumps(report,indent=2))
u.log('ACT1_FILM_SET_VALIDATED')
