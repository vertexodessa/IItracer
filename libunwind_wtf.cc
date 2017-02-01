#include <stdio.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <vector>
#include <thread>

#define WTF_ENABLE 1
#include <wtf/macros.h>

#include <algorithm>
#include <queue>
#include <map>
#include <set>

using namespace std;
namespace
{
    using Event = ::wtf::ScopedEventIf<kWtfEnabledForNamespace>;
    using EventPtr = shared_ptr<Event>;
    using Scope = ::wtf::AutoScopeIf<kWtfEnabledForNamespace>;
    using ScopePtr = unique_ptr< Scope >;

    // Avoid profiling until STL initialized
    int gCanProfile {0}; 

    inline unordered_map<string, queue<ScopePtr>>& gMap() __attribute__((no_instrument_function));
    inline unordered_map<string, queue<ScopePtr>>& gMap()
    {
        thread_local unordered_map<string, queue<ScopePtr>> tlMap {};
        return tlMap;
    };

    inline map<void*, string>& gFuncNamesMap() __attribute__((no_instrument_function));
    inline map<void*, string>& gFuncNamesMap()
    {
        thread_local map<void*, string> tlFuncNamesMap {};
        return tlFuncNamesMap;
    };

    inline void ensureName(void* caller) __attribute__((no_instrument_function));
    inline void ensureName(void* caller)
    {
        if (gFuncNamesMap().find(caller) != gFuncNamesMap().end())
            return;

        unw_context_t ctx;
        unw_cursor_t c;
        unw_getcontext(&ctx);
        unw_init_local(&c, &ctx);
        unw_step(&c);
        unw_step(&c);

        thread_local char name[300];
        unw_word_t offset;
        unw_get_proc_name(&c, name, 200, &offset);

        gFuncNamesMap()[caller] = name;
    }
}

extern "C" {
void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

void __cyg_profile_func_enter (void *func,  void *caller)
{
     if(!gCanProfile)
         return;
     WTF_AUTO_THREAD_ENABLE();

     ensureName(caller);

     ::wtf::ScopedEventIf<kWtfEnabledForNamespace>  __wtf_scope_event0_35{gFuncNamesMap()[caller].c_str()};
     ScopePtr s(new Scope(__wtf_scope_event0_35));
     s->Enter();

     gMap()[gFuncNamesMap()[caller]].emplace(std::move(s));
}

void __cyg_profile_func_exit (void *func, void *caller)
{
    if (!gCanProfile)
        return;

    WTF_AUTO_THREAD_ENABLE();

    gMap()[gFuncNamesMap()[caller]].pop();
}
} //extern C

void bar(void)
{
    static int count = 10;
    if (--count > 0)
        bar();
}

void foo (void)
{
    int x;
    for (x = 0; x < 4; x++)
        bar ();
}

int main (int argc, char *argv[])
{
    gCanProfile = 1;
    foo ();
    bar ();

    gCanProfile = 0;
    wtf::Runtime::GetInstance()->SaveToFile("tmptestbuf_clearafter.wtf-trace");

    return 0;
}
