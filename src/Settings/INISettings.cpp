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

		bAdjustStaffEnchanters = static_cast<bool>(
			ini.GetBoolValue("StaffEnchanting", "bAdjustStaffEnchanters", false));

		bStaffChargeEnabled = static_cast<bool>(
			ini.GetBoolValue("StaffEnchanting", "bStaffChargeEnabled", false));

		bEnableMultiEnchantments = static_cast<bool>(
			ini.GetBoolValue("StaffEnchanting", "bEnableMultiEnchantments", false));

		fStaffChargeMult = static_cast<float>(
			ini.GetDoubleValue("StaffEnchanting", "fStaffChargeMult", 10.0));

		fAmmoChargeMult = static_cast<float>(
			ini.GetDoubleValue("AmmoEnchanting", "fAmmoChargeMult", 0.2));

		fAmmoEffectCostMult = static_cast<float>(
			ini.GetDoubleValue("AmmoEnchanting", "fAmmoEffectCostMult", 0.45));
	}
}
