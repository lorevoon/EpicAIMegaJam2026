import unreal as u
base='/Game/DirectorsCut/Materials/'
at=u.AssetToolsHelpers.get_asset_tools();eal=u.EditorAssetLibrary
def solid(name,color):
    m=u.load_asset(base+name)
    if not m:
        m=at.create_asset(name,base.rstrip('/'),u.Material,u.MaterialFactoryNew())
        x=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector)
        x.set_editor_property('constant',u.LinearColor(*color,1))
        u.MaterialEditingLibrary.connect_material_property(x,'',u.MaterialProperty.MP_BASE_COLOR)
        u.MaterialEditingLibrary.recompile_material(m)
        eal.save_loaded_asset(m,False)
    return m
gray=solid('M_VillagerHair',(.5,.48,.44));black=solid('M_Headset',(.012,.014,.018))
for role in ['OldMan','Crew','OrcArmored']:
    mesh=u.load_asset('/Game/DirectorsCut/Characters/'+role+'/SK_'+role)
    mesh.modify();slots=[]
    for old in mesh.materials:
        n=str(old.material_slot_name)
        mat=gray if 'GrayHair' in n else black if 'Headset' in n else u.load_asset(base+('MI_Polysplit_Orc_'+('Body' if 'Body' in n else 'Objects') if role=='OrcArmored' else 'MI_Polysplit_Hero_Body' if 'Body' in n else 'MI_Crew_Black' if role=='Crew' else 'MI_OldMan_Cloth'))
        slots.append(u.SkeletalMaterial(material_interface=mat,material_slot_name=old.material_slot_name,uv_channel_data=old.uv_channel_data))
    mesh.materials=slots
    assert all(s.material_interface for s in mesh.materials)
    eal.save_loaded_asset(mesh,False)
u.log('CAST_MATERIALS_FIXED')
