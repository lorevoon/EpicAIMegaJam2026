"""Execute a project Python script in this project's already-open Unreal Editor."""
import sys,time,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,r'C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python')
import remote_execution as remote
client=remote.RemoteExecution()
try:
 client.start();time.sleep(2)
 nodes=[n for n in client.remote_nodes if Path(n.get('project_root','')).resolve()==ROOT]
 assert len(nodes)==1,nodes
 client.open_command_connection(nodes[0]['node_id'])
 script=Path(sys.argv[1]).resolve();assert script.is_relative_to(ROOT)
 command='exec(compile(open('+repr(str(script))+', encoding="utf-8").read(),'+repr(str(script))+',"exec"),{"__file__":'+repr(str(script))+',"__name__":"__main__"})'
 result=client.run_command(command,raise_on_failure=False)
 print(json.dumps(result,indent=2))
 if not result.get('success'):sys.exit(1)
finally:client.stop()
