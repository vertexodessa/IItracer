#include "iitracer.h"
#include <atomic>
#include <thread>
#include <vector>
#include <algorithm>
#include <iostream>

#include <unistd.h>

#define SAVE_TRACE_ON_EXIT
// #define SLEEP_UNTIL_SIGNAL

using namespace std;

class A {
public:
    void TestMethod() {
        //printf("%s called\n", __PRETTY_FUNCTION__);
    };
};

void baz() {
    A a;
    a.TestMethod();
    //usleep(50);
}

void bar() {
    static atomic<int> count {2000};
    baz();
    if (--count > 0)
        bar();
}

void foo() {
    int x;
    for(x = 0; x < 4; x++)
        bar ();
}

int main (int /*argc*/, char** /*argv*/) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    vector<thread> v;
    for(int i=0; i<10; ++i)
        v.emplace_back(foo);

    foo();
    bar();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    for_each(v.begin(), v.end(), [](thread& f) {
#if defined (SLEEP_UNTIL_SIGNAL)
            f.detach();
#else
            f.join();
#endif
        });

#if defined (SLEEP_UNTIL_SIGNAL)
    sleep(100000);
#endif
    std::cerr << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << " microseconds" << std::endl;

#if defined (SLEEP_UNTIL_SIGNAL)
    // sleep until signal
    sleep(3);
#endif
#if defined (SAVE_TRACE_ON_EXIT)
    SaveTraceData("tmptestbuf.wtf-trace");
#endif

    sleep(1);
    return 0;
}
