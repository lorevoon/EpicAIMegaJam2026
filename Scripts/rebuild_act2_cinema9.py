import runpy
from pathlib import Path
for name in ['author_act2_cinema9.py','cameras_act2_cinema9.py','finish_act2_cinema9.py','validate_act2_cinema9.py']:runpy.run_path(str(Path(__file__).parent/name),run_name='__main__')
