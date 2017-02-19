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
// TODO
#endif
using ScopedEventPtr = std::unique_ptr<Scope>;

void SpawnWatcherThread() __attribute__((no_instrument_function));
void SaveToFile(const char* filename) __attribute__((no_instrument_function));

// not modifyable after initialization
inline std::unordered_set<std::string>& NameBlackList() __attribute__((no_instrument_function));
inline std::unordered_set<std::string>& NameBlackList() {
    static std::unordered_set<std::string> sSet;
    return sSet;
}

inline std::unordered_map<std::string, std::queue<ScopedEventPtr>>& ScopedEventsMap() __attribute__((no_instrument_function));
inline std::unordered_map<std::string, std::queue<ScopedEventPtr>>& ScopedEventsMap() {
    // TODO(vertexodessa): measure timings with rwlock (not thread-local) and compare
    thread_local std::unordered_map<std::string, std::queue<ScopedEventPtr>> *tlMap {nullptr};
    if(!tlMap)
        tlMap = new std::unordered_map<std::string, std::queue<ScopedEventPtr>>();
    return *tlMap;
}

inline std::unordered_map<void*, std::string>& FuncNamesMap() __attribute__((no_instrument_function));
inline std::unordered_map<void*, std::string>& FuncNamesMap() {
    /* thread_local std::unordered_map<void*, std::string> *tlMap {nullptr}; */
    /* if(!tlMap) */
    /*     tlMap = new std::unordered_map<void*, std::string>(); */
    /* return *tlMap; */

    static std::unordered_map<void*, std::string> sMap;
    return sMap;
}

inline void EnsureFunctionNameCached(void* caller) __attribute__((no_instrument_function));
inline void EnsureFunctionNameCached(void* caller) {
    {
        std::unique_lock<rwlock> lock(gFuncNamesLock);
        if (FuncNamesMap().find(caller) != FuncNamesMap().end())
            return;
    }
    // FIXME(vertexodessa): handle errors appropriately
    unw_context_t ctx;
    unw_cursor_t c;
    unw_getcontext(&ctx);
    unw_init_local(&c, &ctx);
    unw_step(&c);
    unw_step(&c);

    constexpr int len = 200;
    thread_local char mangled_name[len];
    /*thread_local char* demangled_name = (char*)malloc(len); // LEAKS!!*/
    unw_word_t offset;
    bool unwound = !unw_get_proc_name(&c, mangled_name, len, &offset);

    /*thread_local */size_t result_len = len;
    int status = 0;

    char const* final_name = mangled_name;

    std::string s = std::to_string((intptr_t)caller);
    if (!unwound)
        final_name = s.c_str();
    char* demangled_name = abi::__cxa_demangle(final_name,
                        NULL, NULL,
                        &status);

    result_len = strlen(demangled_name ?: final_name);

    if (demangled_name && !status) {
        final_name = demangled_name;
        std::replace(demangled_name, demangled_name + result_len, ':', '#');
        std::replace(demangled_name, demangled_name + result_len, ',', '.');
    }

    unique_write_lock lock(gFuncNamesLock);

    // Another thread could possibly cache the function name already
    if (FuncNamesMap().find(caller) != FuncNamesMap().end())
        return;

    FuncNamesMap()[caller] = final_name;

    if(demangled_name)
        free(demangled_name);
}

inline void EnterEventScope(const std::string& funcName, void* caller) __attribute__((no_instrument_function));
inline void EnterEventScope(const std::string& funcName, void* caller) {
#if defined(WTF_ENABLE)
    Event event{funcName.c_str()};
    ScopedEventPtr s(new Scope(event));
    s->Enter();
#else
    static std::atomic<uint32_t> a_counter{0};
    uint32_t counter = a_counter.fetch_add(1);
    
    std::string unique_descr("event_");
    unique_descr += counter;
    
    const ::profiler::BaseBlockDescriptor* unique_profiler_descriptor_130 =
        ::profiler::registerDescription(
            profiler::FORCE_ON/*::profiler::extract_enable_flag(profiler::colors::Magenta)*/,
            unique_descr.c_str(),
            funcName.c_str(),
            "event_",
            counter,
            ::profiler::BLOCK_TYPE_BLOCK,
            profiler::colors::Magenta,
            false); // TODO: copy the string (false --> true) ??
    ScopedEventPtr s(new Scope(unique_profiler_descriptor_130 ,""));
    ::profiler::beginBlock(*s);;
    // TODO
#endif
    /* printf("Entering %s\n", funcName.c_str()); */
    ScopedEventsMap()[funcName].emplace(std::move(s));
}

inline void LeaveEventScope(const std::string& funcName, void* /*caller*/) __attribute__((no_instrument_function));
inline void LeaveEventScope(const std::string& funcName, void* /*caller*/) {
    /* printf("Leaving %s\n", funcName.c_str()); */
    
    ScopedEventsMap()[funcName].pop();
}

inline void MaybeSaveTrace() __attribute__((no_instrument_function));
inline void MaybeSaveTrace() {
    static const char* saveInterval = std::getenv("IITRACER_SAVE_INTERVAL");
    constexpr int kDefaultSaveInterval = 1000;
    static int interval =
        (saveInterval) ?
        std::atoi(saveInterval) : kDefaultSaveInterval;

    static std::atomic<int> saveCounter(interval);
    if(!saveCounter.fetch_sub(1)) {
        static std::atomic<int> checkpointIndex(0);
        std::string filename;
        filename = "./autosave." + std::to_string(checkpointIndex.fetch_add(1)) + ".wtf-trace";
        Utils::SaveToFile(filename.c_str());
        saveCounter = interval;
    }
}

inline bool IsBlackListed(const std::string funcName) __attribute__((no_instrument_function));
inline bool IsBlackListed(const std::string funcName) {
    return NameBlackList().find(funcName) != NameBlackList().end();
}

} // namespace Utils

