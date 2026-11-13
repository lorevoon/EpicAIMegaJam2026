# Village Raid animation lookup and reuse manifest

Checked 2026-11-13. Scope: updated GDD v2.1, 4 / 5 / 6 beats. Speech excluded.

## Evidence and status

The exact filenames below were verified on disk under `C:/Unity Projects/COMP 3329 POC/Assets/Animation/`. These are existing local FBX assets, not claims that new downloads were made. Existing Act1 sources are in `SourceArt/Mixamo/Act1`; seven retargeted clips per hero already exist in `Content/DirectorsCut/Animations`.

Official Adobe Mixamo page: https://www.mixamo.com/ . Its JavaScript catalog did not expose readable results through web retrieval, and the connected in-app browser was unavailable. Consequently, unverified search terms below are explicitly marked; no invented catalog IDs or download URLs are supplied.

Adobe's workflow confirms searching the Animations library, previewing on a character, adjusting speed/arm spacing, then downloading: https://helpx.adobe.com/creative-cloud/help/animate-characters-mixamo.html . Rigging/upload guidance: https://helpx.adobe.com/creative-cloud/help/mixamo-rigging-animation.html .

## Exact local clips suitable for reuse

All source paths in this table are relative to the verified Animation folder above.

| Story need | Exact existing FBX | Use / limitation |
|---|---|---|
| Act1 hero, heroine and crew idle | `Pro Melee Axe Pack/unarmed idle.fbx` | Neutral base; retarget per skeleton |
| Act1 couple walk / crew patrol | `Pro Melee Axe Pack/unarmed walk forward.fbx` | Use root movement from authored path; verify foot sliding |
| Act1 gentle looking / promise | `Pro Melee Axe Pack/unarmed idle looking ver. 1.fbx` | Base only; direct head aim and paired pose need authoring |
| Act1 looking alternative | `Pro Melee Axe Pack/unarmed idle looking ver. 2.fbx` | Inspect variation before choosing |
| Act1 turns | `Pro Melee Axe Pack/unarmed turn left 90.fbx`, `unarmed turn right 90.fbx` | Match turn timing to route |
| Act1 hand/prop examination base | `Standing Idle 03 Examine.fbx` | Not a carry, set-down, or shoulder-pat clip |
| Act2 couple charge / crew runner | `Pro Melee Axe Pack/unarmed run forward.fbx` | Run base |
| Act2 armed hero charge | `Pro Melee Axe Pack/standing run forward.fbx` | Axe-pack pose needs sword-hand/grip inspection |
| Act2 draw sword | `Pro Melee Axe Pack/unarmed equip underarm.fbx` or `unarmed equip over shoulder.fbx` | Select according to sword sheath placement |
| Act2 / Act3 armoured orc strike | `Pro Melee Axe Pack/standing melee attack downward.fbx` | Existing melee source; author contact and victim reaction |
| Act3 duel | `Pro Melee Axe Pack/standing melee attack horizontal.fbx`, `standing melee attack backhand.fbx` | Stagger opponent attacks and blocks |
| Act3 duel blocks | `Pro Melee Axe Pack/standing block idle.fbx`, `standing block react large.fbx` | Match contact frames |
| Act3 stab reaction | `Pro Melee Axe Pack/standing react large gut.fbx` | Victim reaction, not an attacker stab |
| Act2 old man / Act3 hero collapse | `Lite Magic Pack/Standing React Death Backward.fbx` | Check silhouette and final ground alignment |
| Alternate death direction | `Pro Longbow Pack/standing death forward 01.fbx`, `standing death backward 01.fbx` | Align old man with box and hero with later cradle |
| Act2 goblin/orc raid locomotion | `Creature Pack/mutant run.fbx`, `mutant walking.fbx` | Stylized body proportions require retarget QC |
| Raider threat/attack | `Creature Pack/mutant roaring.fbx`, `mutant swiping.fbx`, `mutant punch.fbx` | Unarmed raider variants; armoured killer uses blade animation |
| Raider hit/death | `Creature Pack/mutant dying.fbx` | Use where needed by storyboard |
| Archer bow equip | `Pro Longbow Pack/standing equip bow.fbx` | Prop attachment/visibility must match |
| Archer draw and aim | `Pro Longbow Pack/standing draw arrow.fbx`, `standing aim overdraw.fbx` | Hold aim at story moment |
| Archer release | `Pro Longbow Pack/standing aim recoil.fbx` | Animate arrow prop independently at release |
| Archer combat idle | `Pro Longbow Pack/standing idle 01.fbx` | Existing bow idle base |
| Archer run then stop | `Pro Longbow Pack/standing run forward.fbx`, `standing run forward stop.fbx` | Landing descent should be authored path |
| Hit reaction | `Pro Longbow Pack/standing react small from front.fbx` | Only if compatible with intended impact |

## Missing specialized motion: Mixamo search queue

These are search terms, NOT verified exact catalog titles or downloaded clips.

| Priority | GDD action | Search terms | Required authored work |
|---|---|---|---|
| Act1 | Carry box, set down | carrying, carry box, lifting, put down | Two-hand contact on box; hide attachment transfer behind pat |
| Act1 | Old man thanks and pats shoulder | pat shoulder, greeting, thanking | Paired hand-to-shoulder IK/pose; inspect both rigs |
| Act1 | Old man waves couple away | waving, wave | Wave with walking stick kept in other hand |
| Act1 | Affectionate greeting / shared gaze | greeting, idle, look around | Eye/head direction and facing authored together |
| Act1 | Kiss then forehead-to-forehead | kissing, kiss, hugging | Paired alignment and head offsets; do not label generic idle as kiss |
| Act2 | Alarm / reaching / stumble | frightened, reacting, reach, stumble | Box and walking stick callbacks remain readable |
| Act3 | Blade thrust | sword thrust, sword stab, melee stab | Exact sword contact and offset victim reaction |
| Act3 | Archer kneels and cradles hero | kneeling, crying, mourning | Paired kneel + seated/reclined hero; head/hand alignment custom |
| All | Crew with headset | walking, idle, looking around | Headset is model accessory; route creates framing hazard |

## Implementation checks

1. Keep original source FBXs and record provenance. Verify runtime animation skeleton matches target mesh.
2. Use the scene clock for animation time so pause, scrub, rewind and repeated takes stay deterministic.
3. Keep movement on the authored stage route. Reject root-motion double translation and inspect feet on slopes.
4. Inspect hands, weapons, bow and head alignment on the actual Polysplit rigs. Retarget success alone is insufficient.
5. Carry, pat, kiss and cradle are synchronized contact actions. Use explicit authored contact poses/IK when a single-character library clip does not satisfy them.
6. Score Act1 only after the relevant pose is established: pat after box placement, couple walking after greeting, shared lookout gaze, then kiss/forehead hold.
7. Do not import new speech/audio tracks for this task.

## Hyper3D status

User requested skipping Hyper3D after a service 503. Pending OAuth login was canceled. No generation submitted and no generated models claimed. Official `hyper3d-rodin` MCP entry was added, but authentication was not completed. Reference thumbnails are preserved in `SourceArt/Hyper3D/References` for a later retry.
