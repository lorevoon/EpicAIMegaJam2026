"""Apply the final camera framing, refresh sequence, and validate saved assets."""
from pathlib import Path
for name in ['author_act2_continuous_camera.py','finish_act2_revision4.py','validate_act2_revision4.py']:
 path=Path(__file__).with_name(name)
 exec(compile(path.read_text(),str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
