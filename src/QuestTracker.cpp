#include "QuestTracker.h"

namespace Wanderer {

    QuestTracker& QuestTracker::GetSingleton() {
        static QuestTracker instance;
        return instance;
    }

    void QuestTracker::Enable() {
        enabled_ = true;
        lastEvalPos_ = {0.0f, 0.0f, 0.0f};
        logger::info("Wanderer: tracker enabled");
    }

    void QuestTracker::Disable() {
        enabled_ = false;
        logger::info("Wanderer: tracker disabled");
    }

    void QuestTracker::OnUpdate() {
        if (!enabled_) {
            return;
        }

        const auto& settings = Settings::GetSingleton();
        if (!settings.modEnabled) {
            return;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        auto playerPos = player->GetPosition();
        float movedSq = playerPos.GetSquaredDistance(lastEvalPos_);
        float thresholdSq = settings.recheckDistance * settings.recheckDistance;

        if (movedSq >= thresholdSq) {
            Evaluate();
            lastEvalPos_ = playerPos;
        }
    }

    void QuestTracker::Evaluate() {
        auto startTime = std::chrono::high_resolution_clock::now();

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        const auto& settings = Settings::GetSingleton();
        auto playerPos = player->GetPosition();

        // Gather all running quests with distance info.
        auto quests = GatherQuests(playerPos);

        // Sort by nearest objective distance (ascending).
        std::sort(quests.begin(), quests.end(), [](const QuestInfo& a, const QuestInfo& b) {
            return a.nearestDist < b.nearestDist;
        });

        // Count quests in range.
        int questsInRange = 0;
        for (const auto& qi : quests) {
            if (qi.nearestDist <= settings.maxMarkerDistance) {
                questsInRange++;
            }
        }

        // Activate quests greedily until we hit the limits.
        int activeQuests  = 0;
        int activeMarkers = 0;

        for (auto& qi : quests) {
            bool shouldActivate = false;

            if (qi.nearestDist <= settings.maxMarkerDistance &&
                activeQuests < settings.maxActiveQuests &&
                (activeMarkers + qi.markerCount) <= settings.maxActiveMarkers) {

                shouldActivate = true;
                activeQuests++;
                activeMarkers += qi.markerCount;
            }

            bool isCurrentlyActive = qi.quest->IsActive();

            if (shouldActivate && !isCurrentlyActive) {
                qi.quest->data.flags.set(RE::QuestFlag::kActive);
                logger::trace("Wanderer: activating quest '{}' (dist: {:.0f})",
                    qi.quest->GetName(), qi.nearestDist);
            } else if (!shouldActivate && isCurrentlyActive) {
                qi.quest->data.flags.reset(RE::QuestFlag::kActive);
                logger::trace("Wanderer: deactivating quest '{}' (dist: {:.0f})",
                    qi.quest->GetName(), qi.nearestDist);
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        logger::info("Wanderer: evaluated {} quests, {} in range, {} activated ({} markers), {:.2f}ms",
            quests.size(), questsInRange, activeQuests, activeMarkers, elapsedMs);
    }

    std::vector<QuestInfo> QuestTracker::GatherQuests(RE::NiPoint3 playerPos) {
        std::vector<QuestInfo> result;

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return result;
        }

        for (auto* quest : dataHandler->GetFormArray<RE::TESQuest>()) {
            if (!quest || !quest->IsRunning()) {
                continue;
            }

            // Skip quests with no displayed objectives.
            int markers = CountDisplayedObjectives(quest);
            if (markers == 0) {
                continue;
            }

            float dist = GetNearestTargetDistance(quest, playerPos);

            QuestInfo qi;
            qi.quest       = quest;
            qi.nearestDist = dist;
            qi.markerCount = markers;
            qi.wasActiveBeforeWanderer = quest->IsActive();
            result.push_back(qi);
        }

        return result;
    }

    int QuestTracker::CountDisplayedObjectives(RE::TESQuest* quest) {
        int count = 0;
        for (const auto& obj : quest->objectives) {
            if (!obj) {
                continue;
            }
            auto state = obj->state.get();
            if (state == RE::QUEST_OBJECTIVE_STATE::kDisplayed ||
                state == RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed) {
                count++;
            }
        }
        return count;
    }

    float QuestTracker::GetNearestTargetDistance(RE::TESQuest* quest, RE::NiPoint3 playerPos) {
        float nearest = std::numeric_limits<float>::max();

        for (const auto& obj : quest->objectives) {
            if (!obj) {
                continue;
            }

            auto state = obj->state.get();
            if (state != RE::QUEST_OBJECTIVE_STATE::kDisplayed &&
                state != RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed) {
                continue;
            }

            for (std::uint32_t i = 0; i < obj->numTargets; i++) {
                auto* target = obj->targets[i];
                if (!target) {
                    continue;
                }

                // Resolve the target's alias to an ObjectReference via refAliasMap.
                auto aliasID = static_cast<std::uint32_t>(target->alias);

                RE::ObjectRefHandle refHandle;
                quest->CreateRefHandleByAliasID(refHandle, aliasID);

                if (!refHandle) {
                    continue;
                }

                auto refPtr = refHandle.get();
                if (!refPtr) {
                    continue;
                }

                float dist = playerPos.GetDistance(refPtr->GetPosition());
                if (dist < nearest) {
                    nearest = dist;
                }
            }
        }

        return nearest;
    }

}
