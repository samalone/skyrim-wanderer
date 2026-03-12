#include "Settings.h"

namespace Wanderer {

    Settings& Settings::GetSingleton() {
        static Settings instance;
        return instance;
    }

}
