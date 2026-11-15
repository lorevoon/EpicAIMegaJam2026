"""Rebuild the mix with the supplied full line, keeping the existing final goodbye."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root/'Scripts/prepare_act2_audio.py').read_text()
source=source.replace("OUT=ROOT/'SourceArt/Act2/Audio'", "OUT=ROOT/'SourceArt/Act2/BowRevision6/Audio'")
source=source.replace('4. run-juliet-scream (mp3cut.net).mp3','run-juliet-panicked-scream.mp3')
exec(compile(source,str(root/'Scripts/prepare_act2_audio.py'),'exec'),{'__file__':str(__file__),'__name__':'__main__'})
