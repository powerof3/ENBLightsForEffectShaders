#include "Settings.h"

void Settings::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_ENBLightsForEffectShaders.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, validActors, "Settings", "Valid Actors", ";Display enb lights on effect shaders on these characters.\n;0 - Player Only, 1 - Player and Followers, 2 - Everyone");

	//delete blacklist section and recreate in override_ENBL
	if (auto values = ini.GetSection("Blacklist"); values) {
		for (auto& [key, entry] : *values) {
			blacklistedIDs.emplace(dist::get_record(entry));
			blacklistedIDs_OLD.emplace(key, entry);
		}
		ini.Delete("Blacklist", "EffectShader", true);
	}

	//delete limit
	ini.Delete("Settings", "ENB Light Limit");

	ini.SaveFile(path);
}

void Settings::LoadOverrideSettings()
{
	std::vector<std::string> configs = clib_util::distribution::get_configs("Data\\"sv, "_ENBL"sv);

	if (configs.empty()) {
		logger::warn("No .ini files with _ENBL suffix were found within the Data folder. Overrides will not be loaded");
		return;
	}

	logger::info("{} matching inis found", configs.size());

	for (auto& path : configs) {
		logger::info("\tINI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("\tcouldn't read INI");
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
				blacklistedIDs.emplace(dist::get_record(entry));
			}
		}
	}
}

void get_merged_IDs(std::optional<RE::FormID>& a_formID, std::optional<std::string>& a_modName)
{
	const auto [mergedModName, mergedFormID] = g_mergeMapperInterface->GetNewFormID(a_modName.value_or("").c_str(), a_formID.value_or(0));
	std::string conversion_log{};
	if (a_formID.value_or(0) && mergedFormID && a_formID.value_or(0) != mergedFormID) {
		conversion_log = std::format("0x{:X}->0x{:X}", a_formID.value_or(0), mergedFormID);
		a_formID.emplace(mergedFormID);
	}
	const std::string mergedModString{ mergedModName };
	if (!a_modName.value_or("").empty() && !mergedModString.empty() && a_modName.value_or("") != mergedModString) {
		if (conversion_log.empty()) {
			conversion_log = std::format("{}->{}", a_modName.value_or(""), mergedModString);
		} else {
			conversion_log = std::format("{}~{}->{}", conversion_log, a_modName.value_or(""), mergedModString);
		}
		a_modName.emplace(mergedModName);
	}
	if (!conversion_log.empty()) {
		logger::info("\t\tFound merged: {}", conversion_log);
	}
}

void Settings::LoadBlacklist()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	logger::info("loading blacklist");

	for (const auto& ID : blacklistedIDs) {
		std::visit(overload{
					   [&](dist::formid_pair formMod) {
						   auto& [formID, modName] = formMod;
						   if (formID && g_mergeMapperInterface) {
							   get_merged_IDs(formID, modName);
						   }
						   if (modName && (!formID || *formID == 0)) {
							   if (const RE::TESFile* filterMod = dataHandler->LookupModByName(*modName); filterMod) {
								   blacklistedShaders.insert(filterMod);
							   } else {
								   logger::warn("\tmod ({}) not found", *modName);
							   }
						   } else if (formID) {
							   auto effectShader = modName ?
				                                       dataHandler->LookupForm<RE::TESEffectShader>(*formID, *modName) :
				                                       RE::TESForm::LookupByID<RE::TESEffectShader>(*formID);
							   if (effectShader) {
								   blacklistedShaders.insert(effectShader);
							   } else {
								   logger::warn("\teffect shader (0x{:X}~{}) not found", *formID, modName ? *modName : "");
							   }
						   }
					   },
					   [&](std::string editorID) {
						   if (auto effectShader = RE::TESForm::LookupByEditorID<RE::TESEffectShader>(editorID); effectShader) {
							   blacklistedShaders.insert(effectShader);
						   } else {
							   logger::warn("\teffect shader ({}) not found", editorID);
						   }
					   } },
			ID);
	}

	logger::info("blacklist count : {}/{}", blacklistedShaders.size(), blacklistedIDs.size());
}

void Settings::LoadOverrideShaders()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	logger::info("loading override list");

	for (auto& [ID, light] : overridenIDs) {
		std::visit(overload{
					   [&](dist::formid_pair formMod) {
						   auto& [formID, modName] = formMod;
						   if (formID && g_mergeMapperInterface) {
							   get_merged_IDs(formID, modName);
						   }
						   auto effectShader = formID ?
			                                       modName ? dataHandler->LookupForm<RE::TESEffectShader>(*formID, *modName) :
			                                                 RE::TESForm::LookupByID<RE::TESEffectShader>(*formID) :
			                                       nullptr;
						   if (effectShader) {
							   overridenShaders.emplace(effectShader, light);
						   } else {
							   logger::warn("\teffect shader (0x{:X}~{}) not found", *formID, modName ? *modName : "");
						   }
					   },
					   [&](std::string editorID) {
						   if (auto effectShader = RE::TESForm::LookupByEditorID<RE::TESEffectShader>(editorID); effectShader) {
							   overridenShaders.emplace(effectShader, light);
						   } else {
							   logger::warn("\teffect shader ({}) not found", editorID);
						   }
					   } },
			ID);
	}

	logger::info("override shader count : {}/{}", overridenShaders.size(), overridenIDs.size());
}

bool Settings::IsInBlacklist(RE::TESEffectShader* a_effectShader)
{
	return std::ranges::find_if(blacklistedShaders, [&a_effectShader](const auto& formOrFile) {
		if (std::holds_alternative<RE::TESEffectShader*>(formOrFile)) {
			return std::get<RE::TESEffectShader*>(formOrFile) == a_effectShader;
		}
		if (std::holds_alternative<const RE::TESFile*>(formOrFile)) {
			auto file = std::get<const RE::TESFile*>(formOrFile);
			return file && file->IsFormInMod(a_effectShader->GetFormID());
		}
		return false;
	}) != blacklistedShaders.end();
}

LIGHT Settings::GetOverrideLight(RE::TESEffectShader* a_effectShader)
{
	const auto it = overridenShaders.find(a_effectShader);
	return it != overridenShaders.end() ? it->second : LIGHT::kNone;
}
