/*
  ---------------------------------------------------------------------------------
  FIXME: Consider avoiding using STL in this instrumentation. Which
  might improve performance

  FIXME: Try to avoid current requirement of
  -finstrument-functions-exclude-file-list=/usr/
  -finstrument-functions-exclude-function-list=static_initialization_and_destruction

  FIXME: symbol visibility

  ---------------------------------------------------------------------------------
*/

#include "libunwind_wtf.h"

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

using namespace std;
namespace
{
using Event    = ::wtf::ScopedEventIf<kWtfEnabledForNamespace>;
using Scope    = ::wtf::AutoScopeIf<kWtfEnabledForNamespace>;
using EventPtr = shared_ptr<Event>;
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

extern "C"
{
void __cyg_profile_func_enter (void *func,  void *caller)
{
    WTF_AUTO_THREAD_ENABLE();

    ensureFunctionName(caller);

    ::wtf::ScopedEventIf<kWtfEnabledForNamespace> __wtf_scope_event0_35{gFuncNamesMap()[caller].c_str()};
    ScopePtr s(new Scope(__wtf_scope_event0_35));
    s->Enter();

    gMap()[gFuncNamesMap()[caller]].emplace(std::move(s));
}

void __cyg_profile_func_exit (void *func, void *caller)
{
    gMap()[gFuncNamesMap()[caller]].pop();
}
} //extern C

void saveProfiling(const char* filename)
{
    ::wtf::Runtime::GetInstance()->SaveToFile(filename);
}
