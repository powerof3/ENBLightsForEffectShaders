#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <compare>

#include <MergeMapperPluginAPI.h>
#include <frozen/map.h>
#include <frozen/unordered_map.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#include <ClibUtil/SimpleINI.hpp>
#include <ClibUtil/distribution.hpp>
#include <ClibUtil/editorID.hpp>
#include <ClibUtil/numeric.hpp>
#include <ClibUtil/singleton.hpp>
#include <ClibUtil/string.hpp>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace numeric = clib_util::numeric;
namespace string = clib_util::string;
namespace ini = clib_util::ini;
namespace dist = clib_util::distribution;

using namespace std::literals;
using namespace string::literals;
using namespace clib_util::singleton;

// for visting variants
template <class... Ts>
struct overload : Ts...
{
	using Ts::operator()...;
};

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
}

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#elif SKYRIMVR
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#else
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#endif

#include "Version.h"
