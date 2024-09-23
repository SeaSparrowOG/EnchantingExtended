#include "CreatedObjectManager.h"

namespace Data
{
	void CreatedObjectManager::AddFailedEnchantmentLoad(
		RE::ExtraEnchantment* a_exEnchantment,
		RE::FormID a_formID)
	{
		failedFormLoads[a_exEnchantment] = a_formID;
	}

	bool CreatedObjectManager::Save(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc->WriteRecordData(baseExplosions.size())) {
			_loggerError("Failed to write size of baseExplosions");
			return false;
		}

		for (const auto& [effect, explosion] : baseExplosions) {
			if (!a_intfc->WriteRecordData(effect->formID)) {
				_loggerError("Failed to write FormID ({:08X})", effect->formID);
				return false;
			}

			if (!a_intfc->WriteRecordData(explosion->formID)) {
				_loggerError("Failed to write FormID ({:08X})", explosion->formID);
				return false;
			}
		}

		if (!a_intfc->WriteRecordData(ammoEnchantments.size())) {
			_loggerError("Failed to write size of ammoEnchantments");
			return false;
		}

		for (const auto& entry : ammoEnchantments) {

			RE::BGSExplosion* createdExplosion;
			try {
				createdExplosion = createdExplosions.at(entry.magicItem->As<RE::EnchantmentItem>());
			}
			catch (std::out_of_range&) {
				_loggerError("Failed to look up created explosion");
				return false;
			}

			if (!a_intfc->WriteRecordData(static_cast<std::uint32_t>(entry.refCount))) {
				_loggerError("Failed to write refCount ({})", entry.magicItem->formID);
				return false;
			}

			if (!SaveEnchantment(entry.magicItem, a_intfc)) {
				_loggerError("Failed to save enchantment");
				return false;
			}

			if (!a_intfc->WriteRecordData(createdExplosion->formID)) {
				_loggerError("Failed to write FormID ({:08X})", createdExplosion->formID);
				return false;
			}
		}
		return true;
	}

	bool CreatedObjectManager::Load(SKSE::SerializationInterface* a_intfc)
	{
		Revert();

		std::size_t baseExplosionsSize;
		if (!a_intfc->ReadRecordData(baseExplosionsSize)) {
			_loggerError("Failed to read size of baseExplosions");
			return false;
		}

		for (std::size_t i = 0; i < baseExplosionsSize; i++) {

			RE::FormID effectOldFormID;
			if (!a_intfc->ReadRecordData(effectOldFormID)) {
				_loggerError("Failed to read FormID");
				return false;
			}

			RE::FormID effectFormID;
			if (!a_intfc->ResolveFormID(effectOldFormID, effectFormID)) {
				_loggerError("Failed to resolve FormID ({:08X})", effectOldFormID);
				return false;
			}

			RE::FormID explosionOldFormID;
			if (!a_intfc->ReadRecordData(explosionOldFormID)) {
				_loggerError("Failed to read FormID");
				return false;
			}

			RE::FormID explosionFormID;
			if (!a_intfc->ResolveFormID(explosionOldFormID, explosionFormID)) {
				_loggerError("Failed to resolve FormID ({:08X})", explosionOldFormID);
				return false;
			}

			auto effect = RE::TESForm::LookupByID<RE::EffectSetting>(effectFormID);
			auto explosion = RE::TESForm::LookupByID<RE::BGSExplosion>(explosionFormID);

			if (effect && explosion) {
				baseExplosions.insert({ effect, explosion });
			}
		}

		RE::BSTArrayBase::size_type ammoEnchantmentsSize;
		if (!a_intfc->ReadRecordData(ammoEnchantmentsSize)) {
			_loggerError("Failed to read size of ammoEnchantments");
			return false;
		}

		for (RE::BSTArrayBase::size_type i = 0; i < ammoEnchantmentsSize; i++) {
			EnchantmentEntry entry{};

			std::uint32_t refCount;
			if (!a_intfc->ReadRecordData(refCount)) {
				_loggerError("Failed to read refCount");
				return false;
			}

			entry.refCount = refCount;

			RE::EnchantmentItem* enchantment;
			if (!LoadEnchantment(enchantment, a_intfc)) {
				_loggerError("Failed to load enchantment");
				return false;
			}

			const auto costliestEffect = enchantment->GetCostliestEffectItem()->baseEffect;
			if (!costliestEffect) {
				_loggerError(
					"Failed to look up base effect of enchantment ({:08X})",
					enchantment->formID);
			}


			RE::FormID explosionFormID;
			if (!a_intfc->ReadRecordData(explosionFormID)) {
				_loggerError("Failed to read FormID");
				return false;
			}

			RE::BGSExplosion* explosion = nullptr;
			RE::TESForm* explosionForm = RE::TESForm::LookupByID(explosionFormID);
			if (explosionForm) {
				explosion = explosionForm->As<RE::BGSExplosion>();
				if (!explosion) {
					RE::BGSSaveLoadGame::GetSingleton()->ClearForm(explosionForm);
				}
			}
			if (!explosion) {
				RE::BGSExplosion* baseExplosion;
				try {
					baseExplosion = baseExplosions.at(costliestEffect);
				}
				catch (std::out_of_range&) {
					_loggerError("Failed to look up base explosion");
					return false;
				}

				explosion = EnchantExplosion(baseExplosion, enchantment, explosionFormID);
			}

			entry.magicItem = enchantment;

			ammoEnchantments.push_back(std::move(entry));
			createdExplosions.insert({ enchantment, explosion });
		}

		for (auto& [exEnchantment, formID] : failedFormLoads) {
			exEnchantment->enchantment = RE::TESForm::LookupByID<RE::EnchantmentItem>(formID);
		}
		failedFormLoads.clear();

		return true;
	}

	void CreatedObjectManager::Revert()
	{
		ammoEnchantments.clear();
		baseExplosions.clear();
		createdExplosions.clear();
	}

	bool CreatedObjectManager::SaveEnchantment(
		const RE::MagicItem* a_enchantment,
		SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc->WriteRecordData(a_enchantment->formID)) {
			_loggerError("Failed to write FormID ({:08X})", a_enchantment->formID);
			return false;
		}

		if (!a_intfc->WriteRecordData(a_enchantment->effects.size())) {
			_loggerError("Failed to write size of enchantment effects");
			return false;
		}

		for (auto& effect : a_enchantment->effects) {
			if (!a_intfc->WriteRecordData(effect->effectItem.magnitude)) {
				_loggerError("Failed to write effect magnitude");
				return false;
			}
			if (!a_intfc->WriteRecordData(effect->effectItem.area)) {
				_loggerError("Failed to write effect area");
				return false;
			}
			if (!a_intfc->WriteRecordData(effect->effectItem.duration)) {
				_loggerError("Failed to write effect duration");
				return false;
			}
			if (!a_intfc->WriteRecordData(effect->baseEffect->formID)) {
				_loggerError("Failed to write FormID ({:08X})", effect->baseEffect->formID);
				return false;
			}
			if (!a_intfc->WriteRecordData(effect->cost)) {
				_loggerError("Failed to write effect cost");
				return false;
			}

			RE::TESConditionItem* item;
			for (item = effect->conditions.head; item; item = item->next) {
				if (!a_intfc->WriteRecordData(&item, sizeof(void*))) {
					_loggerError("Failed to write condition item");
					return false;
				}
				if (!a_intfc->WriteRecordData(item->data)) {
					_loggerError("Failed to write condition data");
					return false;
				}
			}

			if (!a_intfc->WriteRecordData(&item, sizeof(void*))) {
				_loggerError("Failed to write condition end");
				return false;
			}
		}

		return true;
	}

	bool CreatedObjectManager::LoadEnchantment(
		RE::EnchantmentItem*& a_enchantment,
		SKSE::SerializationInterface* a_intfc)
	{
		RE::FormID formID;
		if (!a_intfc->ReadRecordData(formID)) {
			_loggerError("Failed to read FormID");
			return false;
		}

		RE::BSTArray<RE::Effect> effects;

		RE::BSTArrayBase::size_type effectsSize;
		if (!a_intfc->ReadRecordData(effectsSize)) {
			_loggerError("Failed to read size of enchantment effects");
			return false;
		}

		effects.resize(effectsSize);

		for (RE::BSTArrayBase::size_type i = 0; i < effectsSize; i++) {
			auto& effect = effects[i];

			if (!a_intfc->ReadRecordData(effect.effectItem.magnitude)) {
				_loggerError("Failed to read effect magnitude");
				return false;
			}
			if (!a_intfc->ReadRecordData(effect.effectItem.area)) {
				_loggerError("Failed to read effect area");
				return false;
			}
			if (!a_intfc->ReadRecordData(effect.effectItem.duration)) {
				_loggerError("Failed to read effect duration");
				return false;
			}
			RE::FormID baseEffectOldFormID;
			if (!a_intfc->ReadRecordData(baseEffectOldFormID)) {
				_loggerError("Failed to read FormID");
				return false;
			}
			RE::FormID baseEffectFormID;
			if (!a_intfc->ResolveFormID(baseEffectOldFormID, baseEffectFormID)) {
				_loggerError("Failed to resolve FormID ({:08X})", baseEffectOldFormID);
				return false;
			}
			effect.baseEffect = RE::TESForm::LookupByID<RE::EffectSetting>(baseEffectFormID);
			if (!a_intfc->ReadRecordData(effect.cost)) {
				_loggerError("Failed to read effect cost");
				return false;
			}

			std::uintptr_t conditionItem;
			if (!a_intfc->ReadRecordData(conditionItem)) {
				_loggerError("Failed to read condition head");
				return false;
			}
			if (conditionItem) {
				auto item = effect.conditions.head = new RE::TESConditionItem();
				while (true) {
					if (!a_intfc->ReadRecordData(item->data)) {
						_loggerError("Failed to read condition data");
						return false;
					}
					if (!a_intfc->ReadRecordData(conditionItem)) {
						_loggerError("Failed to read condition item");
						return false;
					}
					if (!conditionItem)
						break;

					item = item->next = new RE::TESConditionItem();
				}
			}
			else {
				effect.conditions.head = nullptr;
			}
		}

		a_enchantment = nullptr;
		if (const auto enchantmentForm = RE::TESForm::LookupByID(formID)) {
			a_enchantment = enchantmentForm->As<RE::EnchantmentItem>();
			if (!a_enchantment) {
				RE::BGSSaveLoadGame::GetSingleton()->ClearForm(enchantmentForm);
			}
		}
		if (!a_enchantment) {
			a_enchantment = CreateEnchantment(effects, true);
			a_enchantment->SetFormID(formID, false);
		}

		return a_enchantment != nullptr;
	}
}
