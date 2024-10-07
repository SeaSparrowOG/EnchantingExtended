#include "Events/ActivationListener.h"

namespace
{
	RE::TESBoundObject* ParseForm(const std::string& a_identifier)
	{
		if (const auto splitID = clib_util::string::split(a_identifier, "|");
			splitID.size() == 2) {
			if (!clib_util::string::is_only_hex(splitID[0]))
				return nullptr;
			const auto formID = clib_util::string::to_num<RE::FormID>(splitID[0], true);

			const auto& modName = splitID[1];
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

	Enchantment EnchantingTable::GetEnchantmentInfo(
		const std::vector<RE::EnchantmentItem*>& a_enchantments)
	{
		auto response = Enchantment();

		for (auto* enchantment : a_enchantments) {
			if (!enchantment) {
				response.enchantment = enchantment;
			}

			for (auto& pair : this->spellEnchantments) {
				if (pair.second.enchantment != enchantment)
					continue;
				if (pair.second.chargeTime > response.chargeTime) {
					response.chargeTime = pair.second.chargeTime;
				}
				if (pair.second.charges > response.charges) {
					response.charges = pair.second.charges;
				}
				response.cost += pair.second.cost;
				break;
			}
		}
		return response;
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

		if (!isEntering)
			continueEvent;

		auto* playerREF = RE::PlayerCharacter::GetSingleton();
		for (auto& pair : spellEnchantments) {
			auto* spell = pair.first;
			auto* enchant = pair.second.enchantment;

			if (!playerREF->HasSpell(spell)) {
				enchant->formFlags &= ~RE::TESForm::RecordFlags::kKnown;
				enchant->AddChange(RE::TESForm::ChangeFlags::kFlags);
				continueEvent;
			}

			enchant->effects = spell->effects;
			enchant->data.spellType = RE::MagicSystem::SpellType::kStaffEnchantment;
			enchant->SetDelivery(spell->GetDelivery());
			enchant->SetCastingType(spell->GetCastingType());
			enchant->fullName = spell->fullName;

			enchant->formFlags |= RE::TESForm::RecordFlags::kKnown;
			enchant->AddChange(RE::TESForm::ChangeFlags::kFlags);
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

			if (!JSONFile.isArray()) {
				_loggerInfo("Warning: {} is not an array.", config);
				continue;
			}

			for (auto& object : JSONFile) {
				if (!object.isObject()) {
					_loggerInfo("At least one entry in {} is not an object.", config);
					continue;
				}

				auto& objectInfo = object["Matches"];
				if (!objectInfo || !objectInfo.isObject()) {
					_loggerInfo("Missing or non-object entry in object in config file {}", config);
					continue;
				}

				uint32_t charges = 0;
				if (auto& chargesField = object["DefaultCharges"]; chargesField.isUInt()) {
					charges = chargesField.asUInt() > 0 ? chargesField.asUInt() : charges;
				}
				else {
					_loggerInfo("MIssing required field DefaultCharges. Abortin load.");
					continue;
				}

				uint32_t chargeCost = 0;
				if (auto& chargeCostField = object["DefaultCost"]; chargeCostField.isUInt()) {
					chargeCost = chargeCostField.asUInt() > 0 ? chargeCostField.asUInt() : charges;
				}
				else {
					_loggerInfo("MIssing required field DefaultCost. Abortin load.");
					continue;
				}

				auto members = objectInfo.getMemberNames();
				for (auto& identifier : members) {
					auto& member = objectInfo[identifier];
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
							"Couldn't resolve a form in {}:\n    >Name: {}\n    >Key: {}\n    "
							">Value: {}",
							config,
							identifier,
							!key ? "KEY MISSING" : _debugEDID(key),
							!value ? "VALUE MISSING" : _debugEDID(value));
						continue;
					}

					auto* keySpell = key->As<RE::SpellItem>();
					auto* valueSpell = value->As<RE::EnchantmentItem>();
					if (!(keySpell && valueSpell)) {
						_loggerInfo(
							"Forms in {} exist, but are of unexpected type:\n    >Name: {}\n    "
							">Key: {}\n    >Value: {}",
							config,
							identifier,
							!keySpell ? "KEY MISSING" : _debugEDID(keySpell),
							!valueSpell ? "VALUE MISSING" : _debugEDID(valueSpell));
						continue;
					}

					auto keyEnchantment = Enchantment(valueSpell);
					keyEnchantment.charges = charges;
					keyEnchantment.chargeTime = keySpell->GetChargeTime();
					keyEnchantment.cost = chargeCost;
					spellEnchantments.emplace(keySpell, keyEnchantment);
				}
			}
		}
		return true;
	}
}
