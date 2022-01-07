#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <compare>
#include <ranges>
#include <frozen/map.h>
#include <frozen/unordered_map.h>
#include <SimpleIni.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace string = SKSE::stl::string;
namespace numeric = SKSE::stl::numeric;
using namespace std::literals;

namespace stl
{
	using SKSE::stl::adjust_pointer;
	using SKSE::stl::to_underlying;
	using SKSE::stl::report_and_fail;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
}

#include "Version.h"
