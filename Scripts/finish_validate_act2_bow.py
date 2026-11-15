from pathlib import Path
root=Path(__file__).resolve().parent
for name in ['finish_act2_bow.py','validate_act2_bow.py']:
 path=root/name
 exec(compile(path.read_text(),str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
