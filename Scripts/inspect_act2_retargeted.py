import unreal as u,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/DirectorsCut';F=BASE+'/Act2/Revision2';les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
les.load_level(BASE+'/Levels/L_DC_Act2_BrokenPromise');actors=list(aes.get_all_level_actors());out={}
for who,paths in [('Romeo',[F+'/A_Swordsman_LayingHandGesture',BASE+'/Animations/Swordsman/A_Swordsman_Support_DeathBackward']),('Juliet',[F+'/A_Archer_HoldingDown',F+'/A_Archer_Terrified']),('Armoured monster',[BASE+'/Act2/A_Raider_Support_Walk'])]:
 a=next(a for a in actors if isinstance(a,u.DCPerformer) and a.get_actor_label()==who);c=a.get_component_by_class(u.SkeletalMeshComponent);mesh=a.get_editor_property('character_mesh');names=[str(c.get_bone_name(i)) for i in range(c.get_num_bones())]
 for path in paths:
  anim=u.load_asset(path);q=u.DCAnimationCue();q.animation=anim;q.loop=False;a.set_editor_property('animation_cues',[q]);rows=[]
  for t in [0,anim.get_play_length()/2,anim.get_play_length()]:
   a.evaluate(t);tf=a.get_actor_transform();bones=['root_joint','pelvis_joint','chest_joint','head_joint','R_wrist_joint','L_wrist_joint','L_ankle_joint','R_ankle_joint']
   rows.append({'time':t,'points':{b:list(tf.inverse_transform_location(c.get_socket_location(b)).to_tuple()) for b in bones},'root_local':str(u.AnimationLibrary.get_bone_poses_for_time(anim,names[:3],t,False,mesh))})
  out[path]={'names':names[:6],'length':anim.get_play_length(),'samples':rows}
(ROOT/'Saved/Act2/Revision2/poses.json').write_text(json.dumps(out,indent=2));print('RETARGETED_POSES_INSPECTED')
