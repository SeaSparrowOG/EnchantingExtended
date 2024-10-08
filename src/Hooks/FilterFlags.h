#pragma once

#include "Ext/EnchantConstructMenu.h"

namespace Hooks
{
	class FilterFlags
	{
	public:
		using FilterFlag = Ext::EnchantConstructMenu::FilterFlag;
		using Menu = RE::CraftingSubMenus::EnchantConstructMenu;

		FilterFlags() = delete;

		static void Install();

	private:
		// Set up category filters in EnchantConstructMenu
		static void ConstructorPatch();
		// Allow Ammo forms to be added to entry list and assign flags
		static void ItemEntryPatch();
		// Assign EffectAmmo flag for base ammo enchantments
		static void EffectEntryPatch();
		// Recognize DisenchantAmmo for learning enchantments
		static void DisenchantSelectPatch();
		// Enable DisenchantAmmo entries
		static void DisenchantEnablePatch();
		// Learn effect when disenchanting from DisenchantAmmo entries
		static void DisenchantLearnPatch();
		// Correct button text for DisenchantAmmo
		static void ButtonTextPatch();
		// Handle Ammo flags when determining whether entries are selectable
		static void CanSelectEntryPatch();
		// Handle Ammo flags when selecting an entry
		static void SelectEntryPatch();
		// Compute initial magnitude without soul gem multiplier
		static void ComputeMagnitudePatch();
		// Enable magnitude/charge slider for Ammo
		static void ChargeSliderPatch();
		// When selecting an enchantment without an item, identify Ammo form type
		static void ItemPreviewPatch();

		// Allows for staff enchantments to show in the menu. The menu filters out non-self const // touch ff enchantments by default.
		static void EnchantmentEntryPatch();

		static void PushBack(void* a_arg1, Menu::EnchantmentEntry* a_entry);

		static std::uint32_t GetFilterFlag(RE::InventoryEntryData* a_entry);

		static std::uint32_t GetEnabledFilters(Menu::Selections* a_selected);

		static bool EvaluateEnchantment(RE::EnchantmentItem* a_item);

		static RE::FormType GetFormTypeFromEffectFlag(std::uint32_t a_flag);

		inline static REL::Relocation<decltype(&PushBack)> _PushBack;
	};
}
