import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out={}
for file in (root/'SourceArt/Act2/Revision2').glob('*.fbx'):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(file))
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');action=arm.animation_data.action
 out[file.name]={'bones':[b.name for b in arm.data.bones],'frames':list(action.frame_range),'fps':bpy.context.scene.render.fps,'root_samples':[]}
 for f in [action.frame_range[0],sum(action.frame_range)/2,action.frame_range[1]]:
  bpy.context.scene.frame_set(int(f));b=arm.pose.bones[0];out[file.name]['root_samples'].append({'frame':f,'bone':b.name,'pos':list(b.head)})
(root/'Saved/Act2/Revision2/attached_fbx.json').write_text(json.dumps(out,indent=2));print('ATTACHED_FBX_INSPECTED')
