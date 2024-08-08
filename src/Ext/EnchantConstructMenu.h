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

				// Ammo Specific:
				DisenchantSpecial = 0x80,
				EnchantSpecial = 0x100,
				EffectSpecial = 0x200,

				// Staff Specific:
				FFAimed = 0x400,
				FFSelf = 0x800,
				FFActor = 0x1000,
				FFLocation = 0x2000,
				ConcAimed = 0x4000,
				ConcSelf = 0x8000,
				ConcActor = 0x10000,
				ConcLocation = 0x20000,

				None = 0x0,

				EffectStaff = FFAimed | FFSelf | FFActor | FFLocation | ConcAimed | ConcSelf |
					          ConcActor | ConcLocation,

				Disenchant = DisenchantWeapon | DisenchantArmor | DisenchantSpecial,
				Item = EnchantWeapon | EnchantArmor | EnchantSpecial,
				Enchantment = EffectWeapon | EffectArmor | EffectSpecial | FFAimed | FFSelf |
					FFActor | FFLocation | ConcAimed | ConcSelf | ConcActor | ConcLocation,

				All = EnchantWeapon | DisenchantWeapon | EnchantArmor | DisenchantArmor |
					EffectWeapon | EffectArmor | SoulGem | EnchantSpecial | DisenchantSpecial |
					EffectSpecial | EffectStaff
			};
		};

		bool CanSelectEntry(Menu* a_menu, std::uint32_t a_entry, bool a_showNotification);

		bool HasCompatibleRestrictions(ItemChangeEntry* a_item, EnchantmentEntry* a_effect);

		std::uint16_t GetAmmoEnchantQuantity(Menu* a_menu);
	};
}
