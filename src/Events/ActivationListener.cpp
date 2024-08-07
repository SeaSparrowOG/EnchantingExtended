#include "Events/ActivationListener.h"

namespace ActivationListener
{
	bool EnchantingTable::RegisterListener()
	{
		auto* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!eventHolder)
			return false;

		eventHolder->AddEventSink(this);
		return true;
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
}
