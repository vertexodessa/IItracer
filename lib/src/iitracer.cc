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

#include "utils.h"

#include <fstream>

#include "rwlock.h"

using namespace std;
using namespace Utils;

extern "C" {
void init() __attribute__((constructor))  __attribute__((no_instrument_function));
void init() {
    cout << "Initializing blacklist\n";
    ifstream infile("./blacklist.txt");
    string line;
    if (!infile) {
        cout << "Failed to open blacklist.txt for reading\n";
    }
    while (getline(infile, line)) {
        cout << "Blacklisting a function: " << line << "\n";
        NameBlackList().insert(line);
    }
    cout << "done initializing\n";
}

void __cyg_profile_func_enter(void */*func*/,  void *caller) {
    static once_flag flag;
    call_once(flag, SpawnWatcherThread);

    WTF_AUTO_THREAD_ENABLE();

    EnsureFunctionNameCached(caller);

    unique_lock<rwlock> lock(gFuncNamesLock);
    const string& funcName = FuncNamesMap()[caller];

    if(IsBlackListed(funcName))
        return;

    MaybeSaveTrace();

    EnterEventScope(funcName, caller);
}

void __cyg_profile_func_exit(void */*func*/, void *caller) {
    unique_lock<rwlock> lock(gFuncNamesLock);
    const string& funcName = FuncNamesMap()[caller];

    if (IsBlackListed(funcName))
        return;

    LeaveEventScope(funcName, caller);
}

void SaveTraceData(const char* filename) {
    SaveToFile(filename);
}
}  //extern C
