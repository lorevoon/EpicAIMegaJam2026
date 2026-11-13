import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out={}
for who in ['SwordsmanAct1','ArcherAct1','OldMan']:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(root/'SourceArt/Polysplit'/f'{who}.fbx'),use_anim=False)
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
 out[who]={'bones':{b.name:{'head':list(b.head_local),'tail':list(b.tail_local),'parent':b.parent.name if b.parent else None,'matrix':[list(r) for r in b.matrix_local]} for b in arm.data.bones},'meshes':[]}
 for o in bpy.context.scene.objects:
  if o.type!='MESH':continue
  out[who]['meshes'].append({'name':o.name,'vertices':len(o.data.vertices),'uvs':[[round(v.uv.x,6),round(v.uv.y,6)] for v in o.data.uv_layers.active.data] if any(k in o.name for k in ['mouth','eyes','eyebrow']) else [],'groups':[g.name for g in o.vertex_groups],'shapes':[k.name for k in o.data.shape_keys.key_blocks] if o.data.shape_keys else [],'bounds':[list(v) for v in o.bound_box]})
(root/'Saved/FilmSetAudit/face_source_rigs.json').write_text(json.dumps(out,indent=2))
print('FACE_SOURCES_AUDITED')
