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

namespace Staves
{
	bool StaffEnchantManager::RegisterListener()
	{
		auto* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!eventHolder)
			return false;

		eventHolder->AddEventSink(this);
		return ReadSettings();
	}

	bool StaffEnchantManager::IsInAdvancedStaffEnchanter()
	{
		return this->isInAdvancedStaffEnchanter;
	}

	bool StaffEnchantManager::IsInValidStaffWorkbench()
	{
		return isInValidStaffWorkbench;
	}

	Enchantment StaffEnchantManager::GetEnchantmentInfo(
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

		if (isInAdvancedStaffEnchanter) {
			response.charges *= 2;
		}

		return response;
	}

	RE::BSEventNotifyControl StaffEnchantManager::ProcessEvent(
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

		isInAdvancedStaffEnchanter = eventFurniture->HasKeywordString(
			"STEN_AdvancedStaffEnchanter");

		auto* playerREF = RE::PlayerCharacter::GetSingleton();
		for (auto& pair : spellEnchantments) {
			auto* spell = pair.first;
			auto* enchant = pair.second.enchantment;

			if (!playerREF->HasSpell(spell)) {
				enchant->formFlags &= ~RE::TESForm::RecordFlags::kKnown;
				enchant->AddChange(RE::TESForm::ChangeFlags::kFlags);
				continue;
			}
			enchant->formFlags |= RE::TESForm::RecordFlags::kKnown;
			enchant->AddChange(RE::TESForm::ChangeFlags::kFlags);
		}
		continueEvent;
	}

	bool StaffEnchantManager::ReadSettings()
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

		auto isSelfTargettingFixInstalled =
			std::filesystem::exists(std::filesystem::path("Data\\Meshes\\Actors\\Character\\_1stPerson\\Animations\\DynamicAnimationReplacer\\_CustomConditions\\3700614\\mlh_equip.hkx"));

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

					if (keySpell->GetDelivery() == RE::MagicSystem::Delivery::kSelf &&
						!isSelfTargettingFixInstalled) {
						_loggerInfo(
							"Note: {} was discarded, because the Self Targetting fix was not "
							"detected.", keySpell->GetName());
						continue;
					}

					bool isValid = true;
					for (auto& effect : keySpell->effects) {
						if (!effect->baseEffect)
							continue;
						if (!effect->baseEffect->HasKeywordString("RitualSpellEffect"sv))
							continue;
						isValid = false;
						break;
					}
					if (!isValid) {
						_loggerInfo(
							"Note: {} was discarded because it is a ritual spell.",
							keySpell->GetName());
						continue;
					}

					valueSpell->data.costOverride = chargeCost;
					valueSpell->data.flags |= RE::EnchantmentItem::EnchantmentFlag::kCostOverride;
					valueSpell->effects = keySpell->effects;
					valueSpell->data.spellType = RE::MagicSystem::SpellType::kStaffEnchantment;
					valueSpell->SetDelivery(keySpell->GetDelivery());
					valueSpell->SetCastingType(keySpell->GetCastingType());
					valueSpell->fullName = keySpell->fullName;

					auto keyEnchantment = Enchantment(valueSpell);
					keyEnchantment.charges = charges;
					keyEnchantment.chargeTime = keySpell->GetChargeTime();
					keyEnchantment.cost = chargeCost;
					spellEnchantments.emplace(keySpell, keyEnchantment);
				}
			}
		}

		_loggerInfo("Supported spells ({}):", spellEnchantments.size());
		size_t maxSize = 0;
		std::vector<std::string> sortedNames{};
		for (auto& pair : this->spellEnchantments) {
			std::string tempName = pair.first->GetName();
			if (tempName.size() > maxSize) {
				maxSize = tempName.size();
			}
			sortedNames.push_back(tempName);
		}
		std::sort(sortedNames.begin(), sortedNames.end());

		for (auto it = sortedNames.begin(); it != sortedNames.end(); ++it) {
			std::string name1 = "";
			std::string name2 = "";
			std::string name3 = "";

			name1 = *it;
			while (name1.size() < maxSize) {
				name1 += " ";
			}
			it++;
			if (it != sortedNames.end()) {
				name2 = *it;
				while (name2.size() < maxSize) {
					name2 += " ";
				}
				it++;
				if (it != sortedNames.end()) {
					name3 = *it;
					while (name3.size() < maxSize) {
						name3 += " ";
					}
				}
			}

			_loggerInfo("{}  {}  {}", name1, name2, name3);
			if (it == sortedNames.end()) {
				break;
			}
		}
		return true;
	}
}
