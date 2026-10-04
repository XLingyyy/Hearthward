"""Audit the uncommitted UI prototype against its explicit local task scope."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

GAME = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
BASE = "4db5789184fe38e041d62a1e68c8517338ea0b01"
task = json.loads((GAME / "docs/tasks/TASK-053.json").read_text(encoding="utf-8"))
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--run-id", required=True, help="The completed verification run to bind this audit to")
args = parser.parse_args()


def git(*args):
    return subprocess.check_output(["git", *args], cwd=GAME).decode("utf-8")


def matches(path, scope):
    return path.startswith(scope) if scope.endswith("/") else path == scope


changes = [row[3:] for row in git("status", "--porcelain=v1", "-z", "--untracked-files=all").split("\0") if row]
outside = [p for p in changes if not any(matches(p, scope) for scope in task["allowed_paths"])]
forbidden = [p for p in changes if any(matches(p, scope) for scope in task["forbidden_paths"])]
config_checks = {}
for filename in ("Resources/UI/interface.json", "Resources/UI/layout.json"):
    before = json.loads(git("show", BASE + ":" + filename))
    after = json.loads((GAME / filename).read_text(encoding="utf-8"))
    for page in ("title", "settings", "save", "hud", "inventory", "skills"):
        del before["pages"][page]
        del after["pages"][page]
    if filename.endswith("interface.json"):
        title_assets = {"titleGlyphGui", "titleGlyphGuiTail", "titleGlyphHuo", "titleGlyphHuoTail",
                        "titleGlyphFlame", "titleWordmarkEnglish"}
        added_assets = set(after["assets"]) - set(before["assets"])
        config_checks["title UV aliases reuse unchanged logo artwork"] = added_assets == title_assets and all(
            after["assets"][name]["file"] == before["assets"]["logo"]["file"] for name in added_assets)
        for name in title_assets:
            after["assets"].pop(name, None)
    config_checks[filename + ": other pages unchanged"] = before == after
content_file = "Source/Hearthward/UI/HearthwardScreenContent.cpp"
split = "void UHearthwardScreenWidget::ComposeSave()"
def outside_hud_save_inventory_skills(text):
    prefix = text.split(split)[0]
    inv_start = prefix.index("void UHearthwardScreenWidget::ComposeInventory(bool Storage)")
    inv_end = prefix.index("void UHearthwardScreenWidget::ComposeMap()", inv_start)
    prefix = prefix[:inv_start] + prefix[inv_end:]
    start = prefix.index("void UHearthwardScreenWidget::ComposeHUD()")
    end = prefix.index("void UHearthwardScreenWidget::ComposeBuilding()", start)
    return prefix[:start] + prefix[end:]

config_checks["composition outside HUD/save/inventory/skills unchanged"] = outside_hud_save_inventory_skills(git("show", BASE+":"+content_file)) == outside_hud_save_inventory_skills((GAME/content_file).read_text(encoding="utf-8"))
inventory_before = (OUT / "inventory-preview/before" / content_file).read_text(encoding="utf-8")
inventory_current = (GAME / content_file).read_text(encoding="utf-8")
def legacy_inventory(text):
    return text.split("void UHearthwardScreenWidget::ComposeInventory(bool Storage)", 1)[1].split("void UHearthwardScreenWidget::ComposeSkills()", 1)[0]
config_checks["legacy storage composition unchanged"] = legacy_inventory(inventory_before) == legacy_inventory(inventory_current).replace("    if(!Storage) { ComposeInventoryScreen(); return; }\n", "", 1)
menu_file = "Source/Hearthward/UI/HearthwardScreenMenu.cpp"
config_checks["previous inventory/menu composition unchanged"] = (OUT / "skills-preview/before" / menu_file).read_bytes() == (GAME / menu_file).read_bytes()
for filename in ("Resources/Data/gameplay.json", "Source/Hearthward/Gameplay/HearthwardGameplayComponent.cpp",
                 "Source/Hearthward/Gameplay/HearthwardProgression.cpp", "Source/Hearthward/Gameplay/HearthwardProgression.h"):
    config_checks[filename + ": original gameplay rules unchanged"] = git("show", BASE + ":" + filename) == (GAME / filename).read_text(encoding="utf-8")
verification = json.loads((OUT / args.run_id / "report.json").read_text(encoding="utf-8"))
fingerprints = json.loads((OUT / args.run_id / "fingerprints.json").read_text(encoding="utf-8"))
fingerprint_checks = {filename: hashlib.sha256((GAME / filename).read_bytes()).hexdigest() == expected
                      for filename, expected in fingerprints.items()}
report = {
    "base": BASE,
    "head": git("rev-parse", "HEAD").strip(),
    "branch": git("branch", "--show-current").strip(),
    "verified_run": args.run_id,
    "changed_paths": changes,
    "outside_allowed_paths": outside,
    "forbidden_paths": forbidden,
    "config_checks": config_checks,
    "final_test_fingerprints_match": fingerprint_checks,
    "passed": verification["passed"] and not outside and not forbidden and all(config_checks.values()) and all(fingerprint_checks.values()),
    "limitation": "This is a local audit of the user-requested scope, not the baseline task-snapshot check; TASK-053 does not exist at the baseline and remains uncommitted."
}
(OUT / "scope-audit.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
print(json.dumps({"passed": report["passed"], "outside": outside, "forbidden": forbidden,
                  "configs": config_checks, "fingerprints": len(fingerprint_checks)}, ensure_ascii=False))
raise SystemExit(0 if report["passed"] else 1)
