#pragma once

namespace Staves
{
#define continueEvent return RE::BSEventNotifyControl::kContinue

	struct Enchantment
	{
		RE::EnchantmentItem* enchantment;
		float chargeTime;
		uint32_t charges;
		uint32_t cost;

		Enchantment()
		{
			this->enchantment = nullptr;
			this->chargeTime = 0.5;
			this->cost = 0;
			this->charges = 0;
		}

		Enchantment(RE::EnchantmentItem* a_template)
		{
			this->enchantment = a_template;
			this->chargeTime = 0.5;
			this->cost = 0;
			this->charges = 0;
		}
	};

	class StaffEnchantManager : public ISingleton<StaffEnchantManager>,
		public RE::BSTEventSink<RE::TESFurnitureEvent>
	{
	public:
		bool RegisterListener();
		bool IsInValidStaffWorkbench();
		Enchantment GetEnchantmentInfo(const std::vector<RE::EnchantmentItem*>& a_enchantments);

	private:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESFurnitureEvent* a_event,
			RE::BSTEventSource<RE::TESFurnitureEvent>*) override;
		bool ReadSettings();

		bool isInValidStaffWorkbench{ false };

		std::unordered_map<RE::SpellItem*, Enchantment> spellEnchantments;
	};
}
