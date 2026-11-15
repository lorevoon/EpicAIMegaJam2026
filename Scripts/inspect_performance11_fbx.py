import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out={}
for file in list((root/'SourceArt/PerformanceRevision11').glob('*.fbx'))+[root/'SourceArt/Act2/BowRevision6/BowBasic.fbx']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(file))
 row={'actions':[{'name':a.name,'frames':list(a.frame_range)} for a in bpy.data.actions],'meshes':[],'samples':[]}
 for o in bpy.context.scene.objects:
  if o.type=='MESH':
   row['meshes'].append({'name':o.name,'bounds':[list(v) for v in o.bound_box]})
   if file.stem=='BowBasic':row['vertices']=[list(o.matrix_world@v.co) for v in o.data.vertices]
 arm=next((o for o in bpy.context.scene.objects if o.type=='ARMATURE'),None)
 if arm and row['actions']:
  lo,hi=row['actions'][0]['frames']
  for i in range(11):
   f=lo+(hi-lo)*i/10;bpy.context.scene.frame_set(int(f));row['samples'].append({'frame':f,'bones':{b.name:list(arm.matrix_world@b.head) for b in arm.pose.bones if b.name.split(':')[-1] in ['Hips','Head','LeftHand','RightHand','LeftFoot','RightFoot']}})
 out[file.name]=row
(root/'Saved/PerformanceRevision11/fbx.json').write_text(json.dumps(out,indent=2))
