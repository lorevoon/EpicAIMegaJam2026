from pathlib import Path
import runpy
p=Path(__file__).parent/'author_kiss12.py'
exec(compile(p.read_text().split("assert les.load_level")[0],str(p),'exec'))
runpy.run_path(str(Path(__file__).parent/'validate_kiss12.py'),run_name='__main__')
