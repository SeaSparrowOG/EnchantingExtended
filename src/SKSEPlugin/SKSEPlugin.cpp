#include "Events/ActivationListener.h"
#include "Hooks/Disenchant.h"
#include "Hooks/Enchanting.h"
#include "Hooks/FilterFlags.h"
#include "Hooks/Gameplay.h"
#include "Hooks/Misc.h"
#include "Hooks/SkyUI.h"
#include "Hooks/VFX.h"
#include "Papyrus/AmmoEnchanting.h"
#include "Serialization/Serialization.h"
#include "Settings/INISettings.h"

namespace
{
	void SetupLog() {
		auto logsFolder = SKSE::log::log_directory();
		if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");

		auto pluginName = Version::NAME;
		auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
		auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
		auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));

		spdlog::set_default_logger(std::move(loggerPtr));
#ifdef DEBUG
		spdlog::set_level(spdlog::level::debug);
		spdlog::flush_on(spdlog::level::debug);
#else
		spdlog::set_level(spdlog::level::info);
		spdlog::flush_on(spdlog::level::info);
#endif

		//Pattern
		spdlog::set_pattern("%v");
	}

	void HandleEnchantingTables()
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		auto& furnitureArray = dataHandler->GetFormArray<RE::TESFurniture>();
		for (auto* furniture : furnitureArray) {
			if (!furniture->workBenchData.usesSkill.any(RE::ActorValue::kEnchanting)) {
				continue;
			}
			if (!furniture->HasKeywordString("DLC2StaffEnchanter"sv)) {
				continue;
			}

			furniture->workBenchData
				.benchType = RE::TESFurniture::WorkBenchData::BenchType::kEnchanting;
			auto name = _debugEDID(furniture);
			if (!name.empty()) {
				_loggerInfo("{} is now a proper staff enchanter.", name);
			}
		}
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPreLoadGame:
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		HandleEnchantingTables();
		break;
	default:
		break;
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion({ Version::MAJOR, Version::MINOR, Version::PATCH });
	v.PluginName(Version::NAME);
	v.AuthorName(Version::PROJECT_AUTHOR);
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });
	return v;
	}();


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SetupLog();
	_loggerInfo("Starting up {}.", Version::NAME);
	_loggerInfo("Plugin Version: {}.", Version::VERSION);
	_loggerInfo("-------------------------------------------------------------------------------------");
	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(273);

	_loggerInfo("Reading settings...");
	Settings::INISettings::GetSingleton()->LoadSettings();

	_loggerInfo("Installing misc hooks...");
	Hooks::Misc::Install();
	_loggerInfo("Installing enchanting hooks...");
	Hooks::Enchanting::Install();
	_loggerInfo("Installing filter flag hooks...");
	Hooks::FilterFlags::Install();
	_loggerInfo("Installing disenchanting hooks...");
	Hooks::Disenchant::Install();
	_loggerInfo("Installing gameplay hooks....");
	Hooks::Gameplay::Install();
	_loggerInfo("Installing VFX hooks...");
	Hooks::VFX::Install();
	_loggerInfo("Installing SkyUI hooks...");
	Hooks::SkyUI::Install();
	
	_loggerInfo("Preparing serialiaztion callbacks...");
	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::ID);
	serialization->SetSaveCallback(&Serialization::SaveCallback);
	serialization->SetLoadCallback(&Serialization::LoadCallback);
	serialization->SetRevertCallback(&Serialization::RevertCallback);
	
	_loggerInfo("Registering new papyrus functions...");
	const auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(&Papyrus::AmmoEnchanting::RegisterFuncs);
	
	_loggerInfo("Modifying staff enchanters...");
	ActivationListener::EnchantingTable::GetSingleton()->RegisterListener();
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);
	_loggerInfo("Finished startup tasks.");
	_loggerInfo("-------------------------------------------------------------------------------------");
	return true;
}
