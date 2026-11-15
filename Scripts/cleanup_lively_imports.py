"""Remove only superseded imports made by this pipeline, after final assets pass validation."""
import unreal as u
from pathlib import Path
R=Path(__file__).resolve().parents[1];assert (R/'Saved/lively_validation.json').exists()
F='/Game/DirectorsCut/LivelySet/';e=u.EditorAssetLibrary
for name in ['A_Director_Idle','SK_Director','SK_Crew_M01_Brown_Crop','SK_Crew_F01_Auburn_Bun','SK_Crew_F02_Blonde_Bob','SK_Crew_M02_Silver_Swept','SK_Crew_F03_Black_Ponytail']:
 if e.does_asset_exist(F+name):assert e.delete_asset(F+name)
print('SUPERSEDED_IMPORTS_REMOVED')
