import unreal as u,json
from pathlib import Path
a=u.load_asset('/Game/DirectorsCut/Animations/Swordsman/A_Swordsman_ForeheadHold')
m=u.load_asset('/Game/DirectorsCut/Materials/M_PolysplitRecolor')
node=u.load_object(None,m.get_path_name()+':MaterialExpressionCustom_0')
out={'controller':str(a.get_editor_property('controller').set_bone_track_keys.__doc__),'poses':str(u.AnimationLibrary.get_bone_poses_for_time.__doc__),'factory':str(u.AnimSequenceFactory.__doc__),'material_code':node.get_editor_property('code'),'material_inputs':[str(x) for x in node.get_editor_property('inputs')],'controller_methods':[x for x in dir(a.get_editor_property('controller')) if any(k in x for k in ['bone','frame','model','bracket'])]}
out['local_poses']=[t.export_text() for t in u.AnimationLibrary.get_bone_poses_for_time(a,['head_joint','eyes_joint','mouth_joint','L_eyebrow_joint','R_eyebrow_joint'],.5,False)]
(Path(__file__).resolve().parents[1]/'Saved/FilmSetAudit/face_authoring_api.json').write_text(json.dumps(out,indent=2))
u.log('FACE_API_PROBED')

