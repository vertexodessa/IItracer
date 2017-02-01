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
    
    map<string, queue<ScopePtr>> gMap {};
    set<string> gFuncNames {};

    inline std::string getName(void* caller) __attribute__((no_instrument_function));
    inline std::string getName(void* caller)
    {
        std::size_t this_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

        return to_string((intptr_t)caller) + "." + to_string(this_id);
        // unw_context_t ctx;
        // unw_cursor_t c;
        // unw_getcontext(&ctx);
 
        // unw_init_local(&c, &ctx);
        // unw_step(&c);
        // unw_step(&c);

        // char name[300];
        // unw_word_t offset;
        // unw_get_proc_name(&c, name, 200, &offset);


        // std::size_t this_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        // std::string fullname(name);
        // fullname += std::to_string(this_id);
        // return fullname;
    }
}

extern "C" {
void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

int gCanProfile {0};
    
void __cyg_profile_func_enter (void *func,  void *caller)
{
     if(!gCanProfile)
         return;
     WTF_AUTO_THREAD_ENABLE();

     string name(getName(caller));

     // LEAK
     string* nn;
     if(gFuncNames.find(name) != gFuncNames.end())
     {
         nn = (string*)&(*gFuncNames.find(name));
     }
     else
     {
         nn = new string(name);
         gFuncNames.insert(*nn);
     }
     
     ::wtf::ScopedEventIf<kWtfEnabledForNamespace>  __wtf_scope_event0_35{nn->c_str()};
     ScopePtr s(new Scope(__wtf_scope_event0_35));
     s->Enter();

     gMap[name].emplace(std::move(s));
     //cout << ">>>" << gMap[name].size() <<name << '\n';

     //WTF_SCOPE0("aaaaa");

     // wtf::ScopedEventIf<kWtfEnabledForNamespace> event_(name.c_str());
    
     // event_.Enter();
     // event_.Leave();

     // // WTF_SCOPED_EVENT0 :
     // static ::wtf::ScopedEventIf<kWtfEnabledForNamespace>  __wtf_scope_event0_35{"asd"};
     // ::wtf::AutoScopeIf<kWtfEnabledForNamespace>  __wtf_scope0_35{ __wtf_scope_event0_35};
     // __wtf_scope0_35.Enter();
}

void __cyg_profile_func_exit (void *func, void *caller)
{
    if (!gCanProfile)
        return;
    WTF_AUTO_THREAD_ENABLE();
    string name(getName(caller));

    gMap[name].pop();
    //cout << "<<< " << gMap[name].size()<<name << '\n';
}
} //extern C

void bar(void)
{
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
