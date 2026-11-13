import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
m=u.load_asset('/Game/DirectorsCut/Materials/M_PolysplitRecolor')
e=u.load_object(None,m.get_path_name()+':MaterialExpressionCustom_0')
e.set_editor_property('code',(root/'Saved/Stage1Audit/recolor_original.txt').read_text())
u.MaterialEditingLibrary.recompile_material(m)
u.EditorAssetLibrary.save_loaded_asset(m,False)
exec((root/'Scripts/validate_act1.py').read_text())
