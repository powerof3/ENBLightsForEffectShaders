#include "Hooks.h"
#include "Manager.h"
#include "Settings.h"

namespace ENBLight
{
	struct ShaderReferenceEffect
	{
		struct detail
		{
			static bool is_valid(const RE::Actor* a_user)
			{
				switch (Settings::GetSingleton()->validActors) {
				case Settings::kPlayer:
					return a_user->IsPlayerRef();
				case Settings::kTeammates:
					return a_user->IsPlayerRef() || a_user->IsPlayerTeammate();
				default:
					return true;
				}
			}

			static void CullEffectShader(RE::NiAVObject* a_addon)
			{
				if (a_addon && a_addon->name == ENB_LIGHT_GLOW) {
					const auto user = a_addon->GetUserData();
					const auto actor = user ? user->As<RE::Actor>() : nullptr;

					if (actor && !is_valid(actor)) {
						a_addon->SetAppCulled(true);
					}
				}
			}

			static inline auto ENB_LIGHT_GLOW{ "po3_ENBLightGlow" };
		};

#ifdef SKYRIM_AE  // StartAnimation
		static void thunk(RE::NiAVObject* a_addon)
		{
			detail::CullEffectShader(a_addon);
			func(a_addon);
		}
		static inline REL::Relocation<decltype(thunk)> func;
#else  // AttachAddon
		static void thunk(RE::BSTArray<RE::NiPointer<RE::NiAVObject>>& a_this, RE::NiAVObject* a_addon)
		{
			detail::CullEffectShader(a_addon);
			func(a_this, a_addon);
		}
		static inline REL::Relocation<decltype(thunk)> func;
#endif
	};

	void InstallOnPostLoad()
	{
		const auto settings = Settings::GetSingleton();

		settings->LoadSettings();
		settings->LoadOverrideSettings();

		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(34132, 34934), OFFSET(0x8C2, 0x9F9) };
		stl::write_thunk_call<ShaderReferenceEffect>(target.address());
	}

	void InstallOnDataLoad()
	{
		const auto settings = Settings::GetSingleton();

		settings->LoadBlacklist();
		settings->LoadOverrideShaders();

		constexpr auto can_be_applied_to = [](RE::TESEffectShader* a_effectShader) {
			return a_effectShader->data.flags.none(RE::EffectShaderData::Flags::kDisableParticleShader)

			       && a_effectShader->data.particleShaderPersistantParticleCount > 0 &&

			       !Settings::GetSingleton()->IsInBlacklist(a_effectShader);
		};

		const auto lightManager = LightManager::GetSingleton();

		for (const auto& effectShader : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESEffectShader>()) {
			if (effectShader && can_be_applied_to(effectShader)) {
				lightManager->ApplyLight(effectShader);
			}
		}
	}
}
