# TASK-029 validation snapshot

Date: 2026-09-22
Base: TASK-028 compatibility follow-up `df12965`
Branch: `codex/TASK-029-contextual-suggestions`

## Implemented

- Added pure deterministic `HearthwardNPCSuggestions` generator.
- Suggestions are **ephemeral UI state**, not memory:
  - no automatic refresh;
  - explicit refresh produces at most three suggestions;
  - refresh does not write player memory, clarification, model input, filtered context or candidate state.
- Allowed global facts are separated from NPC cognition:
  - camp wood count may appear in the suggestion cache;
  - only a clicked suggestion is submitted as player input with source `quick_suggestion`.
- Every suggestion is stamped with timeline epoch + memory revision; contextual kinds also capture camp count or active command id.
- Selection revalidates authoritative state:
  - changed camp count → stale;
  - changed safety → stale collect suggestion;
  - changed timeline → stale cache;
  - changed/cancelled active command → stale progress suggestion.
- Selection never directly mutates the world and does not bypass the existing model/parser → candidate → player confirm → executor boundary.
- Modern Screen and `-HearthwardLegacyUI` dialogue surfaces share the same cached suggestion service.

## Verification

| Check | Result |
|---|---|
| UE 5.8.2 HearthwardEditor build | PASS |
| Full native `Hearthward` automation | PASS, 32/32, 0 warnings/failures |
| `Hearthward.NPCAgent.ContextualSuggestions` | PASS |
| TASK-029 modern suggestion PIE | PASS, 40/40 |
| TASK-029 legacy dialogue suggestion PIE | PASS, 8/8 |
| TASK-028 executor PIE after compatibility follow-up | PASS, 49/49 |
| TASK-025 workshop deterministic regression | PASS, 129/129 |
| Historical TASK-012 companion PIE | PASS, 45/45 |
| Repository validation / Python repo tests | PASS, 0 repo errors / 31 of 31 |

## Modern PIE coverage

`verify_suggestions_pie.py` verifies:

- cache starts empty and does not refresh in the background;
- explicit modern-Screen refresh yields exactly three suggestions;
- unselected suggestions do not become model input or filtered context;
- camp fact can use current shared storage without becoming NPC memory;
- camp-count, safety, timeline and active-command changes all stale the relevant cached suggestion;
- a clicked suggestion is tagged `quick_suggestion` and enters the normal inference boundary;
- missing GGUF fails safely with no candidate/world mutation;
- active execution prioritizes progress dialogue and suppresses replacement collection suggestions.

## Legacy UI coverage

`verify_legacy_suggestions_pie.py` is launched with `-HearthwardLegacyUI` and verifies:

- three slots initially show “尚未刷新”;
- only explicit refresh populates the shared cache;
- displayed labels exactly match the same AI-subsystem suggestion cache used by the modern Screen;
- refresh alone does not stage a candidate.

## Model limitation

The isolated DevSpace worktree does not contain the local Qwen GGUF, so a real generation is not claimed for TASK-029. This is intentional for the key boundary under test: clicking a suggestion reaches the same existing model boundary and, when that boundary is unavailable, produces **zero execution side effects**.

## Remote / workflow

TASK-027, TASK-028 main, and the TASK-028 compatibility follow-up are local commits. GitHub Issue/reviewer/push/PR are not claimed yet; user requested continued staged local development followed by one reviewable PR with separate commits.
