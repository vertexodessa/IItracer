#include <algorithm>
#include <queue>
#include <map>
#include <set>
#include <vector>
#include <thread>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define WTF_ENABLE 1
#include <wtf/macros.h>

// not needed for profiling to function. Just for main().
#include <chrono>

using namespace std;
namespace
{
using Event    = ::wtf::ScopedEventIf<kWtfEnabledForNamespace>;
using EventPtr = shared_ptr<Event>;
using Scope    = ::wtf::AutoScopeIf<kWtfEnabledForNamespace>;
using ScopePtr = unique_ptr<Scope>;

inline unordered_map<string, queue<ScopePtr>>& gMap() __attribute__((no_instrument_function));
inline unordered_map<string, queue<ScopePtr>>& gMap()
{
    // TODO: measure timings with rwlock and compare
    thread_local unordered_map<string, queue<ScopePtr>> tlMap {};
    return tlMap;
};

inline map<void*, string>& gFuncNamesMap() __attribute__((no_instrument_function));
inline map<void*, string>& gFuncNamesMap()
{
    // TODO: measure timings with rwlock and compare
    thread_local map<void*, string> tlFuncNamesMap {};
    return tlFuncNamesMap;
};

inline void ensureFunctionName(void* caller) __attribute__((no_instrument_function));
inline void ensureFunctionName(void* caller)
{
    if (gFuncNamesMap().find(caller) != gFuncNamesMap().end())
        return;

    // FIXME: handle errors appropriately
    unw_context_t ctx;
    unw_cursor_t c;
    unw_getcontext(&ctx);
    unw_init_local(&c, &ctx);
    unw_step(&c);
    unw_step(&c);

    thread_local char name[200];
    unw_word_t offset;
    unw_get_proc_name(&c, name, 200, &offset);

    gFuncNamesMap()[caller] = name;
}
}

extern "C" {
    void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
    void __cyg_profile_func_enter (void *func,  void *caller)
    {
        WTF_AUTO_THREAD_ENABLE();

        ensureFunctionName(caller);

        ::wtf::ScopedEventIf<kWtfEnabledForNamespace> __wtf_scope_event0_35{gFuncNamesMap()[caller].c_str()};
        ScopePtr s(new Scope(__wtf_scope_event0_35));
        s->Enter();

        gMap()[gFuncNamesMap()[caller]].emplace(std::move(s));
    }

    void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));
    void __cyg_profile_func_exit (void *func, void *caller)
    {
        gMap()[gFuncNamesMap()[caller]].pop();
    }
} //extern C

void bar()
{
    static atomic<int> count {1000};
    if (--count > 0)
        bar();
}

void foo()
{
    int x;
    for(x = 0; x < 4; x++)
        bar ();
}

// FIXME: move main() to a separate file
int main (int argc, char *argv[])
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    vector<thread> v;
    for(int i=0; i<10; ++i)
        v.emplace_back(foo);

    foo();
    bar();

    for_each(v.begin(), v.end(), [](thread& f) { f.join(); });

    std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<std::endl;

    ::wtf::Runtime::GetInstance()->SaveToFile("tmptestbuf_clearafter.wtf-trace");

    return 0;
}
