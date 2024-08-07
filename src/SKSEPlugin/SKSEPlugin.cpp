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

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(273);

	Hooks::Misc::Install();
	Hooks::Enchanting::Install();
	Hooks::FilterFlags::Install();
	Hooks::Disenchant::Install();
	Hooks::Gameplay::Install();
	Hooks::VFX::Install();
	Hooks::SkyUI::Install();

	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::ID);
	serialization->SetSaveCallback(&Serialization::SaveCallback);
	serialization->SetLoadCallback(&Serialization::LoadCallback);
	serialization->SetRevertCallback(&Serialization::RevertCallback);

	const auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(&Papyrus::AmmoEnchanting::RegisterFuncs);

	Settings::INISettings::GetSingleton()->LoadSettings();

	return true;
}
