import unreal as u
m=u.load_asset('/Game/DirectorsCut/Materials/M_PolysplitRecolor')
e=u.load_object(None,m.get_path_name()+':MaterialExpressionCustom_0')
code=e.get_editor_property('code')
open('C:/Unity Projects/EpicAIMegaJam2026/Saved/Stage1Audit/recolor_original.txt','w').write(code)
e.set_editor_property('code',code.replace('float2(UV.x, 1.0 - UV.y)','float2(UV.x, UV.y)'))
u.MaterialEditingLibrary.recompile_material(m)
