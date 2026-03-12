#pragma once

#include "Settings.h"

namespace Wanderer {

    /// Information about a single quest and its nearest objective target.
    struct QuestInfo {
        RE::TESQuest* quest          = nullptr;
        float         nearestDist    = std::numeric_limits<float>::max();
        int           markerCount    = 0;   // number of displayed objectives
        bool          wasActiveBeforeWanderer = false;
    };

    /// Core tracker that re-evaluates quest proximity when the player moves.
    class QuestTracker {
    public:
        static QuestTracker& GetSingleton();

        void Enable();
        void Disable();

        /// Called from the game-thread update hook. Checks whether the player
        /// has moved far enough to justify a full re-evaluation.
        void OnUpdate();

    private:
        QuestTracker() = default;

        /// Full re-evaluation: enumerate quests, compute distances, activate the nearest.
        void Evaluate();

        /// Gather all running quests and compute the distance to their nearest
        /// displayed objective target.
        std::vector<QuestInfo> GatherQuests(RE::NiPoint3 playerPos);

        /// Count the displayed objectives for a quest.
        static int CountDisplayedObjectives(RE::TESQuest* quest);

        /// Get the nearest objective target distance for a quest.
        static float GetNearestTargetDistance(RE::TESQuest* quest, RE::NiPoint3 playerPos);

        RE::NiPoint3 lastEvalPos_{0.0f, 0.0f, 0.0f};
        bool          enabled_ = false;
    };

}
