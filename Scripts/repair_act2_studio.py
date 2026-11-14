"""Restore standalone studio ground/enclosure and hide the gameplay spawn disc."""
import unreal as u, json, shutil, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'Saved/Act2/StudioRepair';OUT.mkdir(parents=True,exist_ok=True)
BASE='/Game/DirectorsCut/Levels/';MAIN=BASE+'L_DC_Act2_BrokenPromise';SET=BASE+'L_DC_Act2_Set'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);eal=u.EditorAssetLibrary
for n in ['L_DC_Act2_Set','L_DC_Act2_BrokenPromise']:
 dst=OUT/(n+'_before.umap')
 if not dst.exists():shutil.copy2(ROOT/'Content/DirectorsCut/Levels'/(n+'.umap'),dst)
assert les.load_level(MAIN)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
ignore=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,9000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
 assert hit[0],(x,y)
 return hit[4].z
xs=list(range(-9250,-4249,50));ys=list(range(-11050,-7449,50))
heights={(x,y):height(x,y) for y in ys for x in xs}
# Keep gameplay start location, hide only its editor-style disc/text in game.
marks=[]
for a in aes.get_all_level_actors():
 if isinstance(a,u.DCStartMark):
  marks.append(dict(label=a.get_actor_label(),position=list(a.get_actor_location().to_tuple())))
  a.set_actor_hidden_in_game(True)
  for c in a.get_components_by_class(u.PrimitiveComponent):c.set_visibility(False)
assert u.EditorLoadingAndSavingUtils.save_map(world,MAIN)
obj=OUT/'Act2_StudioGround.obj';lines=['o Act2_StudioGround']
for y in ys:
 for x in xs:lines.append(f'v {x} {-y} {heights[x,y]-2}')
for y in ys:
 for x in xs:lines.append(f'vt {x/1000} {y/1000}')
w=len(xs)
for j in range(len(ys)-1):
 for i in range(w-1):
  a=j*w+i+1;b=a+1;c=a+w;d=c+1
  lines.extend([f'f {a}/{a} {c}/{c} {b}/{b}',f'f {b}/{b} {c}/{c} {d}/{d}'])
obj.write_text('\n'.join(lines))
task=u.AssetImportTask();task.filename=str(obj);task.destination_path='/Game/DirectorsCut/Environment';task.destination_name='SM_Act2_StudioGround_Final';task.automated=True;task.replace_existing=True;task.save=True
opt=u.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
opt.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False);opt.static_mesh_import_data.set_editor_property('auto_generate_collision',False);task.options=opt
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);assert task.imported_object_paths
assert les.load_level(SET);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=list(aes.get_all_level_actors())
for a in actors:
 if 'DC_Act2GroundRepair' in [str(t) for t in a.tags]:aes.destroy_actor(a)
mat=u.load_asset('/Game/DirectorsCut/Materials/M_Studio_Charcoal')
ground=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector());ground.set_actor_label('Studio scenic ground - matches performance terrain');ground.set_folder_path('Act2/Studio ground');ground.tags=['DC_Act2Studio','DC_Act2GroundRepair']
ground.static_mesh_component.set_static_mesh(u.load_asset(task.imported_object_paths[0]));ground.static_mesh_component.set_material(0,mat);ground.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
for a in actors:
 label=a.get_actor_label()
 if label in ['Green screen - left','Green screen - right','Studio roof']:
  p=a.get_actor_location();p.y=-9250;a.set_actor_location(p,False,False);s=a.get_actor_scale3d();s.y=36.4;a.set_actor_scale3d(s)
# Small support risers preserve carefully cleared prop locations and the original performance.
# Sample the terrain interpolant; all risers are studio-only and noncolliding.
def h(x,y):
 x=max(xs[0],min(xs[-1]-0.001,x));y=max(ys[0],min(ys[-1]-0.001,y));i=int((x-xs[0])//50);j=int((y-ys[0])//50);fx=(x-xs[i])/50;fy=(y-ys[j])/50
 return ((1-fx)*heights[xs[i],ys[j]]+fx*heights[xs[i+1],ys[j]])*(1-fy)+((1-fx)*heights[xs[i],ys[j+1]]+fx*heights[xs[i+1],ys[j+1]])*fy-2
def block(label,x,y,top,sx,sy):
 bottom=min(h(x+dx,y+dy) for dx in [-sx/2,sx/2] for dy in [-sy/2,sy/2])-5
 if top-bottom<1:return
 a=aes.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x,y,(top+bottom)/2));a.set_actor_label(label);a.set_folder_path('Act2/Studio ground');a.tags=['DC_Act2Studio','DC_Act2GroundRepair'];a.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'));a.static_mesh_component.set_material(0,mat);a.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);a.set_actor_scale3d(u.Vector(sx/100,sy/100,(top-bottom)/100))
# Architecture foundations fill beneath wall bases without changing the raid corridor.
for label,x,y,top,sx,sy in [('House scenic foundation',-7800,-9300,1295,1600,1600),('Mill scenic foundation',-6630,-8840,1306,560,560),('Cottage scenic foundation',-6986,-8304,1265,660,660)]:block(label,x,y,top,sx,sy)
floor_labels={'Equipment case','H3D - Rails','H3D - Big Box','H3D - Ladder','H3D - Light Tall Rectangular','H3D - Light Medium Round','H3D - Light Short Large','H3D - Director Chair','Camera station - dark crane','Production key light','Production fill light','Softbox beside video village','Grip stand - spare reflector','Grip stand - spare lamp','Floor lamp beside dolly','Lighting truss foot','Lighting stand base','Table leg'}
adjust=[]
for a in actors:
 if a.get_actor_label() not in floor_labels:continue
 b,e=a.get_actor_bounds(False);bottom=b.z-e.z;surface=h(b.x,b.y)
 if bottom>=surface:
  block('Ground support - '+a.get_actor_label(),b.x,b.y,bottom,max(20,e.x*2+8),max(20,e.y*2+8))
 else:
  # Preserve grouped truss/table assemblies; their foundations are already embedded.
  if a.get_actor_label() in ['Lighting truss foot','Lighting stand base','Table leg','H3D - Rails','H3D - Big Box']:continue
  p=a.get_actor_location();delta=surface+1-bottom;p.z+=delta;a.set_actor_location(p,False,False);adjust.append([a.get_actor_label(),delta])
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(-4700,-10700,3000),u.Rotator(pitch=-25,yaw=140))
assert u.EditorLoadingAndSavingUtils.save_map(world,SET)
(OUT/'repair.json').write_text(json.dumps(dict(ground_vertices=len(xs)*len(ys),ground_range=[min(heights.values()),max(heights.values())],hidden_start_marks=marks,grounded_props=adjust,side_wall_y_range=[-11070,-7430]),indent=2));print('ACT2_STUDIO_REPAIR_DONE')
