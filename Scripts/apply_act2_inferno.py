"""Apply stage-specific fire variants; preserve the studio and source pack."""
import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);level='/Game/DirectorsCut/Levels/L_DC_Act2_Real';assert les.load_level(level)
count=0
for a in aes.get_all_level_actors():
 if 'DC_Act2RealFire' in [str(t) for t in a.tags]:
  c=a.get_component_by_class(u.NiagaraComponent);smoke='Smoke' in c.get_asset().get_name();c.set_asset(u.load_asset('/Game/DirectorsCut/Act2/NS_Act2_VillageInferno'+('_Smoke' if smoke else '')));count+=1
 if a.get_actor_label().startswith('Blaze warm bounce'):
  c=a.get_component_by_class(u.PointLightComponent);c.set_light_color(u.LinearColor(1,.5,.12,1));c.set_intensity(5000.)
assert 20<count<60,count
# Roof fires establish that buildings, not merely grass, are burning.
for i,(x,y,z) in enumerate([(-6650,-8850,2250),(-6950,-8600,1900),(-7450,-2600,2600)]):
 a=aes.spawn_actor_from_class(u.NiagaraActor,u.Vector(x,y,z));a.set_actor_label('Act2 roof blaze '+str(i));a.set_folder_path('Act2/Real fire');a.tags=['DC_Act2RealFire','DC_RealEnvironment'];a.set_actor_scale3d(u.Vector(1.5,1.5,1.5));a.get_component_by_class(u.NiagaraComponent).set_asset(u.load_asset('/Game/DirectorsCut/Act2/NS_Act2_VillageInferno'))
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert u.EditorLoadingAndSavingUtils.save_map(world,level)
p=root/'Saved/Act2/dual_build.json';r=json.loads(p.read_text());r['fire_systems']=count+3;r['fire_variants']=['NS_Act2_VillageInferno','NS_Act2_VillageInferno_Smoke'];p.write_text(json.dumps(r,indent=2));print('ACT2_INFERNO_APPLIED',count+3)
