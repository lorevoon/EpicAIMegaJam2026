import unreal as u, json, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/DirectorsCut'
eal=u.EditorAssetLibrary;at=u.AssetToolsHelpers.get_asset_tools()
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX false')
def load(p):
    o=u.load_asset(p)
    assert o,p
    return o
def imp(file,folder,name,skeleton,mesh=False):
    if eal.does_asset_exist(folder+'/'+name):return load(folder+'/'+name)
    task=u.AssetImportTask();task.filename=str(file);task.destination_path=folder;task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=False
    ui=u.FbxImportUI();ui.import_mesh=mesh;ui.import_as_skeletal=True;ui.import_animations=not mesh
    ui.import_materials=False;ui.import_textures=False;ui.create_physics_asset=mesh;ui.skeleton=skeleton
    ui.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if mesh else u.FBXImportType.FBXIT_ANIMATION
    ui.automated_import_should_detect_type=False
    ui.anim_sequence_import_data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    task.options=ui;at.import_asset_tasks([task])
    for p in task.imported_object_paths:
        obj=load(p)
        if isinstance(obj,u.SkeletalMesh if mesh else u.AnimSequence):
            if obj.get_name()!=name:eal.rename_asset(obj.get_path_name(),folder+'/'+name)
            return load(folder+'/'+name)
    raise RuntimeError('Import failed '+str(file))
def material(name,parent,colors):
    path=BASE+'/Materials/'+name
    m=u.load_asset(path) or at.create_asset(name,BASE+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    u.MaterialEditingLibrary.set_material_instance_parent(m,load(parent))
    for key,v in colors.items():u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(m,key,u.LinearColor(*v,1))
    eal.save_loaded_asset(m);return m
hero=load(BASE+'/Characters/Swordsman/SK_Swordsman')
parent=BASE+'/Materials/M_PolysplitRecolor'
black=material('MI_Crew_Black',parent,{'Color1':(.012,.016,.022),'Color2':(.025,.03,.038),'Color3':(.045,.05,.06)})
gray=material('MI_OldMan_GrayHair',parent,{'Color1':(.48,.46,.42),'Color2':(.48,.46,.42),'Color3':(.48,.46,.42)})
cloth=material('MI_OldMan_Cloth',parent,{'Color1':(.13,.2,.16),'Color2':(.18,.09,.04),'Color3':(.3,.25,.15)})
for role in ['OldMan','Crew']:
    mesh=imp(ROOT/'SourceArt/Polysplit'/f'{role}.fbx',BASE+'/Characters/'+role,'SK_'+role,hero.skeleton,True)
    slots=mesh.get_editor_property('materials')
    for i,slot in enumerate(slots):
        n=str(slot.material_slot_name)
        mat=gray if 'GrayHair' in n else black if role=='Crew' and ('Objects' in n or 'Headset' in n) else cloth if role=='OldMan' and 'Objects' in n else load(BASE+'/Materials/MI_Polysplit_Hero_Body')
        slot.material_interface=mat
        slots[i]=slot
    mesh.set_editor_property('materials',slots)
    eal.save_loaded_asset(mesh)
report=[]
for entry in json.loads((ROOT/'SourceArt/Act1Poses/manifest.json').read_text()):
    who=entry['character'];sk=load(BASE+'/Characters/'+who+'/SK_'+who).skeleton
    anim=imp(ROOT/'SourceArt/Act1Poses'/entry['file'],BASE+'/Animations/'+who,Path(entry['file']).stem,sk)
    assert anim.get_editor_property('skeleton')==sk
    report.append({'asset':anim.get_path_name(),'length':anim.get_play_length()})
(ROOT/'SourceArt/act1_assets_import_report.json').write_text(json.dumps(report,indent=2))
sys.path.insert(0,str(ROOT/'Scripts'))
import import_support_assets
import_support_assets.run_unreal()
u.log('ACT1_ASSETS_COMPLETE')




exec((ROOT/'Scripts/fix_cast_materials.py').read_text())
