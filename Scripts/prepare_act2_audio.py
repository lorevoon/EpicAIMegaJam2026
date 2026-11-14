"""Build a seekable 38-second soundtrack and amplitude gates from supplied recordings."""
from pathlib import Path
import json,sys,subprocess,wave,shutil
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Saved/VoiceTools'))
import imageio_ffmpeg
FF=imageio_ffmpeg.get_ffmpeg_exe();RATE=48000;N=38*RATE
OUT=ROOT/'SourceArt/Act2/Audio';OUT.mkdir(parents=True,exist_ok=True)
RAW=OUT/'Originals';RAW.mkdir(exist_ok=True)
def decode(name,trim=False):
 p=Path('C:/Users/iris6/Downloads')/name;shutil.copy2(p,RAW/name)
 a=np.frombuffer(subprocess.check_output([FF,'-v','error','-i',str(p),'-f','f32le','-ac','2','-ar',str(RATE),'-']),np.float32).reshape(-1,2).copy();lo=0
 if trim:
  mono=a.mean(axis=1);rms=np.array([np.sqrt(np.mean(mono[i:i+480]**2)) for i in range(0,len(a),480)])
  active=np.where(rms>max(.003,float(rms.max())*.025))[0]
  lo=max(0,int(active[0]*480-.07*RATE));hi=min(len(a),int((active[-1]+1)*480+.13*RATE));a=a[lo:hi]
 return a,lo/RATE
def save(name,a):
 with wave.open(str(OUT/(name+'.wav')),'wb') as w:
  w.setnchannels(2);w.setsampwidth(2);w.setframerate(RATE);w.writeframes((np.clip(a,-1,1)*32767).astype('<i2').tobytes())
def fade(a,d=.025):
 n=min(len(a)//2,int(d*RATE));a[:n]*=np.linspace(0,1,n)[:,None];a[-n:]*=np.linspace(1,0,n)[:,None];return a
lines=[];events=[];voice=np.zeros((N,2));effects=np.zeros_like(voice);bed=np.zeros_like(voice)
spec=[('Mr. Smith','Warning','1.fire-watch-out-bill.mp3',.25),('Mr. Smith','Death','2. death-scream (mp3cut.net).mp3',7.5),('Romeo','ShortNo','george-short-no.mp3',10.0),('Romeo','RunJuliet','4. run-juliet-scream (mp3cut.net).mp3',None),('Juliet','NotLeaving','5. not-leaving-you.mp3',None),('Romeo','SwordCry','6. george-sword-slash-scream.mp3',19.35),('Juliet','RomeoCry','7. romeo-scream.mp3',26.15),('Romeo','Goodbye','8. george-dying-goodbye.mp3',28.25),('Juliet','YouPromised','you-promised-agony.mp3',None),('Juliet','Sobbing','10. intense-sobbing.mp3',26.4)]
for who,name,file,start in spec:
 # Chain the reaction exchange tightly using the actual trimmed recording lengths.
 if start is None:start=lines[-1]['end']+(.08 if name=='YouPromised' else .04)
 if name=='YouPromised':
  # Two local ASR passes plus cropped-segment checks identify the first 3.45s as
  # nonverbal sobs. Preserve every word and trailing sobs at original speed.
  a,_=decode(file,False);trim=3.45;a=a[round(trim*RATE):round(8.30*RATE)]
  assert start+len(a)/RATE<38,'Never truncate the final spoken line to fit the take.'
 else:a,trim=decode(file,True)
 a=a[:N-round(start*RATE)];a=fade(a);peak=float(np.max(np.abs(a)));a*=min(3,.68/max(peak,.001))
 if name=='Sobbing':
  # Keep sobbing beneath the spoken farewell, then let it carry the silent ending.
  ts=start+np.arange(len(a))/RATE;gain=.24+.38*np.clip((ts-32.93)/.35,0,1)
  for spoken in lines:
   if spoken['name']=='YouPromised':
    weight=np.minimum(np.clip((ts-spoken['start']+.10)/.10,0,1),np.clip((spoken['end']+.15-ts)/.15,0,1));gain*=1-.85*weight
  a*=gain[:,None];fade(a,.22)
 save(name,a);off=round(start*RATE);voice[off:off+len(a)]+=a
 mono=a.mean(axis=1);gate=[]
 for f in range(1141):
  i=round((f/30-start)*RATE);chunk=mono[max(0,i-720):min(len(a),i+720)] if 0<=i<len(a) else []
  energy=float(np.sqrt(np.mean(np.square(chunk)))) if len(chunk) else 0;gate.append(round(min(1,energy/.025),4))
 lines.append(dict(speaker=who,name=name,source=file,start=start,end=start+len(a)/RATE,trim_start=trim,mouth_gate=gate))
for name,file,start,gain in [('Growl1','creature-orc-growling-01.wav',.4,.38),('Growl2','creature-orc-growling-02.wav',2.5,.38),('Growl3','creature-orc-growling-03.wav',4.35,.4),('SmithAttack','creature-orc-attack-01.wav',6.65,.5),('SmithHit','weapons-sword-sword-hit-02.wav',7.45,.42),('RomeoClash','weapons-sword-sword-hit-01.wav',19.6,.58),('OrcAttack','creature-orc-attack-01.wav',22.15,.5),('OrcClash','weapons-sword-sword-hit-02.wav',22.6,.58),('FinalAttack','creature-orc-attack-01.wav',25.1,.45),('FinalClash','weapons-sword-sword-hit-01.wav',25.55,.6),('FinalImpact','weapons-sword-sword-hit-02.wav',25.7,.45),('OrcDeath','creature-orc-call-pain-long-01.wav',26,.5)]:
 a,_=decode(file);a=fade(a);a*=gain/max(.001,float(np.max(np.abs(a))));off=round(start*RATE);a=a[:N-off];effects[off:off+len(a)]+=a;events.append(dict(name=name,source=file,start=start,end=start+len(a)/RATE))
# Loop fire with an overlap crossfade, never a hard seam. Music is a continuous excerpt.
fire,_=decode('atmo-fire-camp-fire-crackle-03.wav');fire*=.13/max(.001,float(np.max(np.abs(fire))));overlap=RATE
firelong=np.zeros_like(bed)
for off in range(0,N,len(fire)-overlap):
 a=fade(fire.copy(),1);a=a[:N-off];firelong[off:off+len(a)]+=a
music,_=decode('burning_fire_bgm.wav');music=fade(music[:N],1);music*=.17/max(.001,float(np.max(np.abs(music))))
duck=np.ones(N)
for l in lines:
 if l['name']=='Sobbing':continue
 t=np.arange(N)/RATE;weight=np.minimum(np.clip((t-l['start']+.15)/.15,0,1),np.clip((l['end']+.3-t)/.3,0,1));duck=np.minimum(duck,1-.65*weight)
bed=(music+firelong)*duck[:,None];master=voice+effects+bed
master*=min(1,.94/max(.001,float(np.max(np.abs(master)))));fade(master,.15)
save('Act2_Soundtrack',master);save('Act2_Voices',voice*.7);save('Act2_SFX',effects);save('Act2_Background',bed)
report=dict(duration=38,fps=30,lines=lines,effects=events,peak=float(np.max(np.abs(master))),background=['burning_fire_bgm.wav','atmo-fire-camp-fire-crackle-03.wav'])
(OUT/'timing.json').write_text(json.dumps(report,indent=2));print(json.dumps([{k:v for k,v in l.items() if k!='mouth_gate'} for l in lines],indent=2));print('ACT2_MIX_READY',report['peak'])
