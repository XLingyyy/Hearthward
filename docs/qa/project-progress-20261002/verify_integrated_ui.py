import unreal,json,time,traceback,hashlib
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir());out=root/'Saved/ProjectProgress/IntegratedUI';out.mkdir(parents=True,exist_ok=True)
(root/'Saved/Task020').mkdir(exist_ok=True)
config=json.loads((root/'Saved/UpdateCompatibility/conflict-config.json').read_text())
source=Path(config['pool']);before=source.read_bytes()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{}};st={}
def check(name,v):
    report['checks'][name]=bool(v)
    if not v:raise AssertionError(name)
def wait(fn,seconds=60):return fn,time.monotonic()+seconds
def delay(seconds):
    t=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=t,seconds+10)
def bind():
    try:
        w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0)
        ui=pc.get_hud().get_editor_property('screen')
        if not ui:return False
        st.update(world=w,ui=ui,save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer()==w))
        return True
    except Exception:return False
def finish(error=None):
    report['ok']=not error and all(report['checks'].values());report['error']=error
    if bind():report['status']=st['save'].get_status()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield wait(bind);yield delay(3)
    ui=st['ui'];save=st['save']
    check('conflict dialog shows automatically',ui.action_at(unreal.Vector2D(1080,770))=='cancel')
    check('confirmation disabled until final page',not ui.execute_action('confirm'))
    check('conflict screenshot',ui.capture_ui('update-conflicts',1672,941))
    check('cancel dialog',ui.execute_action('cancel'))
    check('cancel leaves file unchanged',source.read_bytes()==before)
    check('new game opens conflict instead of generic error',not ui.execute_action('new'))
    check('new game did not delete file',source.read_bytes()==before)
    check('next page',ui.execute_action('compat.next'))
    check('last-page screenshot',ui.capture_ui('update-conflicts-last',1672,941))
    check('confirm selective cleanup',ui.execute_action('confirm'))
    check('cleaned pool loads',save.load_point_index())
    check('save point preserved',len(save.get_points())==1)
    check('original backup exists',any(p.read_bytes()==before for p in (source.parent/'Backups').glob('*.hws')))
    check('success screenshot',ui.capture_ui('update-conflicts-resolved',1672,941))
    check('update action remains on title',ui.action_at(unreal.Vector2D(760,765))=='update.open')
    check('open integrated settings',ui.execute_action('page:settings'))
    for category in ('游戏','显示','图形','音频','控制','键位','辅助功能','教程'):
        check('settings category '+category,ui.execute_action('category:'+category))
    check('settings screenshot',ui.capture_ui('progress-settings',1672,941))
    check('return title',ui.execute_action('back'))
    check('title restored',str(ui.get_page())=='title')
    check('title screenshot',ui.capture_ui('progress-title',1672,941))
    finish()
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if pending:
            fn,end=pending
            if not fn():
                if time.monotonic()>end:raise TimeoutError('UI test timeout')
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
