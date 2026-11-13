import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
les=u.get_editor_subsystem(u.LevelEditorSubsystem);les.load_level('/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
aes=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in aes.get_all_level_actors():
    label=a.get_actor_label()
    if label.startswith('Green screen -'):
        s=a.get_actor_scale3d();s.z=16.4;a.set_actor_scale3d(s)
        p=a.get_actor_location();p.z=820;a.set_actor_location(p,False,False)
    if label=='Practical village house':
        # Show the broad facade to the camera lane.
        center,extent=a.get_actor_bounds(False)
        a.set_actor_rotation(u.Rotator(yaw=90),False)
        c,e=a.get_actor_bounds(False);p=a.get_actor_location()
        a.set_actor_location(p+u.Vector(center.x-c.x,center.y-c.y,0),False,False)
    if isinstance(a,u.RectLight):a.get_component_by_class(u.RectLightComponent).set_intensity(4000.)
green=u.load_asset('/Game/DirectorsCut/Materials/M_Studio_ChromaGreen')
u.MaterialEditingLibrary.delete_all_material_expressions(green)
obj=u.MaterialEditingLibrary.create_material_expression(green,u.MaterialExpressionConstant3Vector,-300,0)
obj.set_editor_property('constant',u.LinearColor(.04,1.8,.12,1))
u.MaterialEditingLibrary.connect_material_property(obj,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
u.MaterialEditingLibrary.recompile_material(green);u.EditorAssetLibrary.save_loaded_asset(green,False)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/DirectorsCut/Levels/L_DC_Act1_Promise')
exec((root/'Scripts/validate_act1_film_set.py').read_text())
