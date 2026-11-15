FILM CREW / FIVE POLYSPLIT HERO VARIANTS

LATEST REVISION: use v2/ for the thin fabric outfits and realistic work shoes.
Open FilmCrew_Outfits_v2.blend or view FilmCrew_Outfits_v2.png.
The files described below are the retained V1 baseline.

Files:
- FilmCrew_5Variants.blend: editable source, posed lineup, lights and camera.
- Crew_*.fbx: individual rigged characters in their original rest pose.
- Crew_*.glb: individual portable characters with embedded materials/textures.
- FilmCrew_Lineup.png: rendered overview.
- Textures/Hero_Body.png: original pack's bodyColor1 atlas, copied for portability.
- manifest.json: source character and hair selections.
- validation.json: FBX round-trip checks.
- build_crew.py: reproducible Blender construction/export script.

Three female variants: auburn bun, blonde bob, black ponytail.
Two male variants: brown crop, silver longer cut.
Each includes a dark baseball cap, black short-sleeved shirt and trousers,
headset with boom microphone, and a hip-slung hollow megaphone.

Source:
C:/Unity Projects/COMP 3329 POC/Assets/PolysplitGames/LowPolyMedievalFantasyHeroes
Original source files were read, not modified. Original heads, face meshes,
hair selections, body proportions, bone names and source body skin weights are retained.
Trousers derive from the source legs; shirts have new cloth topology with
weights transferred from the source torso. Skin faces concealed by the shirt
are culled in the exported copies to prevent clothing poke-through. New accessories
are weighted to the original head/pelvis bones. No generated replacement hero.

Import:
Import one FBX or GLB per character. Character height including cap/headset
is approximately 2.10 metres, matching the source character scale.
FBX embeds the atlas; Textures contains an explicit copy if the engine's
importer needs one. Material slots distinguish cotton, cap, hair, equipment
and body atlas. In Unity, configure the FBX Rig tab for the desired animation
workflow. In Unreal, import as skeletal mesh; verify skeleton mapping before
sharing animations. No engine-specific prefab, animation retargeting setup,
or gameplay integration is included.

Validation:
All five FBXs were reimported into Blender and checked for one armature,
99 bones, valid overall bounds and zero unweighted mesh vertices.
The source blend includes an arms-lowered preview pose. Exported characters
use the source rest pose. Runtime animation compatibility needs testing in
the target engine; no animation clips are included.

Tool choice:
Blender was chosen for direct mesh and rig preservation. MCP connection
attempts failed, so creation and export used Blender 5.0.1's local Python API.

These are derivatives of the user's licensed source assets; the source
asset licence still applies. Keep this folder within the licensed project.
