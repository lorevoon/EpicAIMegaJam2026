import bpy
from pathlib import Path
R=Path(__file__).resolve().parents[1];O=R/'SourceArt/LivelySet'
for file in (R/'SourceArt/FilmCrew/v6').glob('*.fbx'):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(file))
 rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');rig.name='Armature';rig.animation_data_clear()
 for obj in list(bpy.context.scene.objects):
  if any(part in obj.name for part in ['Megaphone','CarryLoop']):bpy.data.objects.remove(obj,do_unlink=True)
 for bone in rig.pose.bones:bone.matrix_basis.identity()
 bpy.ops.object.select_all(action='SELECT')
 bpy.ops.export_scene.fbx(filepath=str(O/file.name),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
 print('CREW_PREPARED',file.name)
