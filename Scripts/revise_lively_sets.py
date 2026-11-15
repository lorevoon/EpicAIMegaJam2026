import unreal as u,json,shutil,math
from pathlib import Path
R=Path(__file__).resolve().parents[1];B='/Game/DirectorsCut';F=B+'/LivelySet';e=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools();report={}
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
file=R/'SourceArt/LivelySet/Breathing Idle (1).fbx';shutil.copy2('C:/Users/iris6/Downloads/Breathing Idle (1).fbx',file)
mesh=u.load_asset(F+'/SK_Director_Game');task=u.AssetImportTask();task.filename=str(file);task.destination_path=F;task.destination_name='A_DirectorGame_BreathingIdle';task.automated=True;task.save=True;task.replace_existing=True
ui=u.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True;ui.import_materials=False;ui.import_textures=False;ui.skeleton=mesh.skeleton;ui.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION;ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME);task.options=ui;at.import_asset_tasks([task]);idle=next(u.load_asset(p) for p in task.imported_object_paths if isinstance(u.load_asset(p),u.AnimSequence));assert idle.get_play_length()>0;e.save_loaded_asset(idle,False)
# The source atlas has transparent facial cards. Preserve its alpha, rather than displaying its red backing.
m=u.load_asset(F+'/M_Original_Hero_Body_Atlas');mel=u.MaterialEditingLibrary;sample=mel.get_material_property_input_node(m,u.MaterialProperty.MP_BASE_COLOR);assert isinstance(sample,u.MaterialExpressionTextureSample)
m.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True);mel.connect_material_property(sample,'A',u.MaterialProperty.MP_OPACITY_MASK);mel.recompile_material(m);e.save_loaded_asset(m,False)
les=u.get_editor_subsystem(u.LevelEditorSubsystem);aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for act,main in [(1,'L_DC_Act1_Promise'),(2,'L_DC_Act2_BrokenPromise')]:
 les.load_level(B+'/Levels/'+main);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();removed=[]
 for a in list(aes.get_all_level_actors()):
  if isinstance(a,u.DCCrewMember):
   if a.get_editor_property('crew_role') in ['Sound mixer','Boom operator']:removed.append(a.get_actor_label());aes.destroy_actor(a)
   elif a.get_editor_property('director'):
    a.set_editor_property('activity',idle);d=a.character.get_editor_property('animation_data');d.anim_to_play=idle;a.character.set_editor_property('animation_data',d)
 studio=u.GameplayStatics.get_streaming_level(world,'L_DC_Act%d_Set'%act);assert u.EditorLoadingAndSavingUtils.save_map(studio.get_loaded_level().get_outer(),B+'/Levels/L_DC_Act%d_Set'%act)
 report[str(act)]={'removed':removed,'crew':len([a for a in aes.get_all_level_actors() if isinstance(a,u.DCCrewMember)])}
les.load_level(B+'/Levels/L_DC_Act1_Promise');a=next(a for a in aes.get_all_level_actors() if isinstance(a,u.DCPerformer) and a.get_actor_label()=='Romeo');mesh=a.get_editor_property('character_mesh');cues=list(a.get_editor_property('animation_cues'));report['romeo']=[]
for cue in cues:
 source=cue.animation
 if source.get_name().startswith('A_RomeoHappy_'):source=u.load_asset(B+'/Animations/Act1Revision15/A_Act1_Romeo')
 path=F+'/A_RomeoHappy_'+source.get_name();anim=u.load_asset(path) or e.duplicate_asset(source.get_path_name(),path)
 bones=['mouth_joint','L_eyebrow_joint','R_eyebrow_joint'];fps=30;frames=round(anim.get_play_length()*fps)+1;data={b:[[],[],[]] for b in bones}
 for frame in range(frames):
  t=frame/fps;poses=u.AnimationLibrary.get_bone_poses_for_time(source,bones,t,False,mesh)
  # Preserve the authored kiss and body motion; soften the face away from the kiss.
  scene=cue.start_time+t/max(cue.play_rate,.001);happy=(.5+.12*math.sin(scene*.65))*(0 if 15.5<scene<20.5 else 1)
  for bone,tr in zip(bones,poses):
   if bone=='mouth_joint':sc=tr.scale3d;sc.z*=1+.07*happy;tr.scale3d=sc
   else:p=tr.translation;p.x+=.35*happy;tr.translation=p
   data[bone][0].append(tr.translation);data[bone][1].append(tr.rotation);data[bone][2].append(tr.scale3d)
 ctrl=anim.get_editor_property('controller');ctrl.open_bracket('Gentle happy facial rig accents',False)
 for bone,keys in data.items():ctrl.set_bone_track_keys(bone,*keys,False)
 ctrl.close_bracket(False)
 if u.AnimationLibrary.does_curve_exist(anim,'FaceSmile',u.RawCurveTrackTypes.RCT_FLOAT):u.AnimationLibrary.remove_curve(anim,'FaceSmile')
 u.AnimationLibrary.add_curve(anim,'FaceSmile');u.AnimationLibrary.add_float_curve_keys(anim,'FaceSmile',[0.,2.1,2.5,14.8,16.,20.],[0.,0.,.65,.65,0.,0.])
 e.save_loaded_asset(anim,False);cue.animation=anim;report['romeo'].append(path)
a.set_editor_property('animation_cues',cues)
c=a.get_component_by_class(u.SkeletalMeshComponent)
for i,slot in enumerate(mesh.materials):
 if 'Body' in str(slot.material_slot_name):c.set_material(i,u.load_asset(B+'/Animations/Act1Revision14/MI_RomeoFriendlyFace'))
a.evaluate(0);assert u.EditorLoadingAndSavingUtils.save_map(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),B+'/Levels/L_DC_Act1_Promise')
(R/'Saved/lively_revision.json').write_text(json.dumps(report,indent=2));print('LIVELY_REVISION_SAVED',report)

exec((R/'Scripts/fix_lively_face_shader.py').read_text())
