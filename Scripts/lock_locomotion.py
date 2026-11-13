import unreal as u
for who in ['Archer','Swordsman']:
    for name in ['Unarmed_WalkForward','Support_Walk','Support_Run']:
        a=u.load_asset('/Game/DirectorsCut/Animations/'+who+'/A_'+who+'_'+name)
        if a:
            a.modify()
            a.set_editor_property('force_root_lock',True)
            a.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.ANIM_FIRST_FRAME)
            u.EditorAssetLibrary.save_loaded_asset(a,False)
u.EditorLevelLibrary.editor_set_game_view(True)
