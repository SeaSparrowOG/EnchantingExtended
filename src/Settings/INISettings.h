#pragma once

namespace Settings
{
	class INISettings final
	{
	public:
		static INISettings* GetSingleton();

		~INISettings() = default;
		INISettings(const INISettings&) = delete;
		INISettings(INISettings&&) = delete;
		INISettings& operator=(const INISettings&) = delete;
		INISettings& operator=(INISettings&&) = delete;

		void LoadSettings();

		bool bAdjustStaffEnchanters;
		bool bStaffChargeEnabled;
		bool bEnableMultiEnchantments;
		bool bUseSoulGemsForStaves;
		float fStaffChargeMult;
		float fAmmoChargeMult;
		float fAmmoEffectCostMult;

	private:
		INISettings() = default;
	};
}
