#pragma once

namespace Wanderer {

    inline constexpr float kUnitsPerCell = 4096.0f;

    struct Settings {
        bool  modEnabled        = true;
        float maxMarkerDistance = 12.0f * kUnitsPerCell;
        int   maxActiveQuests   = 3;
        int   maxActiveMarkers  = 5;
        float recheckDistance   = 1.0f * kUnitsPerCell;

        static Settings& GetSingleton();

        /// Load settings from MCM Helper INI files.
        void Load();
    };

}
