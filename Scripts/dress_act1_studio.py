"""Dress the user's raised stage; keep shared scenery and principal blocking intact."""
import unreal as u,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert '/L_DC_Act1_Promise.' in world.get_path_name()
actors=list(aes.get_all_level_actors());by={a.get_actor_label():a for a in actors if not a.get_attach_parent_actor()}
assert not any('DC_DressedStudio' in [str(t) for t in a.tags] for a in actors),'Already dressed'
stage=next(a for a in actors if a.get_actor_label().startswith('Soundstage floor'))
c,e=stage.get_actor_bounds(False);floor=c.z+e.z
assert abs(floor-1080)<1,'Preserve and inspect any newer floor adjustment before applying'
studio=u.GameplayStatics.get_streaming_level(world,'L_DC_Act1_Set')
ignore=[a for a in actors if not isinstance(a,u.LandscapeProxy)]
def terrain(x,y):
    h=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,6000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
    assert h[0],(x,y)
    return h[4].z
principal={a.get_path_name():a.get_actor_transform().export_text() for a in actors if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer') and not a.get_editor_property('performer').get_editor_property('ruins_shot')}
scenery={a.get_path_name():a.get_actor_transform().export_text() for a in actors if 'DC_MillAssembly' in [str(t) for t in a.tags] or a.get_actor_label()=='Practical promise tree'}
u.EditorLevelUtils.make_level_current(studio)
cube=u.load_asset('/Engine/BasicShapes/Cube')
black=u.load_asset('/Game/DirectorsCut/Materials/M_Studio_Charcoal')
green=u.load_asset('/Game/DirectorsCut/Materials/M_Studio_ChromaGreen')
created=[]
def spawn(label,mesh,p,yaw=0,scale=(1,1,1)):
    a=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*p),u.Rotator(yaw=yaw))
    a.set_actor_label(label);a.set_folder_path('Act1/Studio/Filming equipment');a.tags=['DC_Studio','DC_DressedStudio']
    a.static_mesh_component.set_static_mesh(u.load_asset(mesh) if isinstance(mesh,str) else mesh)
    a.set_actor_scale3d(u.Vector(*scale));a.static_mesh_component.set_collision_profile_name('BlockAll')
    created.append(a);return a
def block(label,p,size,mat=black):
    a=spawn(label,cube,p,scale=tuple(v/100 for v in size));a.static_mesh_component.set_material(0,mat);return a
def floor_place(a,x,y,yaw=0,z=None):
    a.set_actor_rotation(u.Rotator(yaw=yaw),False);a.set_actor_location(u.Vector(x,y,0),False,False)
    c,e=a.get_actor_bounds(False,True);bottom=c.z-e.z
    a.set_actor_location(u.Vector(x,y,(floor if z is None else z)-bottom+.4),False,False)
    return a
def prop(label,path,x,y,yaw=0):return floor_place(spawn(label,path,(x,y,0)),x,y,yaw)
def group(labels,old_origin,new_origin,yaw=0):
    angle=math.radians(yaw)
    for a in actors:
        if '/L_DC_Act1_Set.' not in a.get_path_name() or a.get_actor_label() not in labels:continue
        p=a.get_actor_location();dx=p.x-old_origin[0];dy=p.y-old_origin[1]
        a.set_actor_location(u.Vector(new_origin[0]+dx*math.cos(angle)-dy*math.sin(angle),new_origin[1]+dx*math.sin(angle)+dy*math.cos(angle),new_origin[2]+p.z-old_origin[2]),False,False)
        r=a.get_actor_rotation();r.yaw+=yaw;a.set_actor_rotation(r,False)
# Trim five metres from the prop-side edge, retaining the user's floor elevation.
new_min_y=-10450.;new_cy=(-7450+new_min_y)/2;depth=3000.
for a in actors:
    label=a.get_actor_label();p=a.get_actor_location();s=a.get_actor_scale3d()
    if a==stage or label=='Studio roof':
        p.y=new_cy;s.y=(depth+(40 if label=='Studio roof' else 0))/100
        a.set_actor_location(p,False,False);a.set_actor_scale3d(s)
        if a==stage:a.set_actor_label('Soundstage floor - 50m x 30m raised studio')
    elif label in ['Green screen - left','Green screen - right']:
        p.y=new_cy;s.y=depth/100;a.set_actor_location(p,False,False);a.set_actor_scale3d(s)
    elif label=='Green screen - front':p.y=new_min_y;a.set_actor_location(p,False,False)
# A screen return joins the mill wall and closes the otherwise empty backstage alley.
block('Green screen - backstage return',(-6300,-10050,2375),(35,800,3450),green)
block('Green screen - backstage end',(-6500,-9650,2375),(435,35,3450),green)
# Re-home complete assembled props instead of scattering their constituent parts.
group({'Video village table','Table leg','Monitor housing','Monitor screen'},(-7650,-10575,1315.8878088),(-4500,-9660,floor),90)
group({'H3D - Jib','H3D - Crane Camera','H3D - Rails'},(-7100,-10250,1316.8878088),(-4750,-10150,floor))
# The rough duplicate rails are obsolete beside the imported rail rig.
for a in actors:
    if '/L_DC_Act1_Set.' in a.get_path_name() and a.get_actor_label() in ['Dolly rail','Rail cross tie']:aes.destroy_actor(a)
floor_place(by['H3D - Director Chair'],-4710,-9630,90)
floor_place(by['H3D - Big Box'],-5820,-10320,0)
box=by['H3D - Big Box'];bc,be=box.get_actor_bounds(False)
floor_place(by['H3D - Director Megaphone'],-5840,-10320,25,bc.z+be.z)
floor_place(by['H3D - Director Slate'],-5780,-10320,0,bc.z+be.z)
floor_place(by['H3D - Ladder'],-6050,-10345,90,max(floor,terrain(-6050,-10345)))
for a,p in zip([a for a in actors if a.get_actor_label()=='Equipment case'],[(-5580,-10320),(-4490,-10000)]):floor_place(a,*p)
floor_place(by['H3D - Light Tall Rectangular'],-5540,-9970,0)
floor_place(by['H3D - Light Medium Round'],-4440,-9310,150)
floor_place(by['H3D - Light Short Large'],-4480,-10330,130)
prop('Camera station - dark crane','/Game/DirectorsCut/Props/Hyper3D_MovieSet/CraneCameraV2/SM_CraneCamera_Dark',-5390,-10100,90)
hollywood='/Game/Hollywood_Vol3/Meshes/'
for label,mesh,x,y,yaw in [
    ('Production key light','SM_Production_Light_01a',-5650,-10100,45),
    ('Production fill light','SM_Production_Light_01b',-4460,-8780,175),
    ('Softbox beside video village','SM_Lightbox_01a',-4400,-9450,90),
    ('Grip stand - spare reflector','SM_Light_Stand_02b',-5680,-10330,15),
    ('Grip stand - spare lamp','SM_Light_Stand_02d',-5460,-10335,-20),
    ('Floor lamp beside dolly','SM_Stage_Lights_01a',-4910,-10320,140)]:prop(label,hollywood+mesh,x,y,yaw)
# Real truss segments along the equipment edge, with hanging lamps facing the practical set.
for x in [-6000,-5600,-5200,-4800]:
    a=spawn('Overhead filming truss',hollywood+'SM_Structural_Beam_01b',(x,-10380,1850))
    a.set_actor_rotation(u.Rotator(pitch=-90),False)
for x in [-6000,-4400]:
    prop('Lighting truss foot',hollywood+'SM_Structural_Beam_01f',x,-10380)
    a=prop('Lighting truss upright',hollywood+'SM_Structural_Beam_01b',x,-10380)
    a.set_actor_scale3d(u.Vector(1,1,1.9))
for x in [-5800,-5300,-4800]:
    a=spawn('Hanging production lamp',hollywood+'SM_Stage_Lights_01a',(x,-10370,1800),yaw=90)
# Floor markings organize the equipment bays while leaving a generous central walking aisle.
for x,y,length in [(-5460,-10395,500),(-4680,-10395,470),(-4360,-9650,300)]:
    block('Studio equipment bay tape',(x,y,floor+.15),(length,3,.2),u.load_asset('/Game/DirectorsCut/Materials/M_Studio_PracticalEarth'))
# Raised floor must support the operator, slate and crew; preserve principal coordinates and timing.
for a in actors:
    if isinstance(a,(u.DCStartMark,u.DCClapperboard)):
        p=a.get_actor_location();p.z=max(floor,terrain(p.x,p.y))+(100 if isinstance(a,u.DCStartMark) else 2);a.set_actor_location(p,False,False)
    performer=a if isinstance(a,u.DCPerformer) else a.get_editor_property('performer') if isinstance(a,u.DCWaypoint) else None
    if performer and performer.get_editor_property('ruins_shot'):
        p=a.get_actor_location()
        if performer.get_actor_label()=='Crew - sound assistant':p.x=-5070;p.y=-10000+(p.y+10550)*.6
        p.z=max(floor,terrain(p.x,p.y))+2;a.set_actor_location(p,False,False)
for a in aes.get_all_level_actors():
    if a.get_path_name() in principal:assert a.get_actor_transform().export_text()==principal[a.get_path_name()]
    if a.get_path_name() in scenery:assert a.get_actor_transform().export_text()==scenery[a.get_path_name()]
# Save only edited maps; preserve other imported assets and outstanding editor work.
u.EditorLevelLibrary.set_current_level_by_name('L_DC_Act1_Promise')
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,False)
report={'studio_size_m':[50,30],'floor_top_cm':floor,'new_props':len(created),'principal_waypoints_unchanged':len(principal),'shared_scenery_unchanged':len(scenery)}
(root/'Saved/FilmSetAudit/dressing_result.json').write_text(json.dumps(report,indent=2))
u.log('STUDIO_DRESSING_SAVED')
