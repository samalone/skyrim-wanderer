# Wanderer - Skyrim SE Quest Proximity Mod

## Concept
Automatically tracks the nearest quest objectives as the player explores Skyrim. When the player moves a configurable distance, the mod recomputes distances to all quest markers, activates the closest quests, and deactivates quests whose markers are too far away. No UI — it just works in the background.

## Technology
This is a **native SKSE plugin** (CommonLibSSE-NG "NG" mod), not a Papyrus script mod. The plugin compiles to a DLL loaded by SKSE64, giving us direct access to game internals at native speed.

## Dependencies
- **Skyrim Special Edition** (installed at `G:\Steam\steamapps\common\Skyrim Special Edition`)
- **SKSE64** (installed)
- **Address Library for SKSE Plugins** (for version-independent function addresses)

## Architecture
- **Trigger**: Player movement distance — hooks `PlayerCharacter::Update()` vtable, checks squared distance from last evaluation point each frame (cheap). Full re-evaluation only when player has moved beyond `recheckDistance` threshold.
- **Core loop**: Enumerate all `RE::TESQuest` forms → filter to running quests with displayed objectives → resolve objective targets to world positions via `CreateRefHandleByAliasID` → compute distances → sort by nearest → activate top N quests within limits.
- **Quest activation**: Toggle `RE::QuestFlag::kActive` flag directly on `quest->data.flags`.
- **Distance units**: 4096 game units ≈ 1 overworld cell.

## Settings (hardcoded defaults, will be user-configurable later)
- `modEnabled` — is the mod active? (default: true)
- `maxMarkerDistance` — maximum distance to nearest marker for a quest to be eligible (default: 50000 units, ~12 cells)
- `maxActiveQuests` — max number of quests to track simultaneously (default: 3)
- `maxActiveMarkers` — max total displayed objective markers across all active quests (default: 5)
- `recheckDistance` — how far the player must move before re-evaluating (default: 4096 units, ~1 cell)

## Development Tools
- **Visual Studio 2026 Professional** — C++ compiler (MSVC 14.50)
- **CMake + Ninja** — build system (bundled with VS)
- **vcpkg** — package manager (installed at `C:\vcpkg`, `VCPKG_ROOT` must be set)
- **CommonLibSSE-NG 3.7.0** — reverse-engineered Skyrim headers (via vcpkg, colorglass registry)
- **SSEEdit** — plugin inspection (optional)

## Project Structure
```
wanderer/
├── CLAUDE.md
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── vcpkg-configuration.json
├── .gitignore
├── src/
│   ├── PCH.h               # Precompiled header (RE/Skyrim.h, SKSE/SKSE.h)
│   ├── Main.cpp             # Plugin entry point, update hook, logging
│   ├── Settings.h/.cpp      # Configuration values
│   └── QuestTracker.h/.cpp  # Core quest proximity logic
└── build/                   # Build output (gitignored)
    └── Wanderer.dll         # The SKSE plugin
```

## Build
```bash
# Requires VCPKG_ROOT=C:\vcpkg and MSVC x64 environment (vcvarsall.bat x64)
cmake --preset default
cmake --build build
```
Output: `build/Wanderer.dll`

## Install
Copy `Wanderer.dll` to `<Skyrim SE>/Data/SKSE/Plugins/Wanderer.dll`

## Key CommonLibSSE-NG APIs Used
- `RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESQuest>()` — enumerate all quests
- `RE::TESQuest::IsRunning()`, `IsActive()` — quest state checks
- `RE::QuestFlag::kActive` — flag to set/reset for tracking
- `RE::BGSQuestObjective` / `RE::QUEST_OBJECTIVE_STATE` — objective state inspection
- `RE::TESQuestTarget` — objective target with alias ID
- `RE::TESQuest::CreateRefHandleByAliasID()` — resolve alias to ObjectRefHandle
- `RE::NiPoint3::GetDistance()` / `GetSquaredDistance()` — distance calculations
- `RE::PlayerCharacter::GetSingleton()` — player reference
- `RE::VTABLE_PlayerCharacter` — vtable for hooking Update()

## Open Questions
- Should the mod remember which quests the player manually selected and not override those?
- Should there be multiple distance tiers (e.g., "nearby" vs "in region")?
- Should the mod work only in the overworld, or also in dungeons/interiors?
- Future: MCM menu for user configuration (requires SkyUI dependency)
