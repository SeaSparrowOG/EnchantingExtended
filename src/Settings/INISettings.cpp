#include "INISettings.h"

namespace Settings
{
	INISettings* INISettings::GetSingleton()
	{
		static INISettings singleton{};
		return &singleton;
	}

	void INISettings::LoadSettings()
	{
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)", Version::NAME).c_str());

<<<<<<< HEAD
=======
		bAdjustStaffEnchanters = static_cast<bool>(
			ini.GetBoolValue("StaffEnchanting", "bAdjustStaffEnchanters", false));

		bStaffChargeEnabled = static_cast<bool>(
			ini.GetBoolValue("StaffEnchanting", "bStaffChargeEnabled", false));

>>>>>>> 02cd8f9481aef4abd753ea4f99bf67da4da2c18a
		fStaffChargeMult = static_cast<float>(
			ini.GetDoubleValue("StaffEnchanting", "fStaffChargeMult", 10.0));

		fAmmoChargeMult = static_cast<float>(
			ini.GetDoubleValue("AmmoEnchanting", "fAmmoChargeMult", 0.2));

		fAmmoEffectCostMult = static_cast<float>(
			ini.GetDoubleValue("AmmoEnchanting", "fAmmoEffectCostMult", 0.45));
	}
}
