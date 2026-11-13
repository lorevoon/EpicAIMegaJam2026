import unreal as u,json
from pathlib import Path
rows={}
for who in ['Swordsman','OldMan','Crew','OrcArmored']:
    mesh=u.load_asset('/Game/DirectorsCut/Characters/'+who+'/SK_'+who)
    rows[who]=[dict(slot=str(s.material_slot_name),mat=str(s.material_interface)) for s in mesh.get_editor_property('materials')]
Path(r'C:\Unity Projects\EpicAIMegaJam2026\Saved\Stage1Audit\materials.json').write_text(json.dumps(rows,indent=2))
