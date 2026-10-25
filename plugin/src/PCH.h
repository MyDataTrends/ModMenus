#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

using namespace std::literals;

#define LOG_INFO(...) SKSE::log::info(__VA_ARGS__)
#define LOG_WARN(...) SKSE::log::warn(__VA_ARGS__)
