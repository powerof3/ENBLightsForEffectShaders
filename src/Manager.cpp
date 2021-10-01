#include "Manager.h"

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

	if (auto values = ini.GetSection("Blacklist"); values) {
		for (auto& [key, entry] : *values) {
			blacklistedFormIDs.insert(detail::parse_ini(entry));
		}
	}

	ini.SaveFile(path);
}

const std::set<std::variant<RE::TESEffectShader*, const RE::TESFile*>>& Settings::LoadBlacklist()
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	for (auto& [formID, modName] : blacklistedFormIDs) {
		if (modName && !formID) {
			if (const RE::TESFile* filterMod = dataHandler->LookupModByName(*modName); filterMod) {
				blacklistedShaders.insert(filterMod);
			}
		} else if (formID) {
			auto effectShader = modName ?
                                    dataHandler->LookupForm<RE::TESEffectShader>(*formID, *modName) :
                                    RE::TESForm::LookupByID<RE::TESEffectShader>(*formID);
			if (effectShader) {
				blacklistedShaders.insert(effectShader);
			}
		}
	}

	logger::info("blacklist count : {}", blacklistedShaders.size());

	return blacklistedShaders;
}

LightManager::LIGHT LightManager::GetLight(RE::TESEffectShader* a_effectShader)
{
	using Flags = RE::EffectShaderData::Flags;

	const auto has_particle_palette = [&](std::string_view a_path) {
		return string::icontains(a_effectShader->particlePaletteTexture.textureName, a_path);
	};
	const auto has_membrane_palette = [&](std::string_view a_path) {
		return string::icontains(a_effectShader->membranePaletteTexture.textureName, a_path);
	};
	const auto has_particle_shader = [&](std::string_view a_path) {
		return string::icontains(a_effectShader->particleShaderTexture.textureName, a_path);
	};
	const auto has_membrane_shader = [&](std::string_view a_path) {
		return string::icontains(a_effectShader->fillTexture.textureName, a_path);
	};

	for (auto& texture : texture::blacklistedShaders.second) {
		if (has_particle_shader(texture) || has_membrane_shader(texture)) {
			return kNone;
		}
	}

	if (!a_effectShader->particlePaletteTexture.textureName.empty() && a_effectShader->data.flags.all(Flags::kParticleGreyscaleColor)) {
		for (auto& [light, textures] : texture::paletteMap) {
			for (auto& texture : textures) {
				if (has_particle_palette(texture)) {
					return light;
				}
			}
		}
	}

	if (!a_effectShader->particleShaderTexture.textureName.empty()) {
		for (auto& [light, textures] : texture::particleShaderMap) {
			for (auto& texture : textures) {
				if (has_particle_shader(texture)) {
					return light;
				}
			}
		}
	}

	if (!a_effectShader->fillTexture.textureName.empty()) {
		for (auto& [light, textures] : texture::membraneShaderMap) {
			for (auto& texture : textures) {
				if (has_membrane_shader(texture)) {
					auto edgeColor = a_effectShader->data.edgeColor;
					if (!color::is_invalid_color(edgeColor)) {
						return color::get_light_by_color(edgeColor).first;
					}
					return light;
				}
			}
		}
	}

	if (!a_effectShader->membranePaletteTexture.textureName.empty() && a_effectShader->data.flags.all(Flags::kGreyscaleToColor)) {
		for (auto& [light, textures] : texture::paletteMap) {
			for (auto& texture : textures) {
				if (has_membrane_palette(texture)) {
					auto edgeColor = a_effectShader->data.edgeColor;
					if (!color::is_invalid_color(edgeColor)) {
						return color::get_light_by_color(edgeColor).first;
					}
					return light;
				}
			}
		}
	}

	std::array<RE::Color, 3> vec{
		a_effectShader->data.colorKey1,
		a_effectShader->data.colorKey2,
		a_effectShader->data.colorKey3
	};

	if (!color::is_invalid_color(vec)) {
		auto lights = color::get_light_by_color(vec);
		if (lights[0].first != lights[1].first && lights[0].first != lights[2].first && lights[1].first != lights[2].first) {
			//by distance
			auto it = std::ranges::min_element(lights,
				[](const auto& a, const auto& b) {
					return a.second < b.second;
				});
			return it != lights.end() ? it->first : kNone;
		} else {
			//by frequency
			std::unordered_map<LIGHT, std::int32_t> hash;
			for (size_t i = 0; i < lights.size(); i++) {
				hash[lights[i].first]++;
			}

			std::int32_t max_count = -1;
			LIGHT light = kNone;
			for (auto i : hash) {
				if (max_count < i.second) {
					light = i.first;
					max_count = i.second;
				}
			}

			return light;
		}
	}

	auto edgeColor = a_effectShader->data.edgeColor;
	if (!color::is_invalid_color(edgeColor)) {
		return color::get_light_by_color(edgeColor).first;
	}

	return kNone;
}

bool LightManager::ApplyLight(RE::TESEffectShader* a_effectShader)
{
	if (!init) {
		for (auto& [type, path] : nif::map) {
			const auto debrisData = new RE::BGSDebrisData(path.data());
			if (!debrisData) {
				continue;
			}
			const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSDebris>();
			const auto debris = factory ? factory->Create() : nullptr;
			if (!debris) {
				continue;
			}
			debris->data.emplace_front(debrisData);

			debrisMap[type] = debris;
			debrisDataMap[type] = debrisData;
		}
		init = true;
	}

	if (auto light = GetLight(a_effectShader); light != kNone) {
		if (auto& addonModel = a_effectShader->data.addonModels; !addonModel) {
			a_effectShader->data.addonModels = debrisMap[light];
		} else {
			auto debris = addonModel->data.front();
			if (debris && string::icontains(debris->fileName, "enb\\")) {
				return false;
			}
			static auto limit = Settings::GetSingleton()->get_light_limit();
			for (std::uint32_t i = 0; i < limit; i++) {
				addonModel->data.emplace_front(debrisDataMap[light]);
			}
		}
		return true;
	}
	return false;
}
