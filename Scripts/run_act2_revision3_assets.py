from pathlib import Path
root=Path(__file__).resolve().parent
for name in ['import_act2_revision3.py', 'patch_act2_revision3_reactions.py', 'finish_act2_revision3.py', 'author_act2_audio_faces.py']:
 p=root/name
 exec(compile(p.read_text(),str(p),"exec"),{"__file__":str(p),"__name__":"__main__"})
print("ACT2_REVISION3_ASSETS_READY")
