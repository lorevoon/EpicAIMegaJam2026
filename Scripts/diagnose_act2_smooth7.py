import unreal as u,json
from pathlib import Path
B='/Game/DirectorsCut';u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(B+'/Levels/L_DC_Act2_BrokenPromise');actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors();out={}
for a in actors:
 if not isinstance(a,u.DCPerformer) or a.get_actor_label() not in ['Romeo','Juliet']:continue
 c=a.get_component_by_class(u.SkeletalMeshComponent);bones=[str(c.get_bone_name(i)) for i in range(c.get_num_bones())];out[a.get_actor_label()]={'bones':bones[:10],'samples':{}}
 for t in [0,2,10,17,22.3,35]:
  a.evaluate(t);out[a.get_actor_label()]['samples'][str(t)]={'actor':list(a.get_actor_location().to_tuple()),'mesh':str(c.get_relative_transform()),'bones':{b:list(c.get_socket_location(b).to_tuple()) for b in bones if any(x in b.lower() for x in ['root','pelvis','ankle','wrist'])}}
 for key,path in [('source',B+'/Act2/BowRevision6/A_Stand_'+a.get_actor_label()),('guard',B+'/Act2/SmoothRevision7/A_Swordsman_Guard' if a.get_actor_label()=='Romeo' else B+'/Animations/Archer/A_Archer_Bow_Idle')]:
  anim=u.load_asset(path);out[a.get_actor_label()][key]=[str(t) for t in u.AnimationLibrary.get_bone_poses_for_time(anim,bones[:3],0,False,a.get_editor_property('character_mesh'))]
Path('C:/Unity Projects/EpicAIMegaJam2026/Saved/Act2/SmoothRevision7/pose_diagnostic.json').write_text(json.dumps(out,indent=2))
