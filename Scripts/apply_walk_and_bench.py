from pathlib import Path
for name in ['smooth_act1_motion.py','align_bench_release.py']:
 p=Path(__file__).parent/name
 exec(compile(p.read_text(),str(p),'exec'),{'__file__':str(p)})
import unreal as u
assert u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
u.log('WALK_AND_BENCH_SAVED_FOR_REBUILD')
