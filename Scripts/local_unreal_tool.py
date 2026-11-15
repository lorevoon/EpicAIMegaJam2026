import json,urllib.request,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1]
headers=json.loads((R/'Saved/unreal_mcp_session.json').read_text())
params=json.loads(sys.argv[1])
request=urllib.request.Request('http://127.0.0.1:8000/mcp',data=json.dumps(dict(jsonrpc='2.0',id=3,method='tools/call',params=params)).encode(),headers=headers)
print(urllib.request.urlopen(request,timeout=120).read().decode())
