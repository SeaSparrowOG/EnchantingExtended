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

	void HandleSoulGemFuel()
	{
		auto* fuelKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("STEN_StaffFuel"sv);
		if (!fuelKeyword) {
			_loggerError(
				"Warning: Failed to find STEN_StaffFuel. Ensure that the plugin is loaded.");
			return;
		}

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		auto& soulGemArray = dataHandler->GetFormArray<RE::TESSoulGem>();
		for (auto* soulGem : soulGemArray) {
			soulGem->AddKeyword(fuelKeyword);
		}
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		if (Settings::INISettings::GetSingleton()->bAdjustStaffEnchanters) {
			HandleEnchantingTables();
		}
		if (Settings::INISettings::GetSingleton()->bUseSoulGemsForStaves) {
			HandleSoulGemFuel();
		}
		Staves::StaffEnchantManager::GetSingleton()->RegisterListener();
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
	SKSE::AllocTrampoline(301);

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
	
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);
	_loggerInfo("Finished startup tasks.");
	_loggerInfo("-------------------------------------------------------------------------------------");
	return true;
}
