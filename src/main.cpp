#include "Manager.h"

namespace ShaderReferenceEffect
{
	namespace Add
	{
		//AddAddonModel was inlined away
		struct StartAnimation
		{
			struct detail
			{
				static bool is_valid(RE::Actor* a_user)
				{
					static auto validActors = Settings::GetSingleton()->get_valid_actors();

					switch (validActors) {
					case Settings::kPlayer:
						return a_user->IsPlayerRef();
					case Settings::kTeammates:
						return a_user->IsPlayerRef() || a_user->IsPlayerTeammate();
					default:
						return true;
					}
				};
			};

			static void thunk(RE::NiAVObject* a_addon)
			{
				if (a_addon && a_addon->name == "po3_ENBLightGlow") {
					const auto user = a_addon->GetUserData();
					const auto actor = user ? user->As<RE::Actor>() : nullptr;
					if (actor && !detail::is_valid(actor)) {
						a_addon->SetAppCulled(true);
					}
				}

				func(a_addon);
			}
			static inline REL::Relocation<decltype(&thunk)> func;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x57D7B0) };
			stl::write_thunk_call<StartAnimation>(target.address() + 0x9F9);
		}
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
		auto& blacklistedShaders = Settings::GetSingleton()->LoadBlacklist();

		const auto can_be_applied_to = [blacklistedShaders](RE::TESEffectShader* a_effectShader) {
			return a_effectShader->data.flags.none(RE::EffectShaderData::Flags::kDisableParticleShader)

			       && a_effectShader->data.particleShaderPersistantParticleCount > 0 &&

			       std::find_if(blacklistedShaders.begin(), blacklistedShaders.end(), [&a_effectShader](const auto& formOrFile) {
					   if (std::holds_alternative<RE::TESEffectShader*>(formOrFile)) {
						   auto effectShader = std::get<RE::TESEffectShader*>(formOrFile);
						   return effectShader == a_effectShader;
					   } else if (std::holds_alternative<const RE::TESFile*>(formOrFile)) {
						   auto file = std::get<const RE::TESFile*>(formOrFile);
						   return file && file->IsFormInMod(a_effectShader->GetFormID());
					   }
					   return false;
				   }) == blacklistedShaders.end();
		};

		auto lightManager = LightManager::GetSingleton();
		for (auto& effectShader : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESEffectShader>()) {
			if (effectShader && can_be_applied_to(effectShader)) {
				lightManager->ApplyLight(effectShader);
			}
		}
	}
}

extern "C" __declspec(dllexport) constexpr auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v{};
	v.pluginVersion = Version::MAJOR;
	v.PluginName("ENB Light For Effect Shaders"sv);
	v.AuthorName("powerofthree"sv);
	v.CompatibleVersions({ SKSE::RUNTIME_1_6_318 });
	return v;
}();

bool InitLogger()
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

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	if (!InitLogger()) {
		return false;
	}

	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	Settings::GetSingleton()->LoadSettings();

	SKSE::AllocTrampoline(14);
	ShaderReferenceEffect::Add::Install();

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
