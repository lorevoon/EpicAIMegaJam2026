"""Retime the practical studio take to 15 seconds, including animation and prop events."""
import unreal as u
from pathlib import Path
root=Path(__file__).resolve().parents[1]
level='/Game/DirectorsCut/Levels/L_DC_Act1_Promise'
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(level)
actors=list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
director=next(a for a in actors if isinstance(a,u.DCZombieDirector))
assert director.get_editor_property('take_duration') in [15.,40.], 'Legacy 15-second retimer; do not apply to the expanded 20-second studio.'
scale=15.0/director.get_editor_property('take_duration')
for a in actors:
    if isinstance(a,u.DCWaypoint):a.set_editor_property('time',a.get_editor_property('time')*scale)
    if isinstance(a,u.DCPerformer):
        cues=list(a.get_editor_property('animation_cues'))
        for cue in cues:
            cue.set_editor_property('start_time',cue.get_editor_property('start_time')*scale)
            cue.set_editor_property('play_rate',cue.get_editor_property('play_rate')/scale)
        a.set_editor_property('animation_cues',cues)
        t=a.get_editor_property('set_down_time')
        if t>=0:a.set_editor_property('set_down_time',t*scale)
    if isinstance(a,u.DCBeat):
        for prop in ['window_start','window_end','pose_ready_time']:
            a.set_editor_property(prop,a.get_editor_property(prop)*scale)
        a.set_editor_property('required_hold',1.0)
        windows={'1.1':(.75,3.75,2.625),'1.2':(4.125,7.125,5.25),'1.3':(8.25,10.875,9.),'1.4':(11.625,14.25,12.75)}
        for prop,value in zip(['window_start','window_end','pose_ready_time'],windows[str(a.get_editor_property('beat_id'))]):a.set_editor_property(prop,value)
director.set_editor_property('take_duration',15.)
director.set_actor_label('Act 1 - The Promise - practical studio (15 seconds)')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world,level)
exec((root/'Scripts/validate_act1_film_set.py').read_text())
