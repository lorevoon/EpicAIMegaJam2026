import unreal as u, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise')
report={'cast':{},'clips':{}}
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.DCPerformer):
  c=a.get_component_by_class(u.SkeletalMeshComponent)
  report['cast'][a.get_actor_label()]={'cues':[q.animation.get_path_name() for q in a.get_editor_property('animation_cues')],'bones':[str(c.get_bone_name(i)) for i in range(c.get_num_bones())]}
for folder in ['/Game/DirectorsCut/Animations/Swordsman','/Game/DirectorsCut/Act2']:
 for p in u.EditorAssetLibrary.list_assets(folder,False,False):
  if 'Support_' in p:
   a=u.load_asset(p)
   if isinstance(a,u.AnimSequence):report['clips'][p]=a.get_play_length()
out=root/'Saved/Act2/Flow';out.mkdir(parents=True,exist_ok=True)
(out/'inspection.json').write_text(json.dumps(report,indent=2))
print('FLOW_INSPECTED')
