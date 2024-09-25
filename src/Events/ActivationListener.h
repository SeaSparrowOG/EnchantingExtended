#pragma once

namespace ActivationListener
{
#define continueEvent return RE::BSEventNotifyControl::kContinue

	class EnchantingTable : public ISingleton<EnchantingTable>,
		public RE::BSTEventSink<RE::TESFurnitureEvent>
	{
	public:
		bool RegisterListener();
		bool IsInValidStaffWorkbench();
		bool IsInValidAmmoWorkbench();

	private:
		struct Enchantment
		{
			RE::EnchantmentItem* enchantment;
			float chargeTime;
			uint32_t charges;
			uint32_t cost;

			Enchantment() = delete;
			Enchantment(RE::EnchantmentItem* a_template)
			{
				this->enchantment = a_template;
				this->chargeTime = 0.5;
				this->cost = 0;
				this->charges = 0;
			}
		};

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESFurnitureEvent* a_event,
			RE::BSTEventSource<RE::TESFurnitureEvent>*) override;
		bool ReadSettings();

		bool isInValidStaffWorkbench{ false };
		bool isInValidAmmoWorkbench{ true };

		std::unordered_map<RE::SpellItem*, Enchantment> spellEnchantments;
	};
}
