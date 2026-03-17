#include "QuestTracker.h"
#include "Overrides.h"

using namespace Wanderer;

/// Native Papyrus function: called by WandererMCM.psc when a setting changes.
void ReloadSettings(RE::StaticFunctionTag*) {
    Settings::GetSingleton().Load();
    QuestTracker::GetSingleton().ForceEvaluate();
}

/// Native Papyrus function: clear all quest overrides.
void ClearOverrides(RE::StaticFunctionTag*) {
    Overrides::GetSingleton().ClearAll();
    QuestTracker::GetSingleton().ForceEvaluate();
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("ReloadSettings", "WandererMCM", ReloadSettings);
    vm->RegisterFunction("ClearOverrides", "WandererMCM", ClearOverrides);
    logger::info("Wanderer: registered Papyrus native functions");
    return true;
}

/// Hook into the player update loop to check movement distance.
class PlayerUpdateHook {
public:
    static void Install() {
        // Hook PlayerCharacter::Update to run our check each frame.
        // We use a lightweight squared-distance check so the per-frame cost is trivial;
        // the full quest evaluation only runs when the player has moved far enough.
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_PlayerCharacter[0]};
        originalUpdate_ = vtbl.write_vfunc(0xAD, UpdateHook);
        logger::info("Wanderer: player update hook installed");
    }

private:
    static void UpdateHook(RE::PlayerCharacter* player, float delta) {
        originalUpdate_(player, delta);
        QuestTracker::GetSingleton().OnUpdate();
    }

    inline static REL::Relocation<decltype(&UpdateHook)> originalUpdate_;
};

void InitializeLogging() {
    auto path = logger::log_directory();
    if (!path) {
        return;
    }
    *path /= "Wanderer.log"sv;
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);
    spdlog::set_default_logger(std::move(log));
    logger::info("Wanderer v{}.{}.{} loaded", 0, 1, 0);
}

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        Settings::GetSingleton().Load();
        Overrides::GetSingleton().Load();
        PlayerUpdateHook::Install();
        QuestTracker::GetSingleton().Enable();

        auto* papyrus = SKSE::GetPapyrusInterface();
        papyrus->Register(RegisterPapyrusFunctions);
    }
    if (message->type == SKSE::MessagingInterface::kPostLoadGame ||
        message->type == SKSE::MessagingInterface::kNewGame) {
        Settings::GetSingleton().Load();
        Overrides::GetSingleton().Load();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    InitializeLogging();

    auto* messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(OnMessage);

    logger::info("Wanderer: plugin registered");
    return true;
}
