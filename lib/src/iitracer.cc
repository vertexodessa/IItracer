/*
  ---------------------------------------------------------------------------------
  FIXME(vertexodessa): Consider avoiding using STL in this instrumentation. Which
  might improve performance

  FIXME(vertexodessa): Try to avoid current requirement of
  -finstrument-functions-exclude-file-list=/usr/
  -finstrument-functions-exclude-function-list=static_initialization_and_destruction

  FIXME(vertexodessa): symbol visibility

  ---------------------------------------------------------------------------------
*/

#include "iitracer.h"

#include <algorithm>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define WTF_ENABLE 1
#include <wtf/macros.h>

using namespace std;
namespace {
using Event    = ::wtf::ScopedEventIf<kWtfEnabledForNamespace>;
using Scope    = ::wtf::AutoScopeIf<kWtfEnabledForNamespace>;
using EventPtr = shared_ptr<Event>;
using ScopePtr = unique_ptr<Scope>;

inline unordered_map<string, queue<ScopePtr>>& ScopeMap() __attribute__((no_instrument_function));
inline unordered_map<string, queue<ScopePtr>>& ScopeMap() {
    // TODO(vertexodessa): measure timings with rwlock and compare
    thread_local unordered_map<string, queue<ScopePtr>> tlMap {};
    return tlMap;
}

inline unordered_map<void*, string>& FuncNamesMap() __attribute__((no_instrument_function));
inline unordered_map<void*, string>& FuncNamesMap() {
    // TODO(vertexodessa): measure timings with rwlock and compare
    thread_local unordered_map<void*, string> tlFuncNamesMap {};
    return tlFuncNamesMap;
}

inline void EnsureFunctionNameInCache(void* caller) __attribute__((no_instrument_function));
inline void EnsureFunctionNameInCache(void* caller) {
    if (FuncNamesMap().find(caller) != FuncNamesMap().end())
        return;

    // FIXME(vertexodessa): handle errors appropriately
    unw_context_t ctx;
    unw_cursor_t c;
    unw_getcontext(&ctx);
    unw_init_local(&c, &ctx);
    unw_step(&c);
    unw_step(&c);

    thread_local char name[200];
    unw_word_t offset;
    unw_get_proc_name(&c, name, 200, &offset);

    FuncNamesMap()[caller] = name;
}
}  // namespace

extern "C" {
void __cyg_profile_func_enter(void *func,  void *caller) {
    WTF_AUTO_THREAD_ENABLE();

    EnsureFunctionNameInCache(caller);

    ::wtf::ScopedEventIf<kWtfEnabledForNamespace> __wtf_scope_event0_35{FuncNamesMap()[caller].c_str()};
    ScopePtr s(new Scope(__wtf_scope_event0_35));
    s->Enter();

    ScopeMap()[FuncNamesMap()[caller]].emplace(std::move(s));
}

void __cyg_profile_func_exit(void *func, void *caller) {
    ScopeMap()[FuncNamesMap()[caller]].pop();
}
}  //extern C

void SaveTraceData(const char* filename) {
    ::wtf::Runtime::GetInstance()->SaveToFile(filename);
}
