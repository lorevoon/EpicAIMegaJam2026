import json,subprocess,sys,time
from pathlib import Path
root=Path.cwd();j=json.loads((root/'Saved/Act2/AudioFaces/report.json').read_text())
def call(ts,n,args,file=None):
 cmd=[sys.executable,'Scripts/ue_mcp.py','call_tool',json.dumps(dict(toolset_name=ts,tool_name=n,arguments=args))]
 if file:cmd.append(str(root/'Saved/Act2/AudioFaces'/file))
 subprocess.run(cmd,check=True)
call('EditorToolset.EditorAppToolset','StartPIE',{'options':{'bSimulate':False,'playMode':'PlayMode_InViewPort','warmupSeconds':0}});start=time.monotonic()
for key,t in [('Romeo_17.3',17.2),('Juliet_21',21),('Juliet_35',35)]:
 time.sleep(max(0,t-(time.monotonic()-start)));r=j['preview'][key];p=r['position'];call('EditorToolset.EditorAppToolset','CaptureViewport',{'captureTransform':{'location':dict(zip(['x','y','z'],p)),'rotation':r['rotation'],'scale':{'x':1,'y':1,'z':1}},'annotations':None,'bShowUI':False},'PIE_'+key+'.png')
print('ACT2_RUNTIME_FACE_PREVIEWS_COMPLETE')
