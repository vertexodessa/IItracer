#pragma once

#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <string.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#if !defined(USE_EASY_PROFILER)
#define WTF_ENABLE 1
#define WTF_PTHREAD_THREADED 1
#endif

#if defined(WTF_ENABLE)
#include <wtf/macros.h>
#define IITRACER_AUTO_THREAD_ENABLE() WTF_AUTO_THREAD_ENABLE()
#else
#define BUILD_WITH_EASY_PROFILER 1
#include <easy/profiler.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

inline uint32_t getCurrentThreadId()
{
#ifdef _WIN32
    return (uint32_t)::GetCurrentThreadId();
#else
    EASY_THREAD_LOCAL static const pid_t x = syscall(__NR_gettid);
    EASY_THREAD_LOCAL static const uint32_t _id = (uint32_t)x;//std::hash<std::thread::id>()(std::this_thread::get_id());
    return _id;
#endif
}

#define IITRACER_AUTO_THREAD_ENABLE() \
    static std::string _____S___(std::to_string(getCurrentThreadId())); \
    EASY_THREAD(_____S___.c_str())// FIXME!!!!!
// TODO
#endif

#include <cctype>

#include <cxxabi.h>

#include "rwlock.h"

namespace Utils {

extern rwlock gFuncNamesLock;

#if defined(WTF_ENABLE)
using Event    = ::wtf::ScopedEventIf<kWtfEnabledForNamespace>;
using Scope    = ::wtf::AutoScopeIf<kWtfEnabledForNamespace>;
#else
using Event    = ::profiler::BaseBlockDescriptor;
using Scope    = ::profiler::Block;
#endif
using ScopedEventPtr = std::unique_ptr<Scope>;

void SpawnWatcherThread();// __attribute__((no_instrument_function));
void SaveToFile(const char* filename);// __attribute__((no_instrument_function));

std::unordered_map<std::string, std::queue<ScopedEventPtr>>& ScopedEventsMap();// __attribute__((no_instrument_function));
std::unordered_map<void*, std::string>& FuncNamesMap();// __attribute__((no_instrument_function));
// not modifyable after initialization
std::unordered_set<std::string>& NameBlackList();// __attribute__((no_instrument_function));

void EnsureFunctionNameCached(void* caller);// __attribute__((no_instrument_function));
Event& EnsureEventCached(const char* name);

void EnterEventScope(const std::string& funcName, void* /*caller*/);// __attribute__((no_instrument_function));
void LeaveEventScope(const std::string& funcName, void* /*caller*/);// __attribute__((no_instrument_function));

void MaybeSaveTrace();// __attribute__((no_instrument_function));

bool IsBlackListed(const std::string funcName);// __attribute__((no_instrument_function));

} // namespace Utils

