"""Prepare supplied Act 1 dialogue without changing pitch or speaking speed."""
import array,json,math,subprocess,sys,wave
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Saved/VoiceTools'))
import imageio_ffmpeg
FFMPEG=imageio_ffmpeg.get_ffmpeg_exe();RATE=48000
OUT=ROOT/'SourceArt/Act1Dialogue';OUT.mkdir(parents=True,exist_ok=True)
# Walk: 7-12 s; face one another: 12.5-16 s; lean into the kiss: 16 s onward.
SOURCES=[('Romeo','LetMeHelp','let-me-help (mp3cut.net).mp3',.15),('Mr. Smith','ThankYou','thank-you-lad-bill.mp3',2.35),('Juliet','Morning','morning-greeting-matilda.mp3',4.0),('Romeo','HeyJuliet','hey-juliet.mp3',5.65),('Juliet','StayBesideMe','stay-beside-me-warmer.mp3',7.3),('Romeo','Promise','whatever-comes-promise.mp3',12.8)]
def save(path,samples):
 with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(samples.tobytes())
master=array.array('h',[0])*(20*RATE);lines=[];previous=0
for who,name,file,start in SOURCES:
 data=subprocess.check_output([FFMPEG,'-v','error','-i',str(Path('C:/Users/iris6/Downloads')/file),'-f','s16le','-ac','1','-ar',str(RATE),'-'])
 samples=array.array('h');samples.frombytes(data)
 window=480;rms=[math.sqrt(sum(x*x for x in samples[i:i+window])/len(samples[i:i+window])) for i in range(0,len(samples),window)]
 threshold=max(70,max(rms)*.012);active=[i for i,v in enumerate(rms) if v>threshold]
 lo=max(0,active[0]*window-int(.055*RATE));hi=min(len(samples),(active[-1]+1)*window+int(.08*RATE))
 samples=samples[lo:hi];duration=len(samples)/RATE
 start=max(start,previous+.12);end=start+duration;previous=end
 assert end<=17.5,(name,end)
 save(OUT/(name+'.wav'),samples)
 offset=round(start*RATE);master[offset:offset+len(samples)]=samples
 # Frame-wise speech gate suppresses mouth movement through internal pauses.
 gates=[]
 for frame in range(601):
  local=frame/30-start;idx=round(local*RATE);chunk=samples[max(0,idx-720):min(len(samples),idx+720)] if 0<=local<duration else []
  energy=math.sqrt(sum(v*v for v in chunk)/len(chunk)) if chunk else 0
  gates.append(round(min(1,energy/max(threshold*2,1)),4))
 lines.append(dict(speaker=who,name=name,source=file,start=round(start,5),end=round(end,5),duration=duration,trim_start=lo/RATE,mouth_gate=gates))
save(OUT/'Act1_Dialogue.wav',master)
(OUT/'timing.json').write_text(json.dumps(dict(duration=20,fps=30,lines=lines),indent=2))
print(json.dumps([{k:v for k,v in l.items() if k!='mouth_gate'} for l in lines],indent=2))
