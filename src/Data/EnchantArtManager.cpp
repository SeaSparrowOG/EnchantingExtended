#include "EnchantArtManager.h"

#include "Ext/NiAVObject.h"
#include "Ext/TaskQueueInterface.h"

namespace Data
{
	EnchantArtManager* EnchantArtManager::GetSingleton()
	{
		static EnchantArtManager singleton{};
		return &singleton;
	}

	void EnchantArtManager::UpdateAmmoEnchantment(
		RE::Actor* a_actor,
		RE::EnchantmentItem* a_enchantment)
	{
		std::scoped_lock lk{ _mutex };

		auto handle = a_actor->GetHandle();
		if (auto it = _fxMap.find(handle); it != _fxMap.end()) {
			auto& fx = it->second;
			if (fx.quiverFXController) {
				fx.quiverFXController->Stop();
				delete fx.quiverFXController;
			}

			if (fx.arrowEffectModel) {
				RE::BSResource::FreeRequestedModel(fx.arrowEffectModel);
				delete fx.arrowEffectModel;
			}

			_fxMap.erase(it);
		}

		if (a_enchantment) {
			const auto costliestEffect = a_enchantment->GetCostliestEffectItem();
			const auto baseEffect = costliestEffect ? costliestEffect->baseEffect : nullptr;

			EnchantFX fx{};

			if (baseEffect) {
				if (auto& enchantArt = baseEffect->data.enchantEffectArt) {
					fx.quiverFXController = new Ext::AmmoEnchantmentController(
						a_actor,
						enchantArt);
					fx.quiverFXController->Start();
				}

				if (auto& castingArt = baseEffect->data.castingArt) {
					auto modelResult = RE::BSResource::RequestModelDirect(
						baseEffect->data.castingArt->model.c_str(),
						fx.arrowEffectModel);

					if (modelResult != 0) {
						RE::BSResource::FreeRequestedModel(fx.arrowEffectModel);
						fx.arrowEffectModel = nullptr;
					}
				}
			}

			_fxMap.insert({ handle, std::move(fx) });
		}
	}

	void EnchantArtManager::AttachArrow(RE::Actor* a_actor)
	{
		std::scoped_lock lk{ _mutex };

		auto handle = a_actor->GetHandle();
		if (auto it = _fxMap.find(handle); it != _fxMap.end()) {
			auto& fx = it->second;
			if (fx.arrowEffectModel && fx.arrowEffectModel->data) {
				auto& model = fx.arrowEffectModel->data;

				if (const auto root = GetArrowAttachRoot(a_actor)) {

					const auto clone = Ext::NiAVObject::Clone(model.get());
					Ext::NiAVObject::SetValueNodeHidden(clone, true);

					Ext::TaskQueueInterface::Attach3D(
						RE::TaskQueueInterface::GetSingleton(),
						clone,
						root);
				}
			}
		}
	}

	RE::NiAVObject* EnchantArtManager::GetArrowAttachRoot(RE::Actor* a_actor)
	{
		const auto process = a_actor->currentProcess;
		const auto middleHigh = process ? process->middleHigh : nullptr;
		const auto rightHand = middleHigh ? middleHigh->rightHand : nullptr;
		const auto weapon = rightHand && rightHand->object
			? rightHand->object->As<RE::TESObjectWEAP>()
			: nullptr;

		if (!weapon)
			return nullptr;

		bool isCrossbow = weapon->weaponData.animationType == RE::WEAPON_TYPE::kCrossbow;
		RE::BSFixedString name = isCrossbow ? "NPC R MagicNode [RMag]"sv : "Weapon"sv;

		const auto actor3D = a_actor->Get3D();
		const auto actorNode = actor3D ? actor3D->AsNode() : nullptr;
		return actorNode ? actorNode->GetObjectByName(name) : nullptr;
	}
}
