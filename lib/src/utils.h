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

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define WTF_ENABLE 1
#define WTF_PTHREAD_THREADED 1
#if defined(WTF_ENABLE)
#include <wtf/macros.h>
#else
// TODO
#endif

#include <cxxabi.h>

namespace Utils {


#if defined(WTF_ENABLE)
using Event    = ::wtf::ScopedEventIf<kWtfEnabledForNamespace>;
using Scope    = ::wtf::AutoScopeIf<kWtfEnabledForNamespace>;
#else
// TODO
#endif
using EventPtr = std::shared_ptr<Event>;
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
    // TODO(vertexodessa): measure timings with rwlock and compare
    thread_local std::unordered_map<void*, std::string> *tlFuncNamesMap {nullptr};
    if(!tlFuncNamesMap)
        tlFuncNamesMap = new std::unordered_map<void*, std::string>();
    return *tlFuncNamesMap;
}

inline void EnsureFunctionNameCached(void* caller) __attribute__((no_instrument_function));
inline void EnsureFunctionNameCached(void* caller) {
    if (FuncNamesMap().find(caller) != FuncNamesMap().end())
        return;

    // FIXME(vertexodessa): handle errors appropriately
    unw_context_t ctx;
    unw_cursor_t c;
    unw_getcontext(&ctx);
    unw_init_local(&c, &ctx);
    unw_step(&c);
    unw_step(&c);

    constexpr int len = 200;
    thread_local char mangled_name[len], demangled_name[len];
    unw_word_t offset;
    unw_get_proc_name(&c, mangled_name, len, &offset);

    size_t result_len = len;
    int status=0;

    char const* final_name = mangled_name;
    abi::__cxa_demangle(mangled_name,
                        demangled_name, &result_len,
                        &status);

    if (!status) {
        final_name = demangled_name;
        std::replace(std::begin(demangled_name), std::end(demangled_name), ':', '#');
    }
    if (!final_name)
        final_name = mangled_name;
    if (!final_name)
        final_name = "Unknown";

    FuncNamesMap()[caller] = final_name;
}

inline void EnterEventScope(const std::string& funcName, void* caller) __attribute__((no_instrument_function));
inline void EnterEventScope(const std::string& funcName, void* caller) {
#if defined(WTF_ENABLE)
    ::wtf::ScopedEventIf<kWtfEnabledForNamespace> __wtf_scope_event0_35{funcName.c_str()};
    ScopedEventPtr s(new Scope(__wtf_scope_event0_35));
    s->Enter();
#else
    // TODO
#endif

    ScopedEventsMap()[FuncNamesMap()[caller]].emplace(move(s));
}

inline void LeaveEventScope(const std::string& funcName, void* /*caller*/) __attribute__((no_instrument_function));
inline void LeaveEventScope(const std::string& funcName, void* /*caller*/) {
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

