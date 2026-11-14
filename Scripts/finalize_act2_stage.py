"""Refine the final pose and effects scale, then refresh the review cut."""
import unreal as u,json,math,warnings
from pathlib import Path
warnings.filterwarnings('ignore',category=DeprecationWarning)
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';FOLDER=BASE+'/Act2';LEVEL=BASE+'/Levels/L_DC_Act2_BrokenPromise';FPS=30
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem);at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary
assert les.load_level(LEVEL);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=list(aes.get_all_level_actors());cast=[a for a in actors if isinstance(a,u.DCPerformer)];by={a.get_actor_label():a for a in cast}
seq=u.load_asset(FOLDER+'/LS_DC_Act2_38s')
for b in seq.get_bindings():
 for tr in b.get_tracks():
  for sec in tr.get_sections():
   print('TRACK',b.get_name(),tr.get_class().get_name(),sec.get_start_frame(),sec.get_end_frame(),[(ch.get_name(),len(ch.get_keys())) for ch in sec.get_all_channels()])
fire=next(a for a in cast if a.get_actor_label().startswith('Fire'));fire.set_editor_property('mesh_offset',u.Transform(scale=u.Vector(.01,.01,.01)))
for name in ['M_FlameOrange','M_FlameGold','M_Smoke']:
 source=u.load_asset(FOLDER+'/'+name);dest=FOLDER+'/'+name+'_Final';mat=u.load_asset(dest) if eal.does_asset_exist(dest) else eal.duplicate_asset(source.get_path_name(),dest);mat.set_editor_property('used_with_skeletal_mesh',True);u.MaterialEditingLibrary.recompile_material(mat);assert eal.save_loaded_asset(mat,False)
 for i,slot in enumerate(fire.get_editor_property('character_mesh').materials):
  if slot.material_interface==source:fire.get_component_by_class(u.SkeletalMeshComponent).set_material(i,mat)
mesh=by['Juliet'].get_editor_property('character_mesh');comp=by['Juliet'].get_component_by_class(u.SkeletalMeshComponent);bones=[comp.get_bone_name(i) for i in range(comp.get_num_bones())];anim=by['Juliet'].get_editor_property('animation_cues')[0].animation;mourn=u.load_asset(FOLDER+'/A_Juliet_Mourning')
original=anim;dest=FOLDER+'/A_Act2_Juliet_Final';anim=u.load_asset(dest) if eal.does_asset_exist(dest) else eal.duplicate_asset(original.get_path_name(),dest)
data={str(b):[[],[],[]] for b in bones}
for f in range(1141):
 t=f/30;source=mourn if t>=29 else original;poses=u.AnimationLibrary.get_bone_poses_for_time(source,bones,t-29 if t>=29 else t,False,mesh)
 for b,tr in zip(bones,poses):
  data[str(b)][0].append(tr.translation);data[str(b)][1].append(tr.rotation);data[str(b)][2].append(tr.scale3d)
c=anim.get_editor_property('controller');c.open_bracket('Final mourning pose',False)
for b,keys in data.items():assert c.set_bone_track_keys(b,*keys,False)
c.close_bracket(False);assert eal.save_loaded_asset(anim,False)
cue=u.DCAnimationCue();cue.animation=anim;cue.loop=False;by['Juliet'].set_editor_property('animation_cues',[cue])
# Kneel beside Romeo's head, both hands surrounding his face.
for a in actors:
 if isinstance(a,u.DCWaypoint) and a.get_editor_property('performer')==by['Juliet'] and a.get_editor_property('time')>=29:
  p=a.get_actor_location();p.x=-6014;p.y=-9180;a.set_actor_location(p,False,False)
# Narrow repair: disconnect calls fed by missing actor literals in this newly generated map only.
from editor_toolset.toolsets.blueprint import BlueprintTools as B
bp=u.load_object(None,LEVEL+'.L_DC_Act2_BrokenPromise:PersistentLevel.L_DC_Act2_BrokenPromise')
repairs=[]
for g in B.list_graphs(bp):
 for n in B.find_nodes(g):
  if n.get_class().get_name()!='K2Node_Literal' or n.get_node_title()!='Unknown':continue
  for output in B._get_node_info(n).output_pins:
   for target in output.connected_pins:
    for pin in B._get_node_info(target.node).input_pins:
     if pin.name=='execute':
      for upstream in pin.connected_pins:B.break_pins(upstream,pin.pin_id);repairs.append(n.get_name())
u.BlueprintEditorLibrary.compile_blueprint(bp)
print('REPAIRED_MISSING_REFERENCES',repairs)
# Hide editing helpers from viewport review as well as game shots.
for a in actors:
 if isinstance(a,(u.DCWaypoint,u.DCBeat,u.DCStartMark)):a.set_is_temporarily_hidden_in_editor(True)

for a in cast:a.evaluate(0)
assert u.EditorLoadingAndSavingUtils.save_map(world,LEVEL)
# Reload to rebuild the performers' waypoint caches.
assert les.load_level('/Game/Levels/L_DC_Zombie');assert les.load_level(LEVEL)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();cast=[a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer)]
# Reuse the already validated Sequencer authoring section, without rebaking source clips.
text=(ROOT/'Scripts/polish_act2_stage.py').read_text();exec(text[text.index('# Bake actor motion'):].replace("name='LS_DC_Act2_38s'","name='LS_DC_Act2_Final38s'"))
print('ACT2_FINAL_COMPLETE')
