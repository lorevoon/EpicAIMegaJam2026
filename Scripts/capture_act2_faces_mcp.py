import json,subprocess,sys
from pathlib import Path
root=Path.cwd();j=json.loads((root/'Saved/Act2/AudioFaces/report.json').read_text())
def call(ts,n,args,file=None):
 cmd=[sys.executable,'Scripts/ue_mcp.py','call_tool',json.dumps(dict(toolset_name=ts,tool_name=n,arguments=args))]
 if file:cmd.append(str(root/'Saved/Act2/AudioFaces'/file))
 subprocess.run(cmd,check=True)
for key,f in [('Romeo_17.3',519),('Romeo_17.45',524),('Juliet_21',630),('Juliet_35',1050)]:
 call('animation_toolset.toolsets.sequencer.SequencerTools','set_playhead_frame',{'frame':f})
 r=j['preview'][key];p=r['position'];call('EditorToolset.EditorAppToolset','CaptureViewport',{'captureTransform':{'location':dict(zip(['x','y','z'],p)),'rotation':r['rotation'],'scale':{'x':1,'y':1,'z':1}},'annotations':None,'bShowUI':False},key+'.png')
