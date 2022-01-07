#include "Settings.h"

#include <ranges>

void Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_ENBLightsForEffectShaders.ini";

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	ini.LoadFile(path);

	validActors = string::lexical_cast<VALID_ACTORS>(ini.GetValue("Settings", "Valid Actors", "0"));
	ini.SetValue("Settings", "Valid Actors", std::to_string(validActors).c_str(), ";Display enb lights on effect shaders on these characters.\n;0 - Player Only, 1 - Player And Followers, 2 - Everyone", true);

	numTimesApplied = string::lexical_cast<std::uint32_t>(ini.GetValue("Settings", "ENB Light Limit", "2"));
	ini.SetValue("Settings", "ENB Light Limit", std::to_string(numTimesApplied).c_str(), ";Number of ENB light models to be spawned per effect shader.", true);

	//delete blacklist section and recreate in override_ENBL
	if (auto values = ini.GetSection("Blacklist"); values) {
		for (auto& [key, entry] : *values) {
		    blacklistedIDs.emplace(detail::parse_IDs(entry));
			blacklistedIDs_OLD.emplace(key, entry);
		}
		ini.Delete("Blacklist", "EffectShader", true);
	}

	ini.SaveFile(path);
}

void Settings::LoadOverrideSettings()
{
	std::vector<std::string> configs;

	auto constexpr folder = R"(Data\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
			if (const auto path = entry.path().string(); path.find("_ENBL") != std::string::npos) {
				configs.push_back(path);
			}
		}
	}

	if (configs.empty()) {
		logger::warn("No .ini files with _ENBL suffix were found within the Data folder. Overrides will not be loaded");
		return;
	}

	logger::info("{} matching inis found", configs.size());

	for (auto& path : configs) {
		logger::info("	INI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("	couldn't read INI");
			continue;
		}

		if (auto values = ini.GetSection("Override"); values) {
			for (auto& [key, entry] : *values) {
				overridenIDs.emplace(detail::parse_override_IDs(entry));
			}
		}

		if (path.find("Overrides_ENBL") != std::string::npos && !blacklistedIDs_OLD.empty()) {
			for (auto& [key, entry] : blacklistedIDs_OLD) {
				ini.DeleteValue("Blacklist", key.pItem, entry.c_str());
				ini.SetValue("Blacklist", key.pItem, entry.c_str(), key.pComment, false);
			}
			ini.SaveFile(path.c_str());
		}

		if (auto values = ini.GetSection("Blacklist"); values) {
			for (const auto& entry : *values | std::views::values) {
				blacklistedIDs.emplace(detail::parse_IDs(entry));
			}
		}
	}
}

void Settings::LoadBlacklist()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	logger::info("loading blacklist");

	for (const auto& ID : blacklistedIDs) {
		if (std::holds_alternative<stl::FormModPair>(ID)) {
			auto& [formID, modName] = std::get<stl::FormModPair>(ID);
			if (modName && !formID) {
				if (const RE::TESFile* filterMod = dataHandler->LookupModByName(*modName); filterMod) {
					blacklistedShaders.insert(filterMod);
				} else {
					logger::warn("	mod ({}) not found", *modName);
				}
			} else if (formID) {
				auto effectShader = modName ?
                                        dataHandler->LookupForm<RE::TESEffectShader>(*formID, *modName) :
                                        RE::TESForm::LookupByID<RE::TESEffectShader>(*formID);
				if (effectShader) {
					blacklistedShaders.insert(effectShader);
				} else {
					logger::warn("	effect shader (0x{:X}~{}) not found", *formID, *modName);
				}
			}
		} else {
			const auto editorID = std::get<std::string>(ID);
			if (auto effectShader = RE::TESForm::LookupByEditorID<RE::TESEffectShader>(editorID); effectShader) {
				blacklistedShaders.insert(effectShader);
			} else {
				logger::warn("	effect shader ({}) not found", editorID);
			}
		}
	}

	logger::info("blacklist count : {}/{}", blacklistedShaders.size(), blacklistedIDs.size());
}

void Settings::LoadOverrideShaders()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	logger::info("loading override list");

    for (auto& [ID, light] : overridenIDs) {
		if (std::holds_alternative<stl::FormModPair>(ID)) {
			if (auto& [formID, modName] = std::get<stl::FormModPair>(ID); formID) {
				auto effectShader = modName ?
                                        dataHandler->LookupForm<RE::TESEffectShader>(*formID, *modName) :
                                        RE::TESForm::LookupByID<RE::TESEffectShader>(*formID);
				if (effectShader) {
					overridenShaders.emplace(effectShader, light);
				} else {
					logger::warn("	effect shader (0x{:X}~{}) not found", *formID, *modName);
				}
			}
		} else {
			const auto editorID = std::get<std::string>(ID);
			if (auto effectShader = RE::TESForm::LookupByEditorID<RE::TESEffectShader>(editorID); effectShader) {
				overridenShaders.emplace(effectShader, light);
			} else {
				logger::warn("	effect shader ({}) not found", editorID);
			}
		}
	}

	logger::info("override shader count : {}/{}", overridenShaders.size(), overridenIDs.size());
}

bool Settings::IsInBlacklist(RE::TESEffectShader* a_effectShader)
{
	return std::ranges::find_if(blacklistedShaders, [&a_effectShader](const auto& formOrFile) {
		if (std::holds_alternative<RE::TESEffectShader*>(formOrFile)) {
			auto effectShader = std::get<RE::TESEffectShader*>(formOrFile);
			return effectShader == a_effectShader;
		}
		if (std::holds_alternative<const RE::TESFile*>(formOrFile)) {
			auto file = std::get<const RE::TESFile*>(formOrFile);
			return file && file->IsFormInMod(a_effectShader->GetFormID());
		}
		return false;
	}) != blacklistedShaders.end();
}

Settings::LIGHT Settings::GetOverrideLight(RE::TESEffectShader* a_effectShader)
{
	const auto it = overridenShaders.find(a_effectShader);
	return it != overridenShaders.end() ? it->second : LIGHT::kNone;
}
