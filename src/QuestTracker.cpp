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

    void QuestTracker::ForceEvaluate() {
        if (enabled_) {
            Evaluate();
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (player) {
                lastEvalPos_ = player->GetPosition();
            }
        }
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

        logger::info("Wanderer: --- evaluation start (maxQuests={}, maxMarkers={}, maxDist={:.0f}) ---",
            settings.maxActiveQuests, settings.maxActiveMarkers, settings.maxMarkerDistance);

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

            // Log every quest we considered
            const char* action = "skip";
            if (shouldActivate && !isCurrentlyActive) {
                qi.quest->data.flags.set(RE::QuestFlag::kActive);
                action = "ACTIVATE";
            } else if (shouldActivate && isCurrentlyActive) {
                action = "keep";
            } else if (!shouldActivate && isCurrentlyActive) {
                qi.quest->data.flags.reset(RE::QuestFlag::kActive);
                action = "DEACTIVATE";
            }

            float distCells = qi.nearestDist / kUnitsPerCell;
            logger::info("  {:>10s}  dist={:7.1f} ({:5.1f} cells)  markers={}  '{}'",
                action, qi.nearestDist, distCells, qi.markerCount, qi.quest->GetName());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        logger::info("Wanderer: --- evaluation done: {} quests, {} in range, {} activated ({} markers), {:.2f}ms ---",
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

            // Compute nearest target distance and count resolvable map targets.
            int targetCount = 0;
            float dist = GetNearestTargetDistance(quest, playerPos, targetCount);
            if (targetCount == 0) {
                continue;
            }

            QuestInfo qi;
            qi.quest       = quest;
            qi.nearestDist = dist;
            qi.markerCount = targetCount;
            qi.wasActiveBeforeWanderer = quest->IsActive();
            result.push_back(qi);
        }

        return result;
    }

    float QuestTracker::GetNearestTargetDistance(RE::TESQuest* quest, RE::NiPoint3 playerPos, int& outTargetCount) {
        float nearest = std::numeric_limits<float>::max();
        outTargetCount = 0;

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

                // Check target conditions — the game uses these to control
                // whether a marker is actually visible on the map/compass.
                if (target->conditions.head) {
                    auto* player = RE::PlayerCharacter::GetSingleton();
                    if (!target->conditions.IsTrue(player, refPtr.get())) {
                        continue;
                    }
                }

                outTargetCount++;

                float dist = playerPos.GetDistance(refPtr->GetPosition());
                if (dist < nearest) {
                    nearest = dist;
                }
            }
        }

        return nearest;
    }

}
