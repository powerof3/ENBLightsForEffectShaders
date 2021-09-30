#pragma once

#include "colorspace/Comparison.h"

class LightManager
{
public:
	enum LIGHT : std::uint32_t
	{
		kNone,
		kFire,
		kFrost,
		kShock,
		kHeal,
		kDrain,
		kFrenzy,
		kPoison,
		kParalyze,
		kReanimate,
		kShield,
		kSoulTrap,
		kSun,
		kLight,
		kTelekinesis,
		kWard,
		kDetectLife,
		kTurnUndead
	};

	static LightManager* GetSingleton()
	{
		static LightManager singleton;
		return std::addressof(singleton);
	}

	static LIGHT GetLight(RE::TESEffectShader* a_effectShader);
	bool ApplyLight(RE::TESEffectShader* a_effectShader);

private:
	struct color
	{
		using RGB = std::array<std::uint8_t, 3>;

		static constexpr frozen::unordered_map<LIGHT, RGB, 15> map{
			{ kFire, { 251, 162, 96 } },
			{ kFrost, { 128, 177, 209 } },
			{ kShock, { 222, 204, 255 } },
			{ kHeal, { 231, 191, 112 } },
			{ kDrain, { 102, 0, 0 } },
			{ kFrenzy, { 217, 48, 48 } },
			{ kPoison, { 0, 107, 57 } },
			{ kParalyze, { 44, 115, 40 } },
			{ kReanimate, { 88, 117, 218 } },
			{ kShield, { 32, 222, 206 } },
			{ kSoulTrap, { 107, 59, 208 } },
			{ kSun, { 209, 172, 50 } },
			{ kLight, { 248, 248, 255 } },
			{ kTelekinesis, { 172, 90, 40 } },
			{ kWard, { 93, 160, 202 } }
		};

		static bool is_invalid_color(const RE::Color& a_color)
		{
			return (a_color.red == 255 && a_color.green == 255 && a_color.blue == 255) ||
			       (a_color.red == 0 && a_color.green == 0 && a_color.blue == 0) ||
			       (a_color.red == a_color.green && a_color.green == a_color.blue);
		}

		static bool is_invalid_color(const std::array<RE::Color, 3>& a_colors)
		{
			return std::ranges::all_of(a_colors, [](const auto& color) {
				return is_invalid_color(color);
			});
		}

		static std::pair<LIGHT, float> get_light_by_color(const RE::Color& a_color)
		{
			std::map<LIGHT, float> distMap;

			auto shortestDistance = std::numeric_limits<float>::max();

			constexpr auto get_color_distance = [](const RE::Color& a_color, const RGB& a_palette) {
				ColorSpace::Rgb a(a_color.red, a_color.green, a_color.blue);
				ColorSpace::Rgb b(a_palette[0], a_palette[1], a_palette[2]);
				return ColorSpace::Cie2000Comparison::Compare(&a, &b);
			};

			for (auto& [type, color] : color::map) {
				auto distance = static_cast<float>(get_color_distance(a_color, color));
				distMap.emplace(type, distance);

				if (distance < shortestDistance) {
					shortestDistance = distance;
				}
			}

			const auto it = std::ranges::find_if(distMap, [shortestDistance](const auto& mo) {
				return numeric::essentially_equal(mo.second, shortestDistance);
			});
			if (it != distMap.end()) {
				return *it;
			}
			return { LIGHT::kNone, 0.0f };
		}

		static std::array<std::pair<LIGHT, float>, 3> get_light_by_color(const std::array<RE::Color, 3>& a_colors)
		{
			std::array<std::pair<LIGHT, float>, 3> vec;

			for (size_t i = 0; i < 3; i++) {
				vec[i] = get_light_by_color(a_colors[i]);
			}

			return vec;
		}
	};

	struct texture
	{
		static inline std::map<LIGHT, std::set<std::string_view>> paletteMap{
			{ kFire, { "gradalduincracks.dds"sv, "gradalduinsoul.dds"sv, "gradblueflame01.dds"sv, "graddragonholes.dds"sv, "graddragonholesskinbits.dds"sv, "gradfireatronach.dds"sv,
						 "gradfirecloak01.dds"sv, "gradfirecloak02.dds"sv, "gradfiredrauger.dds"sv, "gradfireexplosion.dds"sv, "gradfirestream02.dds"sv, "gradfirestream03.dds"sv, "gradfirestream04.dds"sv,
						 "gradflame01.dds"sv, "gradflameench.dds"sv, "gradflamethin01.dds"sv, "graddragonaspect.dds"sv, "graddragonghostmagic.dds"sv } },

			{ kFrost, { "gradfrosteffectshader.dds"sv, "gradfrostench.dds"sv, "gradfrosticeform.dds"sv, "gradfrosticeform02.dds"sv, "gradfrostrune.dds"sv, "gradfroststream02.dds"sv,
						  "gradicesparkle.dds"sv, "gradicestorm.dds"sv } },

			{ kShock, { "gradshockdisintegrate.dds"sv, "gradshockench.dds"sv, "gradshockencharmor.dds"sv, "gradshockexplosion.dds"sv, "gradshockhit.dds"sv, "gradshockrune.dds"sv } },
			{ kHeal, { "gradflamesovn.dds"sv, "gradhealmagic.dds"sv } },
			{ kDrain, { "gradblood.dds"sv, "gradblood02.dds"sv, "gradredbright.dds"sv, "gradredench.dds"sv } },

			{ kFrenzy, { "gradfireatronachredder.dds"sv, "illusiondark.dds"sv, "gradillusiondark01.dds"sv, "gradillusiondarkshell.dds"sv, "gradillusiondarkthin01.dds"sv,
						   "gradwolftrans01.dds"sv, "gradvampiretrans01.dds"sv } },

			{ kPoison, { "gradbloodinsect.dds"sv, "gradbloodvomitcolor.dds"sv, "gradchaurusspit.dds"sv, "graddarkarmor.dds"sv, "graddisguiseshader01.dds"sv, "graddisguiseshader02.dds"sv,
						   "graddwarvenfiregreenblue.dds"sv, "gradpoisonform.dds"sv, "gradspiderspit.dds"sv, "gradbloodflyer.dds"sv, "gradlurkergoo.dds"sv, "gradsteamthin_poison.dds"sv } },

			{ kParalyze, { "gradalternation01.dds"sv, "gradgreenench.dds"sv, "gradparalyze01.dds"sv, "gradspriggan.dds"sv, "gradspriggangreen.dds"sv, "gradapocrypha.dds"sv,
							 "gradapocryphabook01.dds"sv, "gradapocryphasummonbright.dds"sv, "gradaposummonbright.dds"sv } },

			{ kReanimate, { "gradghostshadernightingale.dds"sv, "gradghostshadernightingale_dark.dds"sv, "gradbloodblackdragonrend.dds"sv, "gradbluelightench.dds"sv,
							  "gradbluestripe.dds"sv, "graddragonrendskin.dds"sv, "gradheal.dds"sv, "gradmuffle.dds"sv, "gradmysthealmagic.dds"sv, "gradnighteye01.dds"sv, "gradreanimate.dds"sv,
							  "gradreanimateench.dds"sv, "graddeaddragonholes.dds"sv, "gradinvis01.dds"sv } },

			{ kShield, { "gradbluefirestream04.dds"sv, "gradmagicanomaly.dds"sv, "gradillusion01.dds"sv, "gradillusion01thin.dds"sv, "gradruneglow.dds"sv, "gradshieldspell.dds"sv,
						   "gradsummonburstliteblue.dds"sv, "gradsummonmagicconduit.dds"sv } },

			{ kSoulTrap, { "gradpurple01.dds"sv, "gradpurple05.dds"sv, "gradpurpleench.dds"sv, "gradsoultrap01.dds"sv,
							 "gradsummonburst.dds"sv, "gradsummonburstblue.dds"sv, "gradsummondepth.dds"sv, "gradsummonedge.dds"sv, "gradviobright.dds"sv, "gradviobrightdark.dds"sv, "gradviobrightinvertalpha.dds"sv,
							 "gradsoulcairnsummonholes.dds"sv } },

			{ kSun, { "gradlightbeam.dds"sv, "gradpurpleflame01.dds"sv } },
			{ kLight, { "gradambfog01.dds"sv, "gradambfogmisty01.dds"sv, "gradcobweb.dds"sv, "gradfogdisssoft.dds"sv, "gradlightspell.dds"sv, "gradshoutselfarea.dds"sv, "gradwisp.dds"sv } },

			{ kTelekinesis, { "graddirt.dds"sv, "graddragondissolvemagic.dds"sv, "graddragonpriestcloth.dds"sv, "gradfrosticeformorange.dds"sv, "gradgreybeardteach.dds"sv,
								"gradlightbeamorange.dds"sv, "gradsmokedark.dds"sv, "gradsprigganmagicorange.dds"sv, "gradsprigganorange.dds"sv, "gradtelekinesis01.dds"sv } },

			{ kWard, { "gradbluebright.dds"sv, "gradghostethershader.dds"sv, "gradghostshader.dds.dds"sv, "gradsummonvalorbright.dds"sv, "gradsummonvalorburst.dds"sv, "gradsummonvalordepth.dds"sv,
						 "gradsummonvaloredge.dds"sv, "gradward.dds"sv, "gradwardflashcolor.dds"sv } },

			{ kDetectLife, { "gradboundweapon.dds"sv } },

			{ kTurnUndead, { "gradturnundedge.dds "sv, "gradturnmagic.dds"sv, "gradbaneofthedead.dds"sv } },
		};

		static inline std::map<LIGHT, std::set<std::string_view>> particleShaderMap{
			{ kFire, { "FireAtlas"sv, "FireBall"sv } },
			{ kFrost, { "IceShards"sv } },
			{ kShield, { "ShieldParticles.dds"sv } }
		};

		static inline std::map<LIGHT, std::set<std::string_view>> membraneShaderMap{
			{ kDrain, { "GhostShaderRedGrad.dds"sv } },
			{ kReanimate, { "GhostShaderVioletGrad.dds"sv, "ghostshadergradsoulcairn.dds" } },
			{ kWard, { "GhostShaderGrad.dds"sv } }
		};

		static inline std::pair<LIGHT, std::set<std::string_view>> blacklistedShaders{
			kNone, { "steam"sv, "blood"sv, "fluid"sv, "dirt"sv, "smokesoft"sv, "dust"sv }
		};
	};

	struct nif
	{
		static constexpr frozen::map<LIGHT, std::string_view, 17> map{
			{ kFire, "effects/enblightforeffectshaders/fire.nif"sv },
			{ kFrost, "effects/enblightforeffectshaders/frost.nif"sv },
			{ kShock, "effects/enblightforeffectshaders/shock.nif"sv },
			{ kHeal, "effects/enblightforeffectshaders/heal.nif"sv },
			{ kDrain, "effects/enblightforeffectshaders/drain.nif"sv },
			{ kFrenzy, "effects/enblightforeffectshaders/frenzy.nif"sv },
			{ kPoison, "effects/enblightforeffectshaders/poison.nif"sv },
			{ kParalyze, "effects/enblightforeffectshaders/paralyze.nif"sv },
			{ kReanimate, "effects/enblightforeffectshaders/reanimate.nif"sv },
			{ kShield, "effects/enblightforeffectshaders/shield.nif"sv },
			{ kSoulTrap, "effects/enblightforeffectshaders/soultrap.nif"sv },
			{ kSun, "effects/enblightforeffectshaders/sun.nif"sv },
			{ kLight, "effects/enblightforeffectshaders/light.nif"sv },
			{ kTelekinesis, "effects/enblightforeffectshaders/telekinesis.nif"sv },
			{ kWard, "effects/enblightforeffectshaders/ward.nif"sv },
			{ kDetectLife, "effects/enblightforeffectshaders/detectlife.nif"sv },
			{ kTurnUndead, "effects/enblightforeffectshaders/turnundead.nif"sv }
		};
	};

	bool init{ false };

	std::map<LIGHT, RE::BGSDebrisData*> debrisDataMap;
	std::map<LIGHT, RE::BGSDebris*> debrisMap;
};

class Settings
{
public:
	using FormIDPair = std::pair<
		std::optional<RE::FormID>,
		std::optional<std::string>>;

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
	const std::set<RE::TESEffectShader*>& LoadBlacklist();

	VALID_ACTORS get_valid_actors()
	{
		return validActors;
	}
	std::uint32_t get_light_limit() { return numTimesApplied; };

private:
	struct detail
	{
		static FormIDPair parse_ini(const std::string& a_str)
		{
			if (a_str.find("~"sv) != std::string::npos) {
				auto splitID = string::split(a_str, "~");
				return std::make_pair(
					string::lexical_cast<RE::FormID>(splitID.at(0), true),
					splitID.at(1));
			} else if (string::icontains(a_str, ".esp") || string::icontains(a_str, ".esl") || string::icontains(a_str, ".esm")) {
				return std::make_pair(
					std::nullopt,
					a_str);
			} else {
				return std::make_pair(
					string::lexical_cast<RE::FormID>(a_str, true),
					std::nullopt);
			}
		}
	};

	VALID_ACTORS validActors{ 0 };
	std::uint32_t numTimesApplied{ 1 };

	std::set<FormIDPair> blacklistedFormIDs;
	std::set<RE::TESEffectShader*> blacklistedShaders;
};
