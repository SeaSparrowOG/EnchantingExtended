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

<<<<<<< HEAD
=======
		bool bAdjustStaffEnchanters;
		bool bStaffChargeEnabled;
>>>>>>> 02cd8f9481aef4abd753ea4f99bf67da4da2c18a
		float fStaffChargeMult;
		float fAmmoChargeMult;
		float fAmmoEffectCostMult;

	private:
		INISettings() = default;
	};
}
