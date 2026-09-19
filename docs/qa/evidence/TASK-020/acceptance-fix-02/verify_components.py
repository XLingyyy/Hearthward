"""Regression: component geometry, hit testing, visibility and persisted layouts in real PIE."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
layout_file=Path(unreal.Paths.project_dir())/'Resources/UI/layout.json'
original_layout=layout_file.read_bytes()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'scope':'real PIE state, native captures, component transforms and input hit testing'}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>end
def rows(ui):return {r['id']:r for r in json.loads(ui.describe_layout())['components']}
def vector(x,y):return unreal.Vector2D(x,y)
def center(row):
    x,y,w,h=row['rect'];return vector(x+w*.5,y+h*.5)
def finish():
    layout_file.write_bytes(original_layout)
    (out/'fix02-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
def run():
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('title_capture',ui.capture_ui('fix02-title',1672,941))
    ui.open_page('save');check('empty_save_capture',ui.capture_ui('fix02-save-empty',1672,941))
    check('save_has_own_sheet', 'save.sheet' in rows(ui) and 'pause.menu' not in rows(ui))
    check('new_campaign',ui.execute_action('new'))
    pawn=unreal.GameplayStatics.get_player_pawn(world,0);game=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    for item in ('axe','hood','armor','gloves','belt','boots','shield','bow','quiver','amulet'):game.equip(item)
    for page in ('inventory','storage','pause','dialogue','map','skills','journal','hud','save','settings'):
        ui.open_page(page);yield delay(.2)
        current=rows(ui)
        check(page+'_has_components',len(current)>3)
        check(page+'_capture',ui.capture_ui('fix02-'+page,1672,941))
        if page in ('save','inventory','map'):check(page+'_small_capture',ui.capture_ui('fix02-'+page+'-1280',1280,720))
    # Move/resize a real menu including its surface, labels and hit targets.
    ui.open_page('pause');before=rows(ui)
    button=next(r for r in before.values() if r.get('action')=='page:settings')
    check('original_pause_hit',ui.action_at(center(button))=='page:settings')
    check('move_resize_pause',ui.set_component_rect('pause.menu',vector(170,100),vector(364,672)))
    after=rows(ui);moved=after[button['id']]
    x,y,w,h=button['rect']
    expected=[170+(x-587)*.8,100+(y-50)*.8,w*.8,h*.8]
    check('children_follow_parent',all(abs(a-b)<.01 for a,b in zip(moved['rect'],expected)))
    check('hit_moves_with_button',ui.action_at(center(moved))=='page:settings')
    check('old_hit_released',ui.action_at(center(button))!='page:settings')
    check('move_capture',ui.capture_ui('fix02-pause-moved',1672,941))
    check('hide_pause_group',ui.set_component_visible('pause.menu',False))
    check('hidden_descendants',all(not r['visible'] for r in rows(ui).values() if r['parent']=='pause.menu'))
    check('hidden_not_clickable',ui.action_at(center(moved))!='page:settings')
    check('hide_capture',ui.capture_ui('fix02-pause-hidden',1672,941))
    ui.set_component_visible('pause.menu',True)
    check('save_layout',ui.save_layout())
    ui.set_component_rect('pause.menu',vector(600,50),vector(400,800))
    check('reload_layout',ui.reload_layout())
    check('persisted_geometry',rows(ui)['pause.menu']['rect']==[170,100,364,672])
    # A single decorative element moves independently without changing the group.
    check('move_ornament',ui.set_component_rect('pause.ornament.top',vector(220,130),vector(180,28)))
    check('leaf_geometry',all(abs(a-b)<.01 for a,b in zip(rows(ui)['pause.ornament.top']['rect'],[220,130,180,28])))
    check('parent_unchanged',rows(ui)['pause.menu']['rect']==[170,100,364,672])
    layout_file.write_bytes(original_layout);ui.reload_layout()
    # Every surface can be removed, leaving no baked paper behind.
    for page in ('inventory','storage','pause','skills','journal','save','settings'):
        ui.open_page(page)
        for row in list(rows(ui).values()):
            if row['id'].startswith('surface:'):ui.set_component_visible(row['id'],False)
        check(page+'_surfaces_detached',ui.capture_ui('fix02-'+page+'-surfaces-hidden',1672,941))
    layout_file.write_bytes(original_layout);ui.reload_layout()
    # Native editable textbox uses the same transformed bounds as its component.
    ui.open_page('dialogue');base=rows(ui)['dialogue.input']['rect']
    check('move_dialogue',ui.set_component_rect('dialogue.panel',vector(350,250),vector(585,562.5)))
    field=rows(ui)['dialogue.input'];x,y,w,h=base
    check('input_follows_panel',all(abs(a-b)<.01 for a,b in zip(field['rect'],[350+(x-950)*.9,250+(y-278)*.9,w*.9,h*.9])))
    ui.set_layout_editing(True);check('editor_capture',ui.capture_ui('fix02-editor-dialogue',1672,941))
    ui.set_layout_editing(False);check('moved_input_capture',ui.capture_ui('fix02-dialogue-moved',1672,941))
    ui.open_page('map');before=rows(ui)
    marker=next(r for r in before.values() if r.get('action','').startswith('location:'))
    check('move_map',ui.set_component_rect('map.canvas',vector(460,160),vector(888,608)))
    marker_new=rows(ui)[marker['id']]
    check('map_marker_hit',ui.action_at(center(marker_new))==marker['action'])
    check('map_moved_capture',ui.capture_ui('fix02-map-moved',1672,941))
    check('invalid_size_rejected',not ui.set_component_rect('map.canvas',vector(0,0),vector(0,0)))
    layout_file.write_bytes(original_layout);ui.reload_layout()
    ui.open_page('save')
    check('save_page_no_pause_components',all(not k.startswith('pause.') for k in rows(ui)))
    report['passed']=True;finish()
iterator=run();pending=None;deadline=time.monotonic()+150
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('component regression')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
