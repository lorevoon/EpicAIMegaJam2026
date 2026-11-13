import {generate, status, result} from 'file:///C:/Users/iris6/AppData/Roaming/npm/node_modules/@hyper3d/cli/dist/operations.js';
import fs from 'node:fs/promises';
import path from 'node:path';
const root = path.dirname(new URL(import.meta.url).pathname.replace(/^\/(?=[A-Za-z]:)/, ''));
const dir = decodeURIComponent(root);
const temp = 'C:/Users/iris6/AppData/Local/Temp/';
const refs = {
 crane:'codex-clipboard-94fc5c86-e19f-4395-b3b0-cdb24ef9b8df.png',
 box:'codex-clipboard-f02a402e-5d15-4cbe-b3fe-f4306b00b3a1.png',
 lights:'codex-clipboard-5ced2830-eeb3-4544-a969-b6c9e65b4ec5.png',
 director:'codex-clipboard-d355eef9-cf68-4e68-9001-987df9bf559b.png'
};
const common = 'Create ONLY the single requested prop as an isolated textured 3D game asset. Reference image contains multiple objects: exclude every other object, ground, backdrop, lettering, watermarks and logos. Match the reference stylized toon movie set aesthetic: chunky readable silhouette, simple clean beveled geometry, muted colors, matte surfaces, minimal realistic wear. Complete geometry on all sides. ';
const specs = [
 ['01_jib','crane','ONLY the dark charcoal teal camera jib crane assembly, WITHOUT camera and WITHOUT rails. Include the low rectangular four-wheel dolly base with pale gray wheels, upright cylindrical pivot pedestal, long rectangular boom angled upward, thinner parallel upper support rod, vertical end mounting bracket, short rear arm with stacked round counterweights. Preserve open spaces between rods and the base frame.'],
 ['02_crane_camera','crane','ONLY the small vintage cinema camera hanging from the end of the crane, isolated with no crane, no pedestal and no rails. Dark charcoal teal boxy camera body, rounded oval top film magazine, long cylindrical lens projecting forward with wider front lens ring and dark glass, small mounting bracket above camera. Compact chunky proportions.'],
 ['03_rails','crane','ONLY one straight camera dolly rail track section. Two parallel long dark brown metal rails joined by evenly spaced dark teal perpendicular cross ties, approximately 4 meters long and 0.8 meters wide. Low profile, straight and symmetrical with open spaces between cross ties. No crane, no camera, no dolly and no wheels.'],
 ['04_big_box','box','ONLY one large closed rectangular film equipment flight case like the front left box. Dark desaturated teal panels, pale silver aluminum trim around every edge and around the lid seam, black rounded protective corner caps, recessed silver carrying handles on short sides. Wide horizontal trunk, roughly 1.2 meters wide, 0.65 meters deep, 0.75 meters tall. No other boxes or ladder.'],
 ['05_ladder','box','ONLY the tall freestanding open wooden A-frame stepladder. Honey tan wooden rectangular side rails, evenly spaced thick rungs on both sides, hinge bolts at top, four black rubber feet, sturdy slightly tapered silhouette. Approximately 2.4 meters tall. Preserve all open spaces between rungs. No boxes.'],
 ['06_light_tall_rectangular','lights','ONLY the tallest light at left: a small rectangular warm ivory lamp face in ochre yellow housing, four dark barn doors, U-shaped tilt yoke, silver telescoping upper pole, tall brown wooden tripod with three splayed legs and black feet. Full freestanding light approximately 2.6 meters tall. No other lights.'],
 ['07_light_medium_round','lights','ONLY the center light: medium height compact round studio spotlight, pale blue faceted housing, circular ivory lens, four dark gray barn doors and tilt yoke, slim black telescoping metal stand with three low splayed feet. Approximately 1.8 meters tall. No other lights.'],
 ['08_light_short_large','lights','ONLY the right light as a shorter sturdy light: large deep barrel-shaped pale blue studio Fresnel spotlight housing, broad circular ivory lens, four broad dark barn doors, black U-yoke, short black telescoping stand with three splayed feet. Approximately 1.3 meters tall overall, visually distinct from the taller slim center light. A simple black power cable down the stand. No other lights.'],
 ['09_director_chair','director','ONLY the wooden folding director chair in center. Warm honey brown frame, crossed X-shaped folding legs with long feet, broad wooden armrests, cream canvas sling seat and rectangular cream canvas back stretched between two vertical posts with rounded finials. Empty chair, no person, no objects around or on it. Approximately 0.95 meters tall.'],
 ['10_director_megaphone','director','ONLY the handheld megaphone at left. Broad flared ivory white hollow conical horn with thick rounded rim and small inner sound cone, muted blue rear housing and blue angled pistol grip with small ivory trigger. Horn approximately 0.35 meters diameter, entire prop 0.5 meters long. No chair or other objects.'],
 ['11_director_slate','director','ONLY the black movie clapperboard at lower right. Thin rectangular charcoal black slate with fine white ruled production fields, hinged top clapper bar with alternating diagonal black and ivory white stripes, small round hinge bolt on left. Top bar slightly open. Approximately 0.3 meters wide. No chair or other props.']
];
const manifestPath=path.join(dir,'isolated_manifest.json');
let jobs;
try {jobs=JSON.parse(await fs.readFile(manifestPath,'utf8'));} catch {jobs=specs.map(([name,ref,description])=>({name,reference:temp+refs[ref],prompt:common+description}));}
const save=()=>fs.writeFile(manifestPath,JSON.stringify(jobs,null,2));
const mode=process.argv[2]||'generate';
if(mode==='generate') {
 for(const job of jobs) {
  if(job.generation) continue;
  console.log('Submitting '+job.name);
  try {job.generation=await generate({prompt:job.prompt.replace(common,'A single isolated stylized toon movie studio prop. Clean hard surface geometry, straight edges, subtly rounded bevels, chunky simple shapes, matte materials and muted colors. No scene, no floor, no surrounding objects. '),tier:'Gen-2.5-Medium',meshMode:'Quad',quality:12000,format:'glb'});await save();console.log(JSON.stringify({name:job.name,generation:job.generation}));}
  catch(e){job.error=e.message;await save();console.error(e.message);break;}
 }
} else {
 for(const job of jobs) {
  if(!job.generation)continue;
  const id=job.generation.generation_id;
  if(!id){console.log(JSON.stringify(job.generation));continue;}
  try {
   job.status=await status(id);
   if(job.status.status==='completed') {
    job.result=await result(id);
    const file=job.result.files.find(f=>f.name.endsWith('_pbr.glb'));
    if(file){const folder=path.join(dir,'Individual',job.name);await fs.mkdir(folder,{recursive:true});const dest=path.join(folder,'original.glb');try{await fs.access(dest);}catch{const response=await fetch(file.url);if(!response.ok)throw Error('Download HTTP '+response.status);const bytes=Buffer.from(await response.arrayBuffer());if(bytes.toString('ascii',0,4)!=='glTF')throw Error('Invalid GLB');await fs.writeFile(dest,bytes);}job.local_model=dest;}
   }
   await save();console.log(JSON.stringify({name:job.name,status:job.status,local_model:job.local_model}));
  }catch(e){console.error(job.name+': '+e.message);}
 }
}
