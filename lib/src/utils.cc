#include "utils.h"
#include "iitracer.h"

#include <condition_variable>
#include <mutex>
#include <thread>
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

rwlock Utils::gFuncNamesLock;

void Utils::SpawnWatcherThread() {
    ::profiler::setEnabled(true);; /// ????
    thread t(WaitForDumpSignal);
    signal(SIGUSR1, SignalHandler);
    t.detach();
}

void Utils::SaveToFile(const char* filename) {
#if defined(WTF_ENABLE)
    ::wtf::Runtime::GetInstance()->SaveToFile(filename);
#else
    auto blocks_count = profiler::dumpBlocksToFile("test.prof");
    std::cout << "Blocks count: " << blocks_count << std::endl;
    // TODO
#endif
}
