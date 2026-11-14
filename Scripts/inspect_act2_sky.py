import unreal as u,json
from pathlib import Path
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);les.load_level('/Game/DirectorsCut/Levels/L_DC_Act2_Real');out=[]
for a in aes.get_all_level_actors():
 if isinstance(a,u.PostProcessVolume):out.append({'postprocess':a.get_actor_label(),'blendables':str(a.get_editor_property('settings').get_editor_property('weighted_blendables'))})
 c=a.get_component_by_class(u.StaticMeshComponent)
 if c:
  center,extent=a.get_actor_bounds(False)
  out.append({'label':a.get_actor_label(),'path':a.get_path_name(),'mesh':str(c.static_mesh),'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())],'scale':list(a.get_actor_scale3d().to_tuple()),'hidden':a.get_editor_property('hidden')})
for cls in [u.SkyLight,u.ExponentialHeightFog,u.AtmosphericFog]:
 out.extend({'label':a.get_actor_label(),'class':a.get_class().get_name(),'path':a.get_path_name()} for a in aes.get_all_level_actors() if isinstance(a,cls))
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/Act2/Revision2/sky.json').write_text(json.dumps(out,indent=2));print('SKY_INSPECTED')
