#include "Overrides.h"

#include <SimpleIni.h>

namespace Wanderer {

    static constexpr const char* kOverridesPath = "Data/SKSE/Plugins/Wanderer_Overrides.ini";

    Overrides& Overrides::GetSingleton() {
        static Overrides instance;
        return instance;
    }

    std::string Overrides::GetQuestKey(RE::TESQuest* quest) {
        if (!quest) return "";

        auto formID = quest->GetFormID();
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) return "";

        std::uint8_t modIndex = (formID >> 24) & 0xFF;

        if (modIndex == 0xFE) {
            // Light/ESL plugin
            std::uint16_t lightIndex = static_cast<std::uint16_t>((formID >> 12) & 0xFFF);
            auto* file = dataHandler->LookupLoadedLightModByIndex(lightIndex);
            if (!file) return "";
            std::uint32_t localID = formID & 0x00000FFF;
            return fmt::format("{}|0x{:03X}", file->fileName, localID);
        } else {
            // Regular plugin
            auto* file = dataHandler->LookupLoadedModByIndex(modIndex);
            if (!file) return "";
            std::uint32_t localID = formID & 0x00FFFFFF;
            return fmt::format("{}|0x{:06X}", file->fileName, localID);
        }
    }

    OverrideState Overrides::GetState(RE::TESQuest* quest) const {
        auto key = GetQuestKey(quest);
        if (key.empty()) return OverrideState::Auto;

        auto it = overrides_.find(key);
        if (it != overrides_.end()) {
            return it->second;
        }
        return OverrideState::Auto;
    }

    bool Overrides::DetectManualToggles() {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) return false;

        bool changed = false;

        // Iterate over our tracked quests and check for manual toggles.
        for (auto it = lastSetActive_.begin(); it != lastSetActive_.end(); ++it) {
            auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(it->first);
            if (!quest || !quest->IsRunning()) continue;

            bool weSetActive = it->second;
            bool isNowActive = quest->IsActive();

            if (weSetActive == isNowActive) continue;

            // The user manually toggled this quest.
            auto key = GetQuestKey(quest);
            if (key.empty()) continue;

            auto overIt = overrides_.find(key);
            OverrideState currentState = (overIt != overrides_.end())
                ? overIt->second : OverrideState::Auto;

            if (weSetActive && !isNowActive) {
                // User deactivated a quest we activated.
                if (currentState == OverrideState::Auto) {
                    overrides_[key] = OverrideState::Hidden;
                    logger::info("Wanderer: override HIDDEN — '{}'", quest->GetName());
                    RE::DebugNotification(fmt::format("Wanderer: hiding '{}'", quest->GetName()).c_str());
                    changed = true;
                } else if (currentState == OverrideState::Pinned) {
                    overrides_.erase(key);
                    logger::info("Wanderer: override REMOVED (was Pinned) — '{}'", quest->GetName());
                    RE::DebugNotification(fmt::format("Wanderer: unpinned '{}'", quest->GetName()).c_str());
                    changed = true;
                }
            } else if (!weSetActive && isNowActive) {
                // User activated a quest we didn't activate.
                if (currentState == OverrideState::Auto) {
                    overrides_[key] = OverrideState::Pinned;
                    logger::info("Wanderer: override PINNED — '{}'", quest->GetName());
                    RE::DebugNotification(fmt::format("Wanderer: pinning '{}'", quest->GetName()).c_str());
                    changed = true;
                } else if (currentState == OverrideState::Hidden) {
                    overrides_.erase(key);
                    logger::info("Wanderer: override REMOVED (was Hidden) — '{}'", quest->GetName());
                    RE::DebugNotification(fmt::format("Wanderer: unhiding '{}'", quest->GetName()).c_str());
                    changed = true;
                }
            }
        }

        if (changed) {
            Save();
        }

        return changed;
    }

    void Overrides::RecordState(RE::TESQuest* quest, bool active) {
        lastSetActive_[quest->GetFormID()] = active;
    }

    void Overrides::ClearAll() {
        int count = static_cast<int>(overrides_.size());
        overrides_.clear();
        logger::info("Wanderer: all overrides cleared");
        Save();
        if (count > 0) {
            RE::DebugNotification(fmt::format("Wanderer: cleared {} quest override(s)", count).c_str());
        } else {
            RE::DebugNotification("Wanderer: no overrides to clear");
        }
    }

    int Overrides::GetPinnedCount() const {
        int count = 0;
        for (const auto& [key, state] : overrides_) {
            if (state == OverrideState::Pinned) count++;
        }
        return count;
    }

    int Overrides::GetHiddenCount() const {
        int count = 0;
        for (const auto& [key, state] : overrides_) {
            if (state == OverrideState::Hidden) count++;
        }
        return count;
    }

    void Overrides::Load() {
        overrides_.clear();

        CSimpleIniA ini;
        ini.SetUnicode();
        if (ini.LoadFile(kOverridesPath) < 0) {
            logger::info("Wanderer: no overrides file found, starting fresh");
            return;
        }

        CSimpleIniA::TNamesDepend keys;
        ini.GetAllKeys("Overrides", keys);

        for (const auto& entry : keys) {
            const char* value = ini.GetValue("Overrides", entry.pItem, "");
            OverrideState state = OverrideState::Auto;
            if (std::string_view(value) == "Pinned") {
                state = OverrideState::Pinned;
            } else if (std::string_view(value) == "Hidden") {
                state = OverrideState::Hidden;
            } else {
                continue;  // skip unknown values
            }
            overrides_[entry.pItem] = state;
        }

        logger::info("Wanderer: loaded {} overrides ({} pinned, {} hidden)",
            overrides_.size(), GetPinnedCount(), GetHiddenCount());
    }

    void Overrides::Save() {
        CSimpleIniA ini;
        ini.SetUnicode();

        for (const auto& [key, state] : overrides_) {
            const char* value = (state == OverrideState::Pinned) ? "Pinned" : "Hidden";
            ini.SetValue("Overrides", key.c_str(), value);
        }

        if (ini.SaveFile(kOverridesPath) < 0) {
            logger::error("Wanderer: failed to save overrides to {}", kOverridesPath);
        } else {
            logger::info("Wanderer: saved {} overrides ({} pinned, {} hidden)",
                overrides_.size(), GetPinnedCount(), GetHiddenCount());
        }
    }

}
