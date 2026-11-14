"""Tune Act 2-only Niagara copies through the running editor's supported MCP tools."""
import json
from ue_mcp import post
res,sid=post({'jsonrpc':'2.0','id':1,'method':'initialize','params':{'protocolVersion':'2025-03-26','capabilities':{},'clientInfo':{'name':'act2-fire-authoring','version':'1'}}})
post({'jsonrpc':'2.0','method':'notifications/initialized'},sid)
def call(ts,name,args):
 res,_=post({'jsonrpc':'2.0','id':2,'method':'tools/call','params':{'name':'call_tool','arguments':{'toolset_name':ts,'tool_name':name,'arguments':args}}},sid)
 assert not res.get('error') and not res.get('result',{}).get('isError'),res
 texts=[c['text'] for c in res['result'].get('content',[]) if c.get('type')=='text']
 print(name,(' '.join(texts))[:160],flush=True)
 return texts
assets='editor_toolset.toolsets.asset.AssetTools';ns='NiagaraToolsets.NiagaraToolset_System'
for suffix in ['', '_Smoke']:
 src='/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big'+suffix;dst='/Game/DirectorsCut/Act2/NS_Act2_VillageInferno'+suffix
 call(assets,'duplicate',{'path':src,'new_path':dst})
 for emitter in ['NE_Flame_01','NE_Flame_02']:
  for input_name,value in [('Uniform Sprite Size Min',650),('Uniform Sprite Size Max',950),('Lifetime Min',1.2),('Lifetime Max',1.8)]:
   ref={'system':{'refPath':dst+'.'+dst.rsplit('/',1)[-1]},'emitterName':emitter,'scriptName':'ParticleSpawnScript','moduleName':'InitializeParticle','rendererIndex':-1,'inputNameStack':[input_name]}
   call(ns,'SetStackInputData',{'stackInputRef':ref,'inputData':{'struct':{'refPath':'/Script/Niagara.NiagaraFloat'},'value':{'value':value}}})
  ref['moduleName']='AddVelocity';ref['inputNameStack']=['Scale Added Velocity']
  call(ns,'SetStackInputData',{'stackInputRef':ref,'inputData':{'struct':{'refPath':'/Script/CoreUObject.Vector3f'},'value':{'x':1,'y':1,'z':4}}})
 call(assets,'save_assets',{'asset_paths':[dst]})
print('ACT2_INFERNO_VARIANTS_SAVED')
