#pragma once

#include "Manager.h"

class Settings : public ISingleton<Settings>
{
public:
	enum VALID_ACTORS : std::uint32_t
	{
		kPlayer = 0,
		kTeammates = 1,
		kEveryone = 2
	};

	void LoadSettings();
	void LoadOverrideSettings();

	void LoadBlacklist();
	void LoadOverrideShaders();

	bool IsInBlacklist(RE::TESEffectShader* a_effectShader);
	LIGHT GetOverrideLight(RE::TESEffectShader* a_effectShader);

	VALID_ACTORS validActors{ 0 };

private:
	struct detail
	{
		static inline frozen::map<std::string_view, LIGHT, 18> enumMap{
			{ "None"sv, LIGHT::kNone },
			{ "Fire"sv, LIGHT::kFire },
			{ "Frost"sv, LIGHT::kFrost },
			{ "Shock"sv, LIGHT::kShock },
			{ "Heal"sv, LIGHT::kHeal },
			{ "Drain"sv, LIGHT::kDrain },
			{ "Frenzy"sv, LIGHT::kFrenzy },
			{ "Poison"sv, LIGHT::kPoison },
			{ "Paralyze"sv, LIGHT::kParalyze },
			{ "Reanimate"sv, LIGHT::kReanimate },
			{ "Shield"sv, LIGHT::kShield },
			{ "SoulTrap"sv, LIGHT::kSoulTrap },
			{ "Sun"sv, LIGHT::kSun },
			{ "Light"sv, LIGHT::kLight },
			{ "Telekinesis"sv, LIGHT::kTelekinesis },
			{ "Ward"sv, LIGHT::kWard },
			{ "Detect Life"sv, LIGHT::kDetectLife },
			{ "Turn Undead"sv, LIGHT::kTurnUndead }
		};

		static std::pair<dist::record, LIGHT> parse_override_IDs(const std::string& a_str)
		{
			dist::record ID{};
			LIGHT light{ LIGHT::kNone };

			if (const auto splitStr = string::split(string::trim_copy(a_str), "|"); splitStr.size() > 1) {
				ID = dist::get_record(splitStr[0]);
				if (const auto it = enumMap.find(splitStr[1]); it != enumMap.end()) {
					light = it->second;
				}
			}

			return { ID, light };
		}
	};

	std::multimap<CSimpleIniA::Entry, std::string, CSimpleIniA::Entry::LoadOrder> blacklistedIDs_OLD{};
	std::set<dist::record> blacklistedIDs{};

	std::set<std::variant<RE::TESEffectShader*, const RE::TESFile*>> blacklistedShaders{};

	std::map<dist::record, LIGHT> overridenIDs{};
	std::map<RE::TESEffectShader*, LIGHT> overridenShaders{};
};
