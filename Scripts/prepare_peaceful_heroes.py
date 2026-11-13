import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=Path(r'C:\Unity Projects\COMP 3329 POC\Assets\PolysplitGames\LowPolyMedievalFantasyHeroes\BasicHeroes')
for who,source in [('Swordsman','M_Swordsman'),('Archer','F_Archer')]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(src/(source+'.fbx')),use_anim=False)
    for o in list(bpy.context.scene.objects):
        if o.type=='MESH' and (o.hide_viewport or any(s in o.name.replace('Swordsman','') for s in ['Sword','Scabbard','Bow','Arrow','Quiver'])):
            bpy.data.objects.remove(o,do_unlink=True)
    for o in bpy.context.scene.objects:o.hide_viewport=False;o.hide_render=False
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.fbx(filepath=str(root/'SourceArt/Polysplit'/f'{who}Act1.fbx'),use_selection=True,object_types={'MESH','ARMATURE','EMPTY'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
