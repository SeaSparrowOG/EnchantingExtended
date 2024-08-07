#pragma once

namespace Ext
{
	namespace EnchantConstructMenu
	{
		using Menu = RE::CraftingSubMenus::EnchantConstructMenu;
		using CategoryListEntry = Menu::CategoryListEntry;
		using EnchantmentEntry = Menu::EnchantmentEntry;
		using ItemChangeEntry = Menu::ItemChangeEntry;

		struct FilterFlag
		{
			FilterFlag() = delete;

			enum FILTER_FLAG : std::uint32_t
			{
				EnchantWeapon = 0x1,
				DisenchantWeapon = 0x2,
				EnchantArmor = 0x4,
				DisenchantArmor = 0x8,
				EffectWeapon = 0x10,
				EffectArmor = 0x20,
				SoulGem = 0x40,

				// Patching is a little easier if Disenchant mask fits in 1 byte
				DisenchantSpecial = 0x80,
				EnchantSpecial = 0x100,
				EffectSpecial = 0x200,

				None = 0x0,
				Disenchant = DisenchantWeapon | DisenchantArmor | DisenchantSpecial,
				Item = EnchantWeapon | EnchantArmor | EnchantSpecial,
				Enchantment = EffectWeapon | EffectArmor | EffectSpecial,

				All = EnchantWeapon | DisenchantWeapon | EnchantArmor | DisenchantArmor |
					EffectWeapon | EffectArmor | SoulGem | EnchantSpecial | DisenchantSpecial |
					EffectSpecial,
			};
		};

		bool CanSelectEntry(Menu* a_menu, std::uint32_t a_entry, bool a_showNotification);

		bool HasCompatibleRestrictions(ItemChangeEntry* a_item, EnchantmentEntry* a_effect);

		std::uint16_t GetAmmoEnchantQuantity(Menu* a_menu);
	};
}
