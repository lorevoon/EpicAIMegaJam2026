import runpy
from pathlib import Path
for name in ['align_act2_mobile8.py','finish_act2_mobile8.py','validate_act2_mobile8.py']:runpy.run_path(str(Path(__file__).parent/name),run_name='__main__')
