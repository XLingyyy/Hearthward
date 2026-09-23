"""TASK-027 deterministic PIE integration: authoritative observation is rechecked at confirmation and execution."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task027"
out.mkdir(parents=True, exist_ok=True)
result_path = out / "perception-pie-results.json"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
P = unreal.HearthwardCompanionPhase
report = {"ok": False, "mode": "deterministic-pie", "checks": {}, "events": []}
st = {}

def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)

def wait(predicate, seconds=30):
    return predicate, time.monotonic() + seconds

def delay(seconds):
    until = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= until, seconds + 5)

def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)

def collect_goal(quantity=2):
    g = unreal.HearthwardAgentGoal()
    for key, value in {
        "intent": "collect",
        "item": "wood",
        "quantity": quantity,
        "quantity_mode": "additional_acquired",
        "source_ref": "S1",
    }.items():
        g.set_editor_property(key, value)
    return g

def card(ai, pawn, companion, quantity=2):
    check("card_" + str(len(report["checks"])), ai.set_structured_goal(pawn, companion, collect_goal(quantity)))
    return ai.get_candidate_id()

def run():
    levels.editor_request_begin_play()
    yield wait(lambda: levels.is_in_play_in_editor(), 30)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world, 0)), 30)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn.get_movement_component().stop_movement_immediately()
    pawn.set_actor_location(unreal.Vector(-200, 400, 90), False, True)
    unreal.GameplayStatics.set_game_paused(world, False)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    yield delay(.5)

    companions = unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY")
    check("one_fixture", len(companions) == 1)
    companion = companions[0]
    companion.set_actor_location(unreal.Vector(0, 400, 90), False, True)
    companion.camp.set_actor_location(unreal.Vector(0, 400, 90), False, True)
    companion.source.get_owner().set_actor_location(unreal.Vector(600, 400, 90), False, True)
    pawn.set_actor_location(unreal.Vector(-200, 400, 90), False, True)

    ai = subsystem(unreal.HearthwardLocalAISubsystem, world)
    storage = subsystem(unreal.HearthwardStorageSubsystem, world)
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    check("gameplay_component_present", bool(gameplay))
    gameplay.enable_adventure()
    yield delay(.5)
    st.update(world=world, pawn=pawn, pc=pc, companion=companion, ai=ai, storage=storage, gameplay=gameplay)

    # Confirmation must re-read world safety rather than trusting the snapshot that created the card.
    before_source = companion.source.get_item_count("wood")
    before_camp = storage.get_item_count("wood")
    ident = card(ai, pawn, companion, 2)
    companion.set_editor_property("source_safe", False)
    check("unsafe_change_rejects_confirmation", not ai.confirm_candidate(ident))
    check("unsafe_reason_propagated", ai.get_reason_code() == "SOURCE_NOT_TRUSTED_SAFE")
    check("unsafe_confirmation_has_no_world_effect",
          companion.get_requested() == 0 and companion.source.get_item_count("wood") == before_source
          and storage.get_item_count("wood") == before_camp)
    companion.set_editor_property("source_safe", True)
    check("same_card_revalidates_after_safety_restored", ai.confirm_candidate(ident))
    check("cancel_revalidated_goal", companion.cancel(pawn))
    yield delay(.2)

    # Pause is also authoritative at confirmation time.
    ident = card(ai, pawn, companion, 1)
    unreal.GameplayStatics.set_game_paused(world, True)
    check("pause_rejects_confirmation", not ai.confirm_candidate(ident))
    unreal.GameplayStatics.set_game_paused(world, False)
    check("unpause_revalidates_same_card", ai.confirm_candidate(ident))
    check("cancel_after_pause_case", companion.cancel(pawn))
    yield delay(.2)

    # Enter real gameplay combat after a safe card was formed; confirmation must fail closed.
    requested_before_combat = companion.get_requested()
    phase_before_combat = companion.get_phase()
    source_before_combat = companion.source.get_item_count("wood")
    camp_before_combat = storage.get_item_count("wood")
    ident = card(ai, pawn, companion, 1)
    enemy_position = companion.camp.get_actor_location() + unreal.Vector(1000, -600, 0)
    pawn.set_actor_location(enemy_position, False, True)
    yield wait(lambda: gameplay.in_combat(), 5)
    check("real_gameplay_combat_detected", gameplay.in_combat())
    check("combat_change_rejects_confirmation", not ai.confirm_candidate(ident))
    check("combat_reason_propagated", ai.get_reason_code() == "ACTIVE_COMBAT")
    check("combat_confirmation_no_goal_started",
          companion.get_requested() == requested_before_combat
          and companion.get_phase() == phase_before_combat
          and companion.source.get_item_count("wood") == source_before_combat
          and storage.get_item_count("wood") == camp_before_combat)
    pawn.set_actor_location(companion.camp.get_actor_location() + unreal.Vector(-200, 0, 0), False, True)
    yield wait(lambda: not gameplay.in_combat(), 6)
    check("leaving_combat_revalidates_same_card", ai.confirm_candidate(ident))
    check("cancel_after_combat_confirmation", companion.cancel(pawn))
    yield delay(.2)

    # Safety may change during a real five-second gather; no acquisition may settle after the change.
    source_before = companion.source.get_item_count("wood")
    camp_before = storage.get_item_count("wood")
    ident = card(ai, pawn, companion, 2)
    check("active_collect_confirmed", ai.confirm_candidate(ident))
    yield wait(lambda: companion.get_phase() == P.GATHERING, 20)
    companion.set_editor_property("source_safe", False)
    yield wait(lambda: companion.get_phase() == P.WAITING_AT_CAMP, 20)
    check("mid_gather_safety_change_blocks_settlement",
          companion.get_acquired() == 0 and companion.get_delivered() == 0
          and companion.source.get_item_count("wood") == source_before
          and storage.get_item_count("wood") == camp_before)
    check("mid_gather_reason", "SOURCE_NOT_TRUSTED_SAFE" in companion.block_reason)
    companion.set_editor_property("source_safe", True)

    # Combat that begins during a gather uses the same observation policy and returns safely.
    ident = card(ai, pawn, companion, 2)
    check("combat_mid_action_collect_confirmed", ai.confirm_candidate(ident))
    yield wait(lambda: companion.get_phase() == P.GATHERING, 20)
    source_before = companion.source.get_item_count("wood")
    camp_before = storage.get_item_count("wood")
    pawn.set_actor_location(enemy_position, False, True)
    yield wait(lambda: gameplay.in_combat(), 5)
    yield wait(lambda: companion.get_phase() == P.WAITING_AT_CAMP, 20)
    check("mid_gather_combat_blocks_settlement",
          companion.get_acquired() == 0 and companion.get_delivered() == 0
          and companion.source.get_item_count("wood") == source_before
          and storage.get_item_count("wood") == camp_before)
    check("mid_gather_combat_reason", "ACTIVE_COMBAT" in companion.block_reason)

    levels.editor_request_end_play()
    yield wait(lambda: not levels.is_in_play_in_editor(), 20)

def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    result_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()

flow = run()
pending = None
started = time.monotonic()

def tick(_dt):
    global pending
    try:
        if time.monotonic() - started > 180:
            raise TimeoutError("TASK-027 PIE timeout")
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    phase = st["companion"].get_phase() if st.get("companion") else "startup"
                    raise TimeoutError("wait expired; phase=" + str(phase))
                return
        pending = next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
