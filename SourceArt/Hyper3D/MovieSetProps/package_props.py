import json,struct,shutil,zipfile,html
from pathlib import Path
root=Path(__file__).resolve().parent
delivery=root/'Delivery';delivery.mkdir(exist_ok=True)
jobs=json.loads((root/'isolated_manifest.json').read_text())
records=[]
for job in jobs:
 name=job['name'];src=root/'Individual'/name;dest=delivery/name;dest.mkdir(exist_ok=True)
 for filename in [name+'.glb',name+'.fbx','preview.png']:
  assert (src/filename).is_file(),filename
  shutil.copy2(src/filename,dest/filename)
 data=(dest/(name+'.glb')).read_bytes()
 assert data[:4]==b'glTF' and struct.unpack_from('<I',data,8)[0]==len(data)
 size=struct.unpack_from('<I',data,12)[0];doc=json.loads(data[20:20+size])
 assert len(doc['meshes'])==1 and not doc.get('cameras') and not doc.get('skins')
 assert all('uri' not in b for b in doc['buffers'])
 assert (dest/(name+'.fbx')).read_bytes().startswith(b'Kaydara FBX Binary')
 if name!='03_rails' and (src/'textures').exists():shutil.copytree(src/'textures',dest/'textures',dirs_exist_ok=True)
 primitives=doc['meshes'][0]['primitives']
 verts=sum(doc['accessors'][p['attributes']['POSITION']]['count'] for p in primitives)
 triangles=sum(doc['accessors'][p['indices']]['count']//3 for p in primitives)
 generation=job['generation']['generation_id']
 if name[:2] in ['01','02','03']:generation='60af38ff-c0fa-4c2e-97e4-75a17148bd7b'
 if name[:2] in ['06','07','08']:generation='2b503bd5-dc28-49ad-b50c-78dafa5986e5'
 record={'name':name,'source':'Hyper3D Rodin via official CLI / MCP service','generation_id':generation,'vertices':verts,'triangles':triangles,'meshes':1,'units':'meters','pivot':'bottom center','formats':['glb','fbx'],'rigged':False}
 records.append(record)
 (dest/'asset_info.json').write_text(json.dumps(record,indent=2))
(delivery/'asset_manifest.json').write_text(json.dumps(records,indent=2))
(delivery/'README.txt').write_text('MOVIE SET PROPS - 11 INDIVIDUAL STATIC ASSETS\n\nEach named folder contains a textured GLB, FBX, PNG preview and asset metadata.\nGLB embeds its materials and textures. Texture PNGs are also included for FBX workflows.\nUnits: meters. Origins: bottom center. One mesh object per delivered asset.\n\nContents: jib (without camera or rails), crane camera, rails, large flight case, wooden ladder, three studio lights, director chair, megaphone, slate.\nLight heights: tall rectangular 2.6 m; medium round 1.8 m; short barrel 1.3 m.\n\nGenerated with Hyper3D using the supplied visual references and reference-derived descriptions.\nJib/camera/rails and the three lights were separated from image-based Hyper3D generations to preserve their shapes. Remaining props use individual reference-derived text generations.\nBlender processing: isolation, bottom-center pivots, metric scale, mesh joining, small boundary repairs on crane parts, rail material cleanup, GLB/FBX export and preview rendering.\n\nThese are static AI-generated props, not rigged or animated. They have not been imported into the Unreal project or wired as functional lights. Fine details and topology differ from the reference artwork.\nInitial grouped and rejected attempts are excluded from this delivery.\n')
cards=[]
for r in records:
 n=r['name'];label=html.escape(n[3:].replace('_',' ').title())
 cards.append(f'<article><img src="{n}/preview.png" alt="{label}"><h2>{label}</h2><p>{r["triangles"]:,} triangles</p><a href="{n}/{n}.glb">GLB</a> <a href="{n}/{n}.fbx">FBX</a></article>')
(delivery/'Gallery.html').write_text('<!doctype html><meta charset="utf-8"><title>Movie Set Props</title><style>body{background:#1c2028;color:#eef1f5;font:16px system-ui;margin:40px}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:20px}article{background:#2b313c;border-radius:12px;padding:16px}img{width:100%;border-radius:8px}h2{font-size:20px}a{color:#a9d7ff;margin-right:16px}p{color:#aeb8c6}</style><h1>Movie Set Props</h1><p>11 separate Hyper3D assets · GLB + FBX · static meshes · metric scale</p><main>'+''.join(cards)+'</main>',encoding='utf-8')
zip_path=root/'MovieSetProps_11_Assets.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as archive:
 for file in delivery.rglob('*'):
  if file.is_file():archive.write(file,file.relative_to(delivery))
print(json.dumps({'validated_assets':len(records),'zip':str(zip_path),'size_mb':round(zip_path.stat().st_size/1024**2,1),'assets':records},indent=2))
