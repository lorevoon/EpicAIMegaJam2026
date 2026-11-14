"""Local word timestamps used to edit only pauses in the supplied final line."""
from pathlib import Path
import sys,json,os,subprocess
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'Saved/WhisperTools'))
os.environ['HF_HOME']=str(root/'Saved/WhisperModels')
from faster_whisper import WhisperModel
import numpy as np
sys.path.insert(0,str(root/'Saved/VoiceTools'));import imageio_ffmpeg
model=WhisperModel('base.en',device='cpu',compute_type='int8',download_root=str(root/'Saved/WhisperModels'))
report={}
for name in ['you-promised-agony.mp3','8. george-dying-goodbye.mp3']:
 audio=np.frombuffer(subprocess.check_output([imageio_ffmpeg.get_ffmpeg_exe(),'-v','error','-i','C:/Users/iris6/Downloads/'+name,'-f','f32le','-ac','1','-ar','16000','-']),np.float32).copy()
 segments,info=model.transcribe(audio,language='en',word_timestamps=True,beam_size=5,vad_filter=False)
 report[name]=[{'start':s.start,'end':s.end,'text':s.text,'words':[{'start':w.start,'end':w.end,'word':w.word,'probability':w.probability} for w in s.words]} for s in segments]
out=root/'Saved/Act2/EndingAudio';out.mkdir(parents=True,exist_ok=True)
(out/'transcript.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
