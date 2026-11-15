import runpy
from pathlib import Path
for name in ['materials_lively_set.py','dress_lively_sets.py','validate_lively_sets.py']:
 runpy.run_path(str(Path(__file__).parent/name),run_name='__main__')
