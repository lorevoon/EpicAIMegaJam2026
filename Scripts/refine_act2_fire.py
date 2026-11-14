import unreal as u,math,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut/Levels/';MAIN=BASE+'L_DC_Act2_BrokenPromise';REAL=BASE+'L_DC_Act2_Real'
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level(REAL)
architecture=[(a.get_actor_label(),a.get_class().get_path_name(),a.get_actor_transform()) for a in aes.get_all_level_actors() if not a.get_attach_parent_actor()]
for a in list(aes.get_all_level_actors()):
 if 'DC_Act2RealFire' in [str(t) for t in a.tags] or a.get_actor_label().startswith('Blaze warm bounce'):aes.destroy_actor(a)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,REAL)
assert les.load_level(MAIN)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
ignore=[a for a in aes.get_all_level_actors() if not isinstance(a,u.LandscapeProxy)]
def height(x,y):
 hit=u.SystemLibrary.line_trace_single(world,u.Vector(x,y,9000),u.Vector(x,y,-3000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE,True).to_tuple()
 return hit[4].z if hit[0] else 1200.
# Huge flames surround the action, never occupying the duel/mourning footprint.
points=[]
for i,(x,y,scale) in enumerate([(-7300,-8500,3.0),(-6700,-7900,3.5),(-5900,-7750,3.2),(-5100,-7950,3.5),(-4300,-8500,3.4),(-4150,-9500,3.0),(-4700,-10400,3.3),(-5600,-10800,3.0),(-6700,-10850,3.0),(-7600,-10200,3.1),(-7900,-9250,3.5),(-6750,-8750,2.2)]):
 points.append((x,y,height(x,y)+10,scale,i in [0,3,6,10]))
# Fire also spreads among the distant village buildings, with fewer smoke systems.
for label,cls,tf in architecture:
 class_name=cls.rsplit('.',1)[-1]
 if 'House' not in class_name and 'Windmill' not in class_name:continue
 p=tf.translation
 if math.hypot(p.x+5900,p.y+9200)<1800:continue
 for dx,dy in [(-200,120),(230,-150)]:points.append((p.x+dx,p.y+dy,height(p.x+dx,p.y+dy)+25,2.8,False))
assert les.load_level(REAL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for i,(x,y,z,s,smoke) in enumerate(points):
 a=aes.spawn_actor_from_class(u.NiagaraActor,u.Vector(x,y,z));a.set_actor_label('Act2 village blaze %02d'%i);a.set_folder_path('Act2/Real fire');a.tags=['DC_Act2RealFire','DC_RealEnvironment'];a.set_actor_scale3d(u.Vector(s,s,s))
 c=a.get_component_by_class(u.NiagaraComponent);c.set_asset(u.load_asset('/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big'+('_Smoke' if smoke else '')));c.set_auto_activate(True);c.set_cast_shadow(False)
for i,(x,y,z,s,smoke) in enumerate(points[:12]):
 if i%3:continue
 a=aes.spawn_actor_from_class(u.PointLight,u.Vector(x,y,z+250));a.set_actor_label('Blaze warm bounce '+str(i));a.set_folder_path('Act2/Real fire');a.tags=['DC_RealEnvironment']
 c=a.get_component_by_class(u.PointLightComponent);c.set_mobility(u.ComponentMobility.MOVABLE);c.set_light_color(u.LinearColor(1,.23,.025,1));c.set_intensity(18000.);c.set_attenuation_radius(2200.);c.set_cast_shadows(False)
assert u.EditorLoadingAndSavingUtils.save_map(world,REAL)

print('ACT2_FIRE_REFINED',len(points))
