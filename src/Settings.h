#pragma once

#include "Manager.h"

namespace stl
{
	using FormModPair = std::pair<
		std::optional<RE::FormID>,
		std::optional<std::string>>;
	using FormOrEditorID = std::variant<FormModPair, std::string>;
}

class Settings
{
public:
	using LIGHT = LightManager::LIGHT;

    enum VALID_ACTORS : std::uint32_t
	{
		kPlayer = 0,
		kTeammates = 1,
		kEveryone = 2
	};

	static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	void LoadSettings();
	void LoadOverrideSettings();

	void LoadBlacklist();
	void LoadOverrideShaders();

	bool IsInBlacklist(RE::TESEffectShader* a_effectShader);
	LIGHT GetOverrideLight(RE::TESEffectShader* a_effectShader);

	VALID_ACTORS validActors{ 0 };
	std::uint32_t numTimesApplied{ 1 };

private:
	struct detail
	{
		static stl::FormOrEditorID parse_IDs(const std::string& a_str)
		{
			if (const auto pos = a_str.find("~"sv); pos != std::string::npos || string::icontains(a_str, ".es") || string::is_only_hex(a_str)) {
				if (pos != std::string::npos) {
					auto splitID = string::split(a_str, "~");
					return std::make_pair(
						string::lexical_cast<RE::FormID>(splitID.at(0), true),
						splitID.at(1));
				}
				if (string::icontains(a_str, ".es")) {
					return std::make_pair(
						std::nullopt,
						a_str);
				}
				return std::make_pair(
					string::lexical_cast<RE::FormID>(a_str, true),
					std::nullopt);
			}
			return a_str;
		}

		static std::string compose_IDs(const stl::FormOrEditorID& a_formOrEditorID)
		{
			if (std::holds_alternative<stl::FormModPair>(a_formOrEditorID)) {
				auto& [formID, mod] = std::get<stl::FormModPair>(a_formOrEditorID);
				if (formID && mod) {
					std::stringstream value;
					value << "0x" << std::uppercase << std::hex << *formID;
					return value.str().append("~").append(*mod);
				}
				if (formID) {
					std::stringstream value;
					value << "0x" << std::uppercase << std::hex << *formID;
					return value.str();
				}
				if (mod) {
					return *mod;
				}
				return {};
			}
			return std::get<std::string>(a_formOrEditorID);
		}

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

		static std::pair<stl::FormOrEditorID, LightManager::LIGHT> parse_override_IDs(const std::string& a_str)
		{
			stl::FormOrEditorID ID{};
			LIGHT light{ LIGHT::kNone };

			const auto splitStr = string::split(string::trim_copy(a_str), "|");
			if (splitStr.size() > 1) {
				auto& IDStr = splitStr[0];
				auto& lightStr = splitStr[1];

			    if (const auto pos = IDStr.find("~"sv); pos != std::string::npos || string::is_only_hex(IDStr)) {
					if (pos != std::string::npos) {
						auto splitID = string::split(IDStr, "~");
						ID = std::make_pair(
							string::lexical_cast<RE::FormID>(splitID.at(0), true),
							splitID.at(1));
					} else {
						ID = std::make_pair(
							string::lexical_cast<RE::FormID>(IDStr, true),
							std::nullopt);
					}
				} else {
					ID = IDStr;
				}
				if (const auto it = enumMap.find(lightStr); it != enumMap.end()) {
					light = it->second;
				}
			}

			return { ID, light };
		}
	};

	std::set<std::pair<stl::FormOrEditorID, std::string>> blacklistedIDs{};
	std::set<std::variant<RE::TESEffectShader*, const RE::TESFile*>> blacklistedShaders{};

	std::map<stl::FormOrEditorID, LIGHT> overridenIDs{};
	std::map<RE::TESEffectShader*, LIGHT> overridenShaders{};
};
