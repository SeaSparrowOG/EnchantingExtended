#include "Events/ActivationListener.h"

namespace
{
	RE::TESBoundObject* ParseForm(const std::string& a_identifier)
	{
		if (const auto splitID = clib_util::string::split(a_identifier, "|");
			splitID.size() == 2) {
			if (!clib_util::string::is_only_hex(splitID[1]))
				return nullptr;
			const auto formID = clib_util::string::to_num<RE::FormID>(splitID[1], true);

			const auto& modName = splitID[0];
			if (!RE::TESDataHandler::GetSingleton()->LookupModByName(modName))
				return nullptr;

			auto* baseForm = RE::TESDataHandler::GetSingleton()->LookupForm(formID, modName);
			return baseForm ? static_cast<RE::TESBoundObject*>(baseForm) : nullptr;
		}
		auto* form = RE::TESBoundObject::LookupByEditorID(a_identifier);
		if (form)
			return static_cast<RE::TESBoundObject*>(form);
		return nullptr;
	}
}

namespace ActivationListener
{
	bool EnchantingTable::RegisterListener()
	{
		auto* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!eventHolder)
			return false;

		eventHolder->AddEventSink(this);
		return ReadSettings();
	}

	bool EnchantingTable::IsInValidStaffWorkbench()
	{
		return isInValidStaffWorkbench;
	}

	bool EnchantingTable::IsInValidAmmoWorkbench()
	{
		return isInValidAmmoWorkbench;
	}

	RE::BSEventNotifyControl EnchantingTable::ProcessEvent(
		const RE::TESFurnitureEvent* a_event,
		RE::BSTEventSource<RE::TESFurnitureEvent>*)
	{
		if (!a_event || !a_event->targetFurniture.get())
			continueEvent;
		auto* eventActor = a_event->actor.get();
		bool isPlayer = eventActor ? eventActor->IsPlayerRef() : false;
		if (!isPlayer)
			continueEvent;

		auto* eventObject = a_event->targetFurniture.get();
		auto* eventBase = eventObject->GetBaseObject();
		auto* eventFurniture = eventBase ? eventBase->As<RE::TESFurniture>() : nullptr;
		if (!eventFurniture)
			continueEvent;

		bool isEnchantingTable = eventFurniture->workBenchData.benchType.any(
			RE::TESFurniture::WorkBenchData::BenchType::kEnchanting);
		if (!isEnchantingTable)
			continueEvent;

		bool isEntering = a_event->type.underlying() == 0;
		if (eventFurniture->HasKeywordString("DLC2StaffEnchanter"sv)) {
			isInValidStaffWorkbench = isEntering;
		}
		continueEvent;
	}

	bool EnchantingTable::ReadSettings()
	{
		std::vector<std::string> configPaths = std::vector<std::string>();
		try {
			configPaths = clib_util::distribution::get_configs(
				R"(Data\SKSE\Plugins\EnchantingExtended\)",
				"",
				".json"sv);
		}
		catch (std::exception e) {
			_loggerError("Caught error {} while trying to fetch fire config files.", e.what());
			return false;
		}
		if (configPaths.empty())
			return true;

		for (auto& config : configPaths) {
			std::ifstream rawJSON(config);
			Json::Reader JSONReader;
			Json::Value JSONFile;
			JSONReader.parse(rawJSON, JSONFile);

			if (!JSONFile.isObject()) {
				_loggerInfo("Warning: {} is not an object.", config);
				continue;
			}

			auto members = JSONFile.getMemberNames();
			for (auto& identifier : members) {
				auto& member = JSONFile[identifier];
				if (!member || !member.isString()) {
					_loggerInfo(
						"Warning: caught bad member for {}:\n     >{}",
						config,
						identifier);
					continue;
				}

				auto* key = ParseForm(identifier);
				auto* value = ParseForm(member.asString());
				if (!(key && value)) {
					_loggerInfo(
						"Couldn't resolve a form in {}:\n    >Name: {}\n    >Key: {}\n    >Value: {}",
						config,
						identifier,
						key ? "KEY MISSING" : _debugEDID(key),
						value ? "VALUE MISSING" : _debugEDID(value));
					continue;
				}

				auto* keySpell = key->As<RE::SpellItem>();
				auto* valueSpell = value->As<RE::EnchantmentItem>();
				if (!(keySpell && valueSpell)) {
					_loggerInfo(
						"Forms in {} exist, but are of unexpected type:\n    >Name: {}\n    >Key: {}\n    >Value: {}",
						config,
						identifier,
						keySpell ? "KEY MISSING" : _debugEDID(keySpell),
						valueSpell ? "VALUE MISSING" : _debugEDID(valueSpell));
					continue;
				}

				spellEnchantments[keySpell] = valueSpell;
			}
		}
		return true;
	}
}
