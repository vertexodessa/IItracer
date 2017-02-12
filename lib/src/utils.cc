#include "utils.h"
#include "iitracer.h"

#include <condition_variable>
#include <mutex>
#include <signal.h>

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
} //namespace

void Utils::SpawnWatcherThread() {
    thread t(WaitForDumpSignal);
    signal(SIGUSR1, SignalHandler);
    t.detach();
}

void Utils::SaveToFile(const char* filename) {
#if defined(WTF_ENABLE)
    ::wtf::Runtime::GetInstance()->SaveToFile(filename);
#else
    // TODO
#endif
}
