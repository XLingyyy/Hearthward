"""Create and bind the map-only game mode used to browse L_NaturalWorld."""

import json
import traceback
from pathlib import Path

import unreal


root = Path(unreal.Paths.project_dir())
out = root / "Saved/Task026/browse-game-mode.json"
out.parent.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_path = "/Game/Hearthward/World/Natural/BP_NaturalWorldGameMode"

try:
    blueprint = unreal.load_asset(asset_path)
    if not blueprint:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.GameModeBase)
        blueprint = asset_tools.create_asset(
            "BP_NaturalWorldGameMode",
            "/Game/Hearthward/World/Natural",
            unreal.Blueprint,
            factory,
        )
    assert blueprint
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    generated_class = blueprint.generated_class()
    assert generated_class
    defaults = unreal.get_default_object(generated_class)
    defaults.set_editor_property("default_pawn_class", unreal.HearthwardCharacter)
    defaults.set_editor_property("hud_class", unreal.HUD)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

    assert levels.load_level("/Game/Hearthward/World/Natural/L_NaturalWorld")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings = world.get_world_settings()
    settings.set_editor_property("default_game_mode", generated_class)
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    result = {
        "ok": True,
        "asset": asset_path,
        "parent": "GameModeBase",
        "default_pawn": "HearthwardCharacter",
        "hud": "HUD (no Hearthward title/menu UI)",
        "map": "/Game/Hearthward/World/Natural/L_NaturalWorld",
    }
except Exception:
    result = {"ok": False, "error": traceback.format_exc()}

out.write_text(json.dumps(result, indent=2), encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
