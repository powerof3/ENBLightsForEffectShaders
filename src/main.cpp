#include "Manager.h"
#include "Settings.h"

namespace ShaderReferenceEffect
{
	struct AddAddonModel
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
		};

		static void thunk(RE::BSTArray<RE::NiPointer<RE::NiAVObject>>& a_this, RE::NiAVObject* a_addon)
		{
			if (a_addon && a_addon->name == "po3_ENBLightGlow") {
				const auto user = a_addon->GetUserData();
				const auto actor = user ? user->As<RE::Actor>() : nullptr;
				if (actor && !detail::is_valid(actor)) {
					a_addon->SetAppCulled(true);
				}
			}

			func(a_this, a_addon);
		}
		static inline REL::Relocation<decltype(&thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(34132) };
		stl::write_thunk_call<AddAddonModel>(target.address() + 0x8C2);
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
		Settings::GetSingleton()->LoadBlacklist();
		Settings::GetSingleton()->LoadOverrideShaders();

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

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "ENB Light For Effect Shaders";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	const auto settings = Settings::GetSingleton();
	settings->LoadSettings();
	settings->LoadOverrideSettings();

	SKSE::AllocTrampoline(14);
	ShaderReferenceEffect::Install();

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
