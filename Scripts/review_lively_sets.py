import runpy
from pathlib import Path
for name in ['validate_lively_sets.py','render_lively_sets.py']:
 runpy.run_path(str(Path(__file__).parent/name),run_name='__main__')
