from pathlib import Path
root=Path(__file__).resolve().parent
for name in ["reblock_act2_revision3.py","finish_act2_revision3.py","validate_act2_revision3.py"]:
 p=root/name
 exec(compile(p.read_text(),str(p),"exec"),{"__file__":str(p),"__name__":"__main__"})
