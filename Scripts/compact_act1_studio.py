"""Replace studio landscape with a compact flat stage and a sampled timber acting path."""
import unreal as u, math, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
base='/Game/DirectorsCut/Levels/'
les=u.get_editor_subsystem(u.LevelEditorSubsystem)
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
assert les.load_level(base+'L_DC_Act1_Promise')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
ground=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Ground')
studio=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Set')
u.EditorLevelUtils.set_level_visibility(ground.get_loaded_level(),True,False)
actors=list(aes.get_all_level_actors())
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
    hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
    assert hit[0],(x,y)
    return hit[4].z
floor=1050.; cx=-6750.;cy=-9200.;width=5000.;depth=3500.;roof=4100.
wood=u.load_asset('/Game/Stylized_Village/Assets/Architecture/Wood/_Materials/MI_Wood_PiecesPlanks')
green=u.load_asset('/Game/DirectorsCut/Materials/M_Studio_ChromaGreen')
cube=u.load_asset('/Engine/BasicShapes/Cube')
def reshape(a,p,size):
    a.set_actor_location(u.Vector(*p),False,False)
    a.set_actor_rotation(u.Rotator(),False)
    a.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
def block(label,p,size,material=wood):
    a=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*p))
    a.set_actor_label(label);a.set_folder_path('Act1/Studio/Timber practicals')
    a.tags=['DC_Studio','DC_TimberPractical']
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_material(0,material)
    a.static_mesh_component.set_collision_profile_name('BlockAll')
    reshape(a,p,size)
    return a
u.EditorLevelUtils.make_level_current(studio)
for a in actors:
    if 'DC_TimberPractical' in [str(t) for t in a.tags]:aes.destroy_actor(a);continue
    if '/L_DC_Act1_Set.' not in a.get_path_name() or a.get_attach_parent_actor():continue
    label=a.get_actor_label();p=a.get_actor_location()
    if label.startswith('Soundstage floor'):
        reshape(a,(cx,cy,floor-15),(width,depth,30));a.set_actor_label('Soundstage floor - compact 50m x 35m');a.static_mesh_component.set_material(0,green)
    elif label.startswith('Green screen -') or label=='Studio rear shell':
        z=(floor+roof)/2
        if label in ['Green screen - rear','Studio rear shell']:reshape(a,(cx,cy+depth/2+(40 if label=='Studio rear shell' else 0),z),(width+40,40,roof-floor))
        elif label=='Green screen - front':reshape(a,(cx,cy-depth/2,z),(width+40,40,roof-floor))
        elif label=='Green screen - left':reshape(a,(cx-width/2,cy,z),(40,depth,roof-floor))
        else:reshape(a,(cx+width/2,cy,z),(40,depth,roof-floor))
    elif label=='Studio roof':reshape(a,(cx,cy,roof+20),(width+40,depth+40,40))
    elif label.startswith('Tracking marker') or label=='Screen support upright':
        p.x=cx+(p.x+6650)*.78;p.y=cy+depth/2-30;p.z=max(floor+400,min(roof-200,p.z));a.set_actor_location(p,False,False)
    elif label=='Overhead lighting bar':reshape(a,(cx,max(cy-depth/2+200,min(cy+depth/2-200,p.y)),roof-100),(width-200,14,14))
    elif label.startswith('Ceiling soft light') or label.startswith('Studio broad fill'):
        p.x=max(cx-width/2+300,min(cx+width/2-300,p.x));p.y=max(cy-depth/2+300,min(cy+depth/2-300,p.y));p.z=roof-200;a.set_actor_location(p,False,False)
    elif isinstance(a,(u.StaticMeshActor,u.RectLight,u.SpotLight)):
        # Preserve assembled equipment offsets; lower their former ground baseline to the flat studio floor.
        p.z+=floor-height(p.x,p.y)
        p.x=max(cx-width/2+200,min(cx+width/2-200,p.x));p.y=max(cy-depth/2+200,min(cy+depth/2-200,p.y))
        a.set_actor_location(p,False,False)
# Narrow plank strips cover only the acting corridor, including the stationary Smith and kiss marks.
cast=[a for a in actors if isinstance(a,u.DCPerformer) and not a.get_editor_property('ruins_shot')]
points=[]
for a in cast:
    for i in range(201):
        a.evaluate(i*.1);p=a.get_actor_location();points.append((p.x,p.y))
    a.evaluate(0)
planks=0
for x in range(-6200,-4774,25):
    ys=[y for px,y in points if abs(px-x)<=110]
    if not ys:continue
    lo=math.floor((min(ys)-100)/50)*50;hi=math.ceil((max(ys)+100)/50)*50
    for y0 in range(lo,hi,50):
        y=y0+25;z=height(x,y)
        dx=(height(x+12.5,y)-height(x-12.5,y))/25
        dy=(height(x,y+25)-height(x,y-25))/50
        a=block('Path plank %03d'%planks,(x,y,z-4),(25,50,8))
        a.set_actor_rotation(u.Rotator(pitch=math.degrees(math.atan(dx)),roll=-math.degrees(math.atan(dy))),False)
        if x%150==0:
            h=z-8-floor
            block('Exposed timber riser',(x,y,floor+h/2),(12,48,max(1,h)))
        planks+=1
# Flat scenic plinths support the existing buildings, with no terrain outside their footprints.
for label in ['BP_Building9','BP_SmallHouse4','BP_Windmill2','BP_Cart_Wheat5','BP_Barrels_23','BP_WheatStack_15','BP_Mailbox5','Practical promise tree']:
    a=next(a for a in actors if a.get_actor_label()==label and not a.get_attach_parent_actor())
    c,e=a.get_actor_bounds(False,True);p=a.get_actor_location()
    # Tall mill bounds include sails; its small base needs only a compact riser.
    sx,sy=(420,420) if label=='BP_Windmill2' else (180,180) if label=='Practical promise tree' else (e.x*2,e.y*2)
    h=max(10,p.z-floor)
    block('Scenic timber base - '+label,(p.x if label in ['BP_Windmill2','Practical promise tree'] else c.x,p.y if label in ['BP_Windmill2','Practical promise tree'] else c.y,floor+h/2),(sx,sy,h))
# Bench stands beside the high end of the path.
bench=next(a for a in actors if a.get_actor_label()=='Mill-side bench seat')
p=bench.get_actor_location();z=height(p.x,p.y)
block('Bench timber base',(p.x,p.y,floor+(z-floor)/2),(100,220,z-floor))
# Crew traverse the flat stage; principal performances retain every original height and timing.
for a in actors:
    crew=isinstance(a,u.DCPerformer) and a.get_editor_property('ruins_shot')
    waypoint=isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') and a.get_editor_property('performer').get_editor_property('ruins_shot')
    if crew or waypoint or isinstance(a,(u.DCStartMark,u.DCClapperboard)):
        p=a.get_actor_location();p.z=floor+(100 if isinstance(a,u.DCStartMark) else 2);a.set_actor_location(p,False,False)
for a in actors:
    if isinstance(a,u.DCPerformer):a.evaluate(0)
ground.set_editor_property('initially_visible',False);ground.set_editor_property('should_be_visible',False)
u.EditorLevelUtils.set_level_visibility(ground.get_loaded_level(),False,False)
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
assert u.EditorLoadingAndSavingUtils.save_map(world,base+'L_DC_Act1_Promise')
(root/'Saved/FilmSetAudit/compact_studio_build.json').write_text(json.dumps({'width_m':50,'depth_m':35,'floor_z':floor,'planks':planks,'area_reduction_percent':100*(1-width*depth/(5900*4850))},indent=2))
u.log('COMPACT_STUDIO_SAVED')
