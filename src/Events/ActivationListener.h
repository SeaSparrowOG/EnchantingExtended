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
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESFurnitureEvent* a_event,
			RE::BSTEventSource<RE::TESFurnitureEvent>*) override;
		bool ReadSettings();

		bool isInValidStaffWorkbench{ false };
		bool isInValidAmmoWorkbench{ true };

		std::unordered_map<RE::SpellItem*, RE::EnchantmentItem*> spellEnchantments;
	};
}
