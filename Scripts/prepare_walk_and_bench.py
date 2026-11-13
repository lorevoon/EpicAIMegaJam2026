from pathlib import Path
for name in ['audit_bench_walk.py','retarget_juliet_female_walk.py']:
 p=Path(__file__).parent/name
 exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
