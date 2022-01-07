#include "Manager.h"
#include "Settings.h"

LightManager::LIGHT LightManager::GetLight(const RE::TESEffectShader* a_effectShader)
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
					const auto& edgeColor = a_effectShader->data.edgeColor;
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
					const auto& edgeColor = a_effectShader->data.edgeColor;
					if (!color::is_invalid_color(edgeColor)) {
						return color::get_light_by_color(edgeColor).first;
					}
					return light;
				}
			}
		}
	}

	const std::array vec{
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
		}
		//by frequency
		std::unordered_map<LIGHT, std::int32_t> hash;
		for (auto& light : lights | std::views::keys) {
			hash[light]++;
		}

		std::int32_t max_count = -1;
		LIGHT finalLight = kNone;
		for (const auto& [light, count] : hash) {
			if (max_count < count) {
				finalLight = light;
				max_count = count;
			}
		}
		return finalLight;
	}

	const auto& edgeColor = a_effectShader->data.edgeColor;
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

	auto light = Settings::GetSingleton()->GetOverrideLight(a_effectShader);
	if (light == kNone) {
		light = GetLight(a_effectShader);
	}

	if (light != kNone) {
		if (const auto& addonModel = a_effectShader->data.addonModels; !addonModel) {
			a_effectShader->data.addonModels = debrisMap[light];
		} else {
			const auto debris = addonModel->data.front();
			if (debris && string::icontains(debris->fileName, "enb\\")) {
				return false;
			}
			static auto limit = Settings::GetSingleton()->numTimesApplied;
			for (std::uint32_t i = 0; i < limit; i++) {
				addonModel->data.emplace_front(debrisDataMap[light]);
			}
		}
		return true;
	}
	return false;
}
