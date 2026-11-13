import unreal as u,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level("/Game/DirectorsCut/Levels/L_DC_Act1_Promise")
def transform(text):
 values=[float(v) for v in re.findall(r'=([-+]?[0-9]+\.[0-9]+)',text)]
 return u.Transform(u.Vector(*values[4:7]),u.Quat(*values[:4]).rotator(),u.Vector(*values[7:10]))
original=json.loads((root/"Saved/FilmSetAudit/original_layout.json").read_text())
for a in aes.get_all_level_actors():
 if a.get_actor_label() in original["props"] and not a.get_attach_parent_actor():a.set_actor_transform(transform(original["props"][a.get_actor_label()]),False,False)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,"/Game/DirectorsCut/Levels/L_DC_Act1_Promise")
u.log("PROP_TRANSFORMS_CORRECTED")
exec((root/"Scripts/validate_original_act1.py").read_text())
