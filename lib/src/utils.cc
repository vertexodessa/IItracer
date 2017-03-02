#include "utils.h"
#include "proc_maps.h"
#include "iitracer.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <signal.h>

#include <sstream>

using namespace std;

namespace {
condition_variable gShouldDumpCv;
mutex gShouldDumpMutex;

void SignalHandler(int /*signal*/) __attribute__((no_instrument_function));
void SignalHandler(int /*signal*/) {
    gShouldDumpCv.notify_one();
}
void WaitForDumpSignal()  __attribute__((no_instrument_function));
void WaitForDumpSignal() {
    unique_lock<mutex> lock(gShouldDumpMutex);
    gShouldDumpCv.wait(lock);

    char const *filename = "./_dumped_by_signal.wtf-trace";
    cerr << "Dumping trace data to " << filename << "\n";
    Utils::SaveToFile(filename);
}

vector <ProcMaps::Entry> gLibraryMappings;

} //namespace


namespace Utils {
rwlock gFuncNamesLock;

void SpawnWatcherThread() {
#if defined (BUILD_WITH_EASY_PROFILER)
    ::profiler::setEnabled(true);
#endif
    thread t(WaitForDumpSignal);
    signal(SIGUSR1, SignalHandler);
    t.detach();
}

    void SaveToFile(const char* filename) {
#if defined(WTF_ENABLE)
    ::wtf::Runtime::GetInstance()->SaveToFile(filename);
#else
    // FIXME: most often, this hangs.. easy tracer will wait forever for the main loop to finish.
    auto blocks_count = profiler::dumpBlocksToFile("test.prof");
    std::cout << "Blocks count: " << blocks_count << std::endl;
#endif
}

void EnsureFunctionNameCached(void* caller) {
    {
        std::unique_lock<rwlock> lock(gFuncNamesLock);
        if (FuncNamesMap().find(caller) != FuncNamesMap().end())
            return;
    }

// TEMPORARY!!!! TESTING PURPOSES!
    /// with offset to the library
    {
        intptr_t caller_ = (intptr_t)caller;
        const ProcMaps::Entry& e = ProcMaps::find(caller_);
        // printf("caller: %p, e.start_ %lx\n", caller_, e.start_);
        std::stringstream ss;
        ss << std::hex << caller_ - e.start_;
        std::string result = ss.str();

        std::string s = e.path_ + '@' + ss.str();
        unique_write_lock lock(gFuncNamesLock);
        // Another thread could possibly cache the function name already
        if (FuncNamesMap().find(caller) != FuncNamesMap().end())
            return;
        FuncNamesMap()[caller] = s;
        return;
    }

    /// raw pointer
    {std::string s = std::to_string((intptr_t)caller);
    unique_write_lock lock(gFuncNamesLock);
    // Another thread could possibly cache the function name already
    if (FuncNamesMap().find(caller) != FuncNamesMap().end())
        return;
    FuncNamesMap()[caller] = s;
    return;
    }
// TEMPORARY!!!! TESTING PURPOSES!


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

std::unordered_set<std::string>& NameBlackList() {
    static std::unordered_set<std::string> sSet;
    return sSet;
}

std::unordered_map<std::string, std::queue<ScopedEventPtr>>& ScopedEventsMap() {
    // TODO(vertexodessa): measure timings with rwlock (not thread-local) and compare
    thread_local std::unordered_map<std::string, std::queue<ScopedEventPtr>> *tlMap {nullptr};
    if(!tlMap)
        tlMap = new std::unordered_map<std::string, std::queue<ScopedEventPtr>>();
    return *tlMap;
}

std::unordered_map<void*, std::string>& FuncNamesMap() {
    /* thread_local std::unordered_map<void*, std::string> *tlMap {nullptr}; */
    /* if(!tlMap) */
    /*     tlMap = new std::unordered_map<void*, std::string>(); */
    /* return *tlMap; */

    static std::unordered_map<void*, std::string> sMap;
    return sMap;
}


Event& EnsureEventCached(const char* name) {
    static std::unordered_map<std::string, Event&> sMap;
    std::recursive_mutex m;

    auto it = sMap.find(name);

    if (it != sMap.end())
        return it->second;
    Event* ev = new Event(name);
    std::pair<std::string, Event&> e{name, *ev};
    sMap.emplace(e);

    it = sMap.find(name);
    return it->second;
}

void EnterEventScope(const std::string& funcName, void* /*caller*/) {
#if defined(WTF_ENABLE)
    Event& event = EnsureEventCached(funcName.c_str());
    //Event event{funcName.c_str()};
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
#endif
    /* printf("Entering %s\n", funcName.c_str()); */
    ScopedEventsMap()[funcName].emplace(std::move(s));
}

void LeaveEventScope(const std::string& funcName, void* /*caller*/) {
    /* printf("Leaving %s\n", funcName.c_str()); */
    ScopedEventsMap()[funcName].pop();
}

void MaybeSaveTrace() {
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

bool IsBlackListed(const std::string funcName) {
    return NameBlackList().find(funcName) != NameBlackList().end();
}

} //namespace Utils
