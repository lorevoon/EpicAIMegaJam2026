"""Rebuild complete source character meshes, preserving the authored wrist weapon bind."""
import bpy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'SourceArt/Act2';src=Path(r'C:/Unity Projects/COMP 3329 POC/Assets/PolysplitGames')
report={}
for who,path in [('Orc',src/'LowPolyMedievalFantasyBipedCreatures/BipedCreatures/Orc.fbx'),('Romeo',src/'LowPolyMedievalFantasyHeroes/BasicHeroes/M_Swordsman.fbx')]:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path),use_anim=False)
 keep={'Orc_Head','Orc_Top','Orc_Bottom','Orc_Top_Armor','Orc_Bottom_Armor','Orc_Sword'}
 for o in list(bpy.context.scene.objects):
  if o.type!='MESH':continue
  remove=(o.name not in keep) if who=='Orc' else (o.hide_viewport or 'Scabbard' in o.name)
  if remove:bpy.data.objects.remove(o,do_unlink=True)
 for o in bpy.context.scene.objects:o.hide_viewport=False;o.hide_render=False
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=arm
 name='Orc_Complete_Raid' if who=='Orc' else 'Romeo_SourceSword'
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE','EMPTY'},add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y')
 report[who]={'source':str(path),'meshes':[o.name for o in bpy.context.scene.objects if o.type=='MESH'],'vertices':sum(len(o.data.vertices) for o in bpy.context.scene.objects if o.type=='MESH'),'bones':len(arm.data.bones)}
(OUT/'raid_character_sources.json').write_text(json.dumps(report,indent=2));print('RAID_CHARACTER_SOURCES_READY',report)
