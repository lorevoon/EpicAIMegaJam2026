import unreal as u,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);level='/Game/DirectorsCut/Levels/L_DC_Act2_BrokenPromise';assert les.load_level(level);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors())
for a in actors:
 if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo':a.set_editor_property('body_bone','pelvis_joint')
 if isinstance(a,u.DCBeat):
  if str(a.get_editor_property('beat_id'))=='2.5':
   refs=list(a.get_editor_property('anchors'));refs[1].set_editor_property('part',u.DCAnchorPart.BODY);a.set_editor_property('anchors',refs)
  if str(a.get_editor_property('beat_id'))=='2.1':a.set_editor_property('camera_spot',a.get_actor_transform().inverse_transform_location(u.Vector(-5750,-9750,1400)))
 if isinstance(a,u.DCZombieDirector):a.set_editor_property('reference_preferred_distance',850.)
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
print('RAID_REFERENCE_FRAMING_UPDATED')
