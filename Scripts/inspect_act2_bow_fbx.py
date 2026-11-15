import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out={}
for file in (root/'SourceArt/Act2/BowRevision6').glob('*.fbx'):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(file))
 arm=next((o for o in bpy.context.scene.objects if o.type=='ARMATURE'),None)
 row={'meshes':[{ 'name':o.name,'dimensions':list(o.dimensions),'bounds':[list(v) for v in o.bound_box]} for o in bpy.context.scene.objects if o.type=='MESH']}
 if arm and arm.animation_data and arm.animation_data.action:
  action=arm.animation_data.action;row['frames']=list(action.frame_range);row['fps']=bpy.context.scene.render.fps;row['samples']=[]
  for i in range(9):
   f=action.frame_range[0]+(action.frame_range[1]-action.frame_range[0])*i/8;bpy.context.scene.frame_set(int(f));row['samples'].append({'f':f,'hands':{b.name:list(b.head) for b in arm.pose.bones if any(s in b.name for s in ['LeftHand','RightHand','Hips']) and 'Thumb' not in b.name and 'Index' not in b.name and 'Pinky' not in b.name and 'Ring' not in b.name and 'Middle' not in b.name}})
 out[file.name]=row
(root/'Saved/Act2/BowRevision6/fbx.json').write_text(json.dumps(out,indent=2));print('FBX_INSPECTED')
