#pragma once

#include "Settings.h"
#include <unordered_map>
#include <string>

namespace Wanderer {

    enum class OverrideState : std::uint8_t {
        Auto,    // default — Wanderer manages this quest
        Pinned,  // user wants this quest always active
        Hidden   // user wants this quest never auto-activated
    };

    class Overrides {
    public:
        static Overrides& GetSingleton();

        /// Get the override state for a quest.
        OverrideState GetState(RE::TESQuest* quest) const;

        /// Detect manual toggles by comparing actual active flags against
        /// what Wanderer last set. Returns true if any overrides changed.
        bool DetectManualToggles();

        /// Record what Wanderer set a quest's active state to.
        void RecordState(RE::TESQuest* quest, bool active);

        /// Clear all overrides (MCM reset button).
        void ClearAll();

        /// Clear the runtime tracking state (call on game load/new game).
        void ResetTracking();

        /// Load/save overrides from/to INI file.
        void Load();
        void Save();

        int GetPinnedCount() const;
        int GetHiddenCount() const;

        /// Build a stable key from a quest: "PluginName.esp|0x00001234"
        static std::string GetQuestKey(RE::TESQuest* quest);

    private:
        Overrides() = default;

        /// Resolve a quest key back to a FormID (returns 0 if not found).
        static RE::FormID ResolveKey(const std::string& key);

        /// Map from stable quest key → override state (persisted to INI).
        std::unordered_map<std::string, OverrideState> overrides_;

        /// Map from runtime FormID → active state Wanderer last set (not persisted).
        std::unordered_map<RE::FormID, bool> lastSetActive_;
    };

}
