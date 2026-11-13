from pathlib import Path
for name in ['smooth_act1_motion.py','smooth_act1_crew.py','smooth_act1_routes.py']:
 path=Path(__file__).parent/name
 exec(compile(path.read_text(),str(path),'exec'),{'__file__':str(path)})
