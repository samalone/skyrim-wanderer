#pragma once

namespace Wanderer {

    struct Settings {
        bool  modEnabled          = true;
        float maxMarkerDistance   = 50000.0f;  // ~12 cells
        int   maxActiveQuests     = 3;
        int   maxActiveMarkers    = 5;
        float recheckDistance     = 4096.0f;   // Re-evaluate when player moves ~1 cell

        static Settings& GetSingleton();
    };

}
