"""Capture timed checkpoints through the editor's existing MCP preview controls."""
import subprocess,json,time,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def call(toolset,name,args,filename=None):
 cmd=[sys.executable,str(root/'Scripts/ue_mcp.py'),'call_tool',json.dumps({'toolset_name':toolset,'tool_name':name,'arguments':args})]
 if filename:cmd.append(str(root/'Saved/Act2/Revision2'/filename))
 subprocess.run(cmd,check=True)
call('EditorToolset.EditorAppToolset','StartPIE',{'options':{'bSimulate':False,'playMode':'PlayMode_InViewPort','warmupSeconds':0}})
start=time.monotonic()
for t,name in [(3,'entrance.png'),(11,'surprised.png'),(23,'fight.png'),(30,'last_gesture.png'),(35,'holding.png')]:
 time.sleep(max(0,t-(time.monotonic()-start)))
 call('EditorToolset.EditorAppToolset','CaptureEditorImage',{},name)
print('REVISION2_VISUAL_CHECKPOINTS_CAPTURED')
