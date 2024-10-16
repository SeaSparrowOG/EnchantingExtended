#include "FilterFlags.h"

#include "Data/CreatedObjectManager.h"
#include "Events/ActivationListener.h"
#include "Ext/TESAmmo.h"
#include "RE/Offset.h"
#include "Settings/GlobalSettings.h"
#include "Settings/INISettings.h"

#define NOGDI
#undef GetObject

#include <xbyak/xbyak.h>

namespace Hooks
{
	void FilterFlags::Install()
	{
		ConstructorPatch();

		ItemEntryPatch();
		EffectEntryPatch();
		EnchantmentEntryPatch();

		DisenchantSelectPatch();
		DisenchantEnablePatch();
		DisenchantLearnPatch();
		ButtonTextPatch();

		CanSelectEntryPatch();
		SelectEntryPatch();
		ComputeMagnitudePatch();
		ChargeSliderPatch();
		ItemPreviewPatch();
	}

	void FilterFlags::ConstructorPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::Ctor,
			0x1B3);

		if (!REL::make_pattern<"4D 8D A6">().match(hook.address())) {
			util::report_and_fail("FilterFlags::ConstructorPatch failed to install"sv);
		}

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				lea(r12, ptr[r14 + offsetof(Menu, filterDisenchant)]);
				mov(dword[r12], FilterFlag::Disenchant);
				mov(ptr[r14 + offsetof(Menu, filterDivider)], esi);
				mov(dword[r14 + offsetof(Menu, filterItem)], FilterFlag::Item);
				mov(dword[r14 + offsetof(Menu, filterEnchantment)], FilterFlag::Enchantment);
				mov(dword[r14 + offsetof(Menu, filterSoulGem)], FilterFlag::SoulGem);
			}
		};

		Patch patch{};
		patch.ready();

		assert(patch.getSize() == 0x37);

		REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
	}

	void FilterFlags::ItemEntryPatch()
	{
		// Hook after the IsQuestObject call to play nice with Essential Favorites
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::PopulateEntryList,
			0x14D);

		if (!REL::make_pattern<"48 8B CB">().match(hook.address())) {
			util::report_and_fail("FilterFlags::ItemEntryPatch failed to install"sv);
		}

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				Xbyak::Label skip;

				mov(rcx, rsi);
				mov(rax, util::function_ptr(&FilterFlags::GetFilterFlag));
				call(rax);
				test(eax, eax);
				jz(skip);

				mov(edi, eax);

				jmp(ptr[rip]);
				dq(hook.address() + 0xD0);

				L(skip);
				jmp(ptr[rip]);
				dq(hook.address() + 0x1C7);
			}
		};

		Patch patch{};
		patch.ready();

		assert(patch.getSize() <= 0xD0);

		REL::safe_fill(hook.address(), REL::NOP, 0xD0);
		REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
	}

	void FilterFlags::EnchantmentEntryPatch()
	{
		auto init_addr = REL::ID(51285).address();
		auto hook_addr = init_addr + 0x1D5;
		auto return_addr = init_addr + 0x1F5;
		struct Patch : Xbyak::CodeGenerator
		{
			Patch(uintptr_t ret_addr, uintptr_t func_call)
			{
				mov(rcx, rbx);
				mov(rax, func_call);
				call(rax);
				cmp(eax, 1);
				mov(rsi, ret_addr);
				jmp(rsi);
			}
		} static patch{ return_addr, (uintptr_t)EvaluateEnchantment };

		auto& trampoline = SKSE::GetTrampoline();

		trampoline.write_branch<5>(hook_addr, patch.getCode());
	}

	void FilterFlags::EffectEntryPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::AddEnchantmentIfKnown,
			0x243);

		if (!REL::make_pattern<"E8">().match(hook.address())) {
			util::report_and_fail("FilterFlags::EffectEntryPatch failed to install"sv);
		}

		auto& trampoline = SKSE::GetTrampoline();
		_PushBack = trampoline.write_call<5>(hook.address(), &FilterFlags::PushBack);
	}

	void FilterFlags::DisenchantSelectPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::SelectEntry,
			0x6E);

		if (!REL::make_pattern<"F6 41 ?? 0A">().match(hook.address())) {
			util::report_and_fail("FilterFlags::DisenchantSelectPatch failed to install"sv);
		}

		static_assert(FilterFlag::Disenchant <= 0xFF);

		std::uint8_t disenchantMask = static_cast<std::uint8_t>(FilterFlag::Disenchant);
		REL::safe_write(hook.address() + 0x3, &disenchantMask, 1);
	}

	void FilterFlags::DisenchantEnablePatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::UpdateEnabledEntries,
			0x4E);

		if (!REL::make_pattern<"F6 C1 0A">().match(hook.address())) {
			util::report_and_fail("FilterFlags::DisenchantEnablePatch failed to install"sv);
		}

		static_assert(FilterFlag::Disenchant <= 0xFF);

		std::uint8_t disenchantMask = static_cast<std::uint8_t>(FilterFlag::Disenchant);
		REL::safe_write(hook.address() + 0x2, &disenchantMask, 1);
	}

	void FilterFlags::DisenchantLearnPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::DisenchantItem,
			0x41);

		if (!REL::make_pattern<"F6 41 ?? 0A">().match(hook.address())) {
			util::report_and_fail("FilterFlags::DisenchantLearnPatch failed to install"sv);
		}

		static_assert(FilterFlag::Disenchant <= 0xFF);

		std::uint8_t disenchantMask = static_cast<std::uint8_t>(FilterFlag::Disenchant);
		REL::safe_write(hook.address() + 0x3, &disenchantMask, 1);
	}

	void FilterFlags::ButtonTextPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::UpdateInterface,
			0x11D);

		if (!REL::make_pattern<"F6 41 ?? 0A">().match(hook.address())) {
			util::report_and_fail("FilterFlags::ButtonTextPatch failed to install"sv);
		}

		static_assert(FilterFlag::Disenchant <= 0xFF);

		std::uint8_t disenchantMask = static_cast<std::uint8_t>(FilterFlag::Disenchant);
		REL::safe_write(hook.address() + 0x3, &disenchantMask, 1);
	}

	void FilterFlags::CanSelectEntryPatch()
	{
		static const auto func = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::CanSelectEntry);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				mov(rax, util::function_ptr(&Ext::EnchantConstructMenu::CanSelectEntry));
				jmp(rax);
			}
		};

		Patch patch{};
		patch.ready();

		REL::safe_fill(func.address(), REL::INT3, 0x1F0);
		REL::safe_write(func.address(), patch.getCode(), patch.getSize());
	}

	void FilterFlags::SelectEntryPatch()
	{
		static const auto hook1 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::Selections::SelectEntry,
			0x42);

		static const auto hook2 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::Selections::SelectEntry,
			0x27F);

		if (!REL::make_pattern<"41 83 E8 01">().match(hook1.address()) ||
			!REL::make_pattern<"48 8B 16">().match(hook2.address())) {

			util::report_and_fail("FilterFlags::SelectEntryPatch failed to install"sv);
		}

		struct Patch1 : Xbyak::CodeGenerator
		{
			Patch1()
			{
				// mov r8d, r11d
				test(r8d, FilterFlag::Item);
				db(0x0F);  // jnz hook + 0x1E7
				db(0x85);
				dd(0x1DA);
				test(r8d, FilterFlag::Enchantment);
				db(0x0F);  // jnz hook + 0x66
				db(0x85);
				dd(0x4C);
				cmp(r8d, FilterFlag::SoulGem);
				db(0x0F);  // jnz hook + 0x1E7
				db(0x85);
				dd(0x1C3);
				nop(0x6);
			}
		};

		Patch1 patch1{};
		patch1.ready();

		assert(patch1.getSize() == 0x2A);

		struct Patch2 : Xbyak::CodeGenerator
		{
			Patch2()
			{
				// we don't have enough space here to add our new flags in the assembly, so just
				// fall through to the next jump and recheck the conditions in the function

				// func+0x27F (after selecting an enchantment)
				nop(0x20);
				// func+0x29F (after selecting an item)
				mov(rcx, rsi);
				mov(rax, util::function_ptr(&FilterFlags::GetEnabledFilters));
				call(rax);
				mov(edi, eax);
				nop(0x3A);
				// func+0x2EA (selecting null, early return)
			}
		};

		Patch2 patch2{};
		patch2.ready();

		assert(patch2.getSize() == 0x6B);

		REL::safe_write(hook1.address(), patch1.getCode(), patch1.getSize());
		REL::safe_write(hook2.address(), patch2.getCode(), patch2.getSize());
	}

	void FilterFlags::ComputeMagnitudePatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::CreateEffectFunctor::Invoke,
			0x14F);

		if (!REL::make_pattern<"83 F9 10">().match(hook.address())) {
			util::report_and_fail("FilterFlags::ComputeMagnitudePatch failed to install"sv);
		}

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				// we shouldn't need an explicit check for EffectArmor
				test(
					ecx,
					FilterFlag::EffectWeapon | FilterFlag::EffectSpecial |
						FilterFlag::EffectStaff);
				db(0x75);  // jnz hook + 0x1D
				db(0x15);
				nop(0x2);
			}
		};

		Patch patch{};
		patch.ready();

		assert(patch.getSize() == 0xA);

		REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
	}

	void FilterFlags::ChargeSliderPatch()
	{
		static const auto hook1 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::SelectEntry,
			0x2A6);

		// there seems to be some redundant code here, possibly an inlined function call
		static const auto hook2 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::SelectEntry,
			0x380);

		static const auto hook3 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::SliderClose,
			0x2B);

		static const auto hook4 = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::CalculateCharge,
			0x47);

		if (!REL::make_pattern<"83 F8 10">().match(hook1.address()) ||
			!REL::make_pattern<"41 83 7D ?? 10">().match(hook2.address()) ||
			!REL::make_pattern<"83 79 ?? 10">().match(hook3.address()) ||
			!REL::make_pattern<"83 7F ?? 10">().match(hook4.address())) {

			util::report_and_fail("FilterFlags::ChargeSliderPatch failed to install"sv);
		}

		struct Patch1 : Xbyak::CodeGenerator
		{
			Patch1()
			{
				test(
					eax,
					~(FilterFlag::EffectWeapon | FilterFlag::EffectSpecial | FilterFlag::SoulGem |
					  FilterFlag::EffectStaff));
			}
		};

		struct Patch1NoStaff : Xbyak::CodeGenerator
		{
			Patch1NoStaff()
			{
				test(
					eax,
					~(FilterFlag::EffectWeapon | FilterFlag::EffectSpecial | FilterFlag::SoulGem));
			}
		};

		struct Patch2 : Xbyak::CodeGenerator
		{
			Patch2()
			{
				mov(rbx, r13);
				test(
					dword[rbx + offsetof(Menu::CategoryListEntry, filterFlag)],
					~(FilterFlag::EffectWeapon | FilterFlag::EffectSpecial |
					  FilterFlag::EffectStaff));
			}
		};

		struct Patch2NoStaff : Xbyak::CodeGenerator
		{
			Patch2NoStaff()
			{
				mov(rbx, r13);
				test(
					dword[rbx + offsetof(Menu::CategoryListEntry, filterFlag)],
					~(FilterFlag::EffectWeapon | FilterFlag::EffectSpecial));
			}
		};

		REL::safe_fill(hook1.address(), REL::NOP, 0x8);
		REL::safe_fill(hook2.address(), REL::NOP, 0x23);

		if (!Settings::INISettings::GetSingleton()->bStaffChargeEnabled) {
			_loggerInfo("Staff enchanting charge disabled.");
			Patch1NoStaff patch1NoStaff{};
			assert(patch1NoStaff.getSize() <= 0x8);
			Patch2NoStaff patch2NoStaff{};
			assert(patch2NoStaff.getSize() <= 0x23);
			patch1NoStaff.ready();
			patch2NoStaff.ready();
			REL::safe_write(hook1.address(), patch1NoStaff.getCode(), patch1NoStaff.getSize());
			REL::safe_write(hook2.address(), patch2NoStaff.getCode(), patch2NoStaff.getSize());
		}
		else {
			_loggerInfo("Staff enchanting charge enabled.");
			Patch1 patch1{};
			assert(patch1.getSize() <= 0x8);
			Patch2 patch2{};
			assert(patch2.getSize() <= 0x23);
			patch1.ready();
			patch2.ready();
			REL::safe_write(hook1.address(), patch1.getCode(), patch1.getSize());
			REL::safe_write(hook2.address(), patch2.getCode(), patch2.getSize());
		}
		// the flag checks here just get in the way, so nop them
		REL::safe_fill(hook3.address(), REL::NOP, 0x6);
		REL::safe_fill(hook4.address(), REL::NOP, 0xA);
	}

	void FilterFlags::ItemPreviewPatch()
	{
		static const auto hook = REL::Relocation<std::uintptr_t>(
			RE::Offset::CraftingSubMenus::EnchantConstructMenu::Update,
			0xAF);

		if (!REL::make_pattern<"48 8B 86">().match(hook.address())) {
			util::report_and_fail("FilterFlags::ItemPreviewPatch failed to install"sv);
		}

		struct Patch : Xbyak::CodeGenerator
		{
			Patch()
			{
				mov(rax,
					ptr[rsi + offsetof(Menu, selected) + offsetof(Menu::Selections, effects)]);
				mov(rcx, ptr[rax]);
				mov(ecx, dword[rcx + offsetof(Menu::CategoryListEntry, filterFlag)]);
				mov(rax, util::function_ptr(&FilterFlags::GetFormTypeFromEffectFlag));
				call(rax);
				mov(r14d, eax);
			}
		};

		Patch patch{};
		patch.ready();

		assert(patch.getSize() <= 0x1D);

		REL::safe_fill(hook.address(), REL::NOP, 0x1D);
		REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
	}

	//Controller functions

	void FilterFlags::PushBack(void* a_arg1, Menu::EnchantmentEntry* a_entry)
	{
		if (a_entry->filterFlag.underlying() == FilterFlag::EffectWeapon) {
			const auto manager = Data::CreatedObjectManager::GetSingleton();
			auto* enchantment = a_entry->data;
			bool isStaffEnchantment = enchantment ? enchantment->GetSpellType() == RE::MagicSystem::SpellType::kStaffEnchantment : false;

			if (isStaffEnchantment && Staves::StaffEnchantManager::GetSingleton()->IsInValidStaffWorkbench()) {
				auto* costliest = enchantment->GetCostliestEffectItem();
				auto* baseEffect = costliest ? costliest->baseEffect : nullptr;
				if (!baseEffect)
					a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::None);

				auto delivery = baseEffect->data.delivery;
				auto casting = baseEffect->data.castingType;
				if (casting == RE::MagicSystem::CastingType::kConstantEffect ||
					delivery == RE::MagicSystem::Delivery::kTouch) {
					a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::None);
				}
				else if (delivery == RE::MagicSystem::Delivery::kAimed) {
					if (casting == RE::MagicSystem::CastingType::kFireAndForget) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::FFAimed);
					}
					else if (casting == RE::MagicSystem::CastingType::kConcentration) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::ConcAimed);
					}
				}
				else if (delivery == RE::MagicSystem::Delivery::kSelf) {
					if (casting == RE::MagicSystem::CastingType::kFireAndForget) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::FFSelf);
					}
					else if (casting == RE::MagicSystem::CastingType::kConcentration) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::ConcSelf);
					}
				}
				else if (delivery == RE::MagicSystem::Delivery::kTargetActor) {
					if (casting == RE::MagicSystem::CastingType::kFireAndForget) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::FFActor);
					}
					else if (casting == RE::MagicSystem::CastingType::kConcentration) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::ConcActor);
					}
				}
				else if (delivery == RE::MagicSystem::Delivery::kTargetLocation) {
					if (casting == RE::MagicSystem::CastingType::kFireAndForget) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::FFLocation);
					}
					else if (casting == RE::MagicSystem::CastingType::kConcentration) {
						a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::ConcLocation);
					}
				}
			}
			else if (manager->IsBaseAmmoEnchantment(a_entry->data))
				{
				const auto globalSettings = Settings::GlobalSettings::GetSingleton();
				if (!globalSettings->AmmoEnchantingEnabled()) {
					RE::free(a_entry);
					return;
				}

				a_entry->filterFlag = static_cast<Menu::FilterFlag>(FilterFlag::EffectSpecial);
			}
		}

		return _PushBack(a_arg1, a_entry);
	}

	bool FilterFlags::EvaluateEnchantment(RE::EnchantmentItem* a_item)
	{
		if (!Staves::StaffEnchantManager::GetSingleton()->IsInValidStaffWorkbench()) {
			return (
				(a_item->GetDelivery() == RE::MagicSystem::Delivery::kSelf &&
				 a_item->GetCastingType() == RE::MagicSystem::CastingType::kConstantEffect)
				||
				(a_item->GetDelivery() == RE::MagicSystem::Delivery::kTouch &&
				 a_item->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget));
		}

		auto casting_type = a_item->GetCastingType();
		auto deliver_type = a_item->GetDelivery();

		switch (casting_type)
		{
		case RE::MagicSystem::CastingType::kFireAndForget:
		case RE::MagicSystem::CastingType::kConcentration:
			break;

		default:
			return false;
		}

		switch (deliver_type)
		{
		case RE::MagicSystem::Delivery::kAimed:
		case RE::MagicSystem::Delivery::kTargetActor:
		case RE::MagicSystem::Delivery::kTargetLocation:
		case RE::MagicSystem::Delivery::kSelf:
			break;
		default:
			return false;
		}
		return true;
	}

	std::uint32_t FilterFlags::GetFilterFlag(RE::InventoryEntryData* a_entry)
	{
		const auto object = a_entry->GetObject();

		// IsQuestObject was checked before the hook
		if (!object || !object->GetName() || !object->GetPlayable())
			return FilterFlag::None;

		const auto defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		const auto disallowEnchanting = defaultObjects->GetObject<RE::BGSKeyword>(
			RE::DEFAULT_OBJECT::kKeywordDisallowEnchanting);
		auto staff = object->As<RE::TESObjectWEAP>();
		bool isStaff = staff ? staff->IsStaff() : false;

		if (Staves::StaffEnchantManager::GetSingleton()->IsInValidStaffWorkbench()) {
			bool isSoulGem = object ? object->IsSoulGem() : false;
			bool isFuel = object ? object->HasKeywordByEditorID("STEN_StaffFuel") : false;
			bool hasSoul = a_entry->GetSoulLevel() != RE::SOUL_LEVEL::kNone ? true : false;

			if (isStaff) {
				if (disallowEnchanting && staff->HasKeyword(disallowEnchanting)) {
					return FilterFlag::None;
				}
				else if (!a_entry->IsEnchanted()) {
					return FilterFlag::EnchantSpecial;
				}
				return FilterFlag::None;
			}

			if (Staves::StaffEnchantManager::GetSingleton()->IsInAdvancedStaffEnchanter()) {
				if (isFuel) {
					if (!hasSoul) {
						auto* newList = new RE::ExtraDataList();
						newList->Add(new RE::ExtraSoul(RE::SOUL_LEVEL::kGrand));
						a_entry->AddExtraList(newList);
					}
					return FilterFlag::SoulGem;
				}
				else if (isSoulGem && hasSoul &&
					Settings::INISettings::GetSingleton()->bUseSoulGemsForStaves) {
					return FilterFlag::SoulGem;
				}
			}
			else {
				if (isSoulGem && hasSoul) {
					return FilterFlag::SoulGem;
				}
			}
			return FilterFlag::None;
		}
		else if (const auto armor = object->As<RE::TESObjectARMO>()) {
			if (disallowEnchanting && armor->HasKeyword(disallowEnchanting)) {
				return FilterFlag::None;
			}
			else if (!a_entry->IsEnchanted()) {
				return FilterFlag::EnchantArmor;
			}
			else {
				return FilterFlag::DisenchantArmor;
			}
		}
		else if (const auto weapon = object->As<RE::TESObjectWEAP>()) {
			static const auto unarmedWeapon = REL::Relocation<RE::TESObjectWEAP**>{
				RE::Offset::UnarmedWeapon
			};

			if (isStaff ||
				disallowEnchanting && weapon->HasKeyword(disallowEnchanting) ||
				weapon == *unarmedWeapon.get() ||
				(weapon->weaponData.flags.all(RE::TESObjectWEAP::Data::Flag::kNonPlayable))) {

				return FilterFlag::None;
			}
			else if (!a_entry->IsEnchanted()) {
				return FilterFlag::EnchantWeapon;
			}
			else {
				return FilterFlag::DisenchantWeapon;
			}
		} 
		else if (const auto ammo = object->As<RE::TESAmmo>()) {
			const auto globalSettings = Settings::GlobalSettings::GetSingleton();
			if (!globalSettings->AmmoEnchantingEnabled() ||
				(disallowEnchanting && ammo->HasKeyword(disallowEnchanting))) {

				return FilterFlag::None;
			}
			else if (!a_entry->IsEnchanted() && !Ext::TESAmmo::GetEnchantment(ammo)) {
				return FilterFlag::EnchantSpecial;
			}
			else {
				if (globalSettings->AmmoEnchantingAllowDisenchant()) {
					return FilterFlag::DisenchantSpecial;
				}
				else {
					return FilterFlag::None;
				}
			}
		}
		else if (const auto soulGem = object->As<RE::TESSoulGem>()) {
			if (a_entry->GetSoulLevel() == RE::SOUL_LEVEL::kNone) {
				return FilterFlag::None;
			}
			else {
				return FilterFlag::SoulGem;
			}
		}
		else {
			return FilterFlag::None;
		}
	}

	std::uint32_t FilterFlags::GetEnabledFilters(Menu::Selections* a_selected)
	{
		std::uint32_t filters = FilterFlag::SoulGem;
		bool isInStaffEnchanter = Staves::StaffEnchantManager::GetSingleton()
									  ->IsInValidStaffWorkbench();
		if (isInStaffEnchanter) {
			if (!a_selected->effects.empty()) {
				filters |= FilterFlag::EnchantSpecial;
				for (auto& selectedEffect : a_selected->effects) {
					filters |= selectedEffect->filterFlag.underlying();
				}
			}
			else {
				filters = FilterFlag::All;
			}
		}
		else {

			if (a_selected->item) {
				switch (a_selected->item->filterFlag.underlying()) {
				case FilterFlag::EnchantWeapon:
					filters = FilterFlag::SoulGem | FilterFlag::EffectWeapon |
						FilterFlag::EnchantWeapon;
					break;
				case FilterFlag::EnchantArmor:
					filters = FilterFlag::SoulGem | FilterFlag::EffectArmor |
						FilterFlag::EnchantArmor;
					break;
				case FilterFlag::EnchantSpecial:
					filters = FilterFlag::SoulGem | FilterFlag::EffectSpecial |
						FilterFlag::DisenchantSpecial;
					break;
				}
			}

			if (!a_selected->effects.empty()) {
				switch (a_selected->effects[0]->filterFlag.underlying()) {
				case FilterFlag::EffectWeapon:
					filters |= FilterFlag::EffectWeapon | FilterFlag::EnchantWeapon;
					break;
				case FilterFlag::EffectArmor:
					filters |= FilterFlag::EffectArmor | FilterFlag::EnchantArmor;
					break;
				case FilterFlag::EffectSpecial:
					filters |= FilterFlag::EffectSpecial | FilterFlag::EnchantSpecial;
					break;
				}
			}

			if (!a_selected->item && !a_selected->effects.empty()) {
				filters |= FilterFlag::Enchantment;
			}

			if (!(filters & (FilterFlag::Enchantment | FilterFlag::Item))) {
				filters = FilterFlag::All;
			}
		}
		return filters;
	}

	RE::FormType FilterFlags::GetFormTypeFromEffectFlag(std::uint32_t a_flag)
	{
		switch (a_flag) {
		case FilterFlag::EffectWeapon:
			return RE::FormType::Weapon;
		case FilterFlag::EffectSpecial:
			return RE::FormType::Ammo;
		default:
			return RE::FormType::Armor;
		}
	}
}
