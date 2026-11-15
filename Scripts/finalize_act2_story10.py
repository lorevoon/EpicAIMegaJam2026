import runpy
from pathlib import Path
for name in ['clear_act2_story10_foreground.py','finish_act2_story10.py','validate_act2_story10.py']:runpy.run_path(str(Path(__file__).parent/name),run_name='__main__')
