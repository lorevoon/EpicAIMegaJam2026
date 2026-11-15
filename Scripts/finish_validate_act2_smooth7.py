import runpy
from pathlib import Path
root=Path(__file__).parent
for script in ['finish_act2_smooth7.py','validate_act2_smooth7.py']:runpy.run_path(str(root/script),run_name='__main__')
