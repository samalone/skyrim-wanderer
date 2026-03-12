#include "Settings.h"

#include <SimpleIni.h>

namespace Wanderer {

    Settings& Settings::GetSingleton() {
        static Settings instance;
        return instance;
    }

    void Settings::Load() {
        CSimpleIniA ini;
        ini.SetUnicode();

        // Try user settings first (written by MCM Helper when user changes values).
        SI_Error rc = ini.LoadFile("Data/MCM/Settings/Wanderer.ini");
        if (rc < 0) {
            // Fall back to shipped defaults.
            rc = ini.LoadFile("Data/MCM/Config/Wanderer/settings.ini");
        }

        if (rc < 0) {
            logger::warn("Wanderer: no settings INI found, using defaults");
            return;
        }

        modEnabled       = ini.GetBoolValue("General", "bModEnabled", true);
        maxActiveQuests  = static_cast<int>(ini.GetLongValue("Tracking", "iMaxActiveQuests", 3));
        maxActiveMarkers = static_cast<int>(ini.GetLongValue("Tracking", "iMaxActiveMarkers", 5));

        // MCM stores distances in cells; convert to game units.
        float distCells    = static_cast<float>(ini.GetDoubleValue("Tracking", "fMaxMarkerDistance", 12.0));
        float recheckCells = static_cast<float>(ini.GetDoubleValue("Tracking", "fRecheckDistance", 1.0));
        maxMarkerDistance  = distCells * kUnitsPerCell;
        recheckDistance    = recheckCells * kUnitsPerCell;

        logger::info("Wanderer: settings loaded — enabled={}, maxQuests={}, maxMarkers={}, "
                     "maxDist={:.0f} ({:.0f} cells), recheck={:.0f} ({:.1f} cells)",
                     modEnabled, maxActiveQuests, maxActiveMarkers,
                     maxMarkerDistance, distCells, recheckDistance, recheckCells);
    }

}
