#include "proc_maps.h"

#include <set>
#include <fstream>
#include <sstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#include <iostream>
#include <stdio.h>

using namespace std;

namespace {

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

inline bool exists (const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}

static vector<ProcMaps::Entry> ParseMaps(char const* path = "/proc/self/maps") {
    std::ifstream infile(path);
    std::string line;

    vector<ProcMaps::Entry> ret;
    while (std::getline(infile, line)) {
        vector<string> tokens = split(line, ' ');

        if (tokens.empty())
            continue;

        vector<string> addrs = split(tokens[0], '-');

        uintptr_t begin, end;
        string path;

        try{
            begin = std::stoul(addrs[0], nullptr, 16);
            end   = std::stoul(addrs[1], nullptr, 16);
        } catch (...) {
            continue;
        }
        path = tokens[tokens.size() - 1];

        if (path.empty() || !exists(path))
            continue;

        ProcMaps::Entry e(begin, end, path.c_str());

        //cout << hex << "begin: " << begin << ", end: " << end << ", path: " << path << '\n';
        ret.push_back(e);
    }

    sort(ret.begin(), ret.end());
    return ret;
}

}
vector <ProcMaps::Entry> gLibraryMappings;

namespace ProcMaps {
    Entry::Entry(intptr_t start, intptr_t end, const char* path)
        : start_(start),
          end_(end),
          path_(path) {    };
    bool Entry::operator <(const Entry& rhs) {
        return start_ < rhs.start_;
    };

    bool Entry::operator <(const intptr_t& rhs) {
        return end_ < rhs;
    };

    vector<Entry>& Libraries() {
        return gLibraryMappings;
    }

    void InitLibraryMappings() {
        Libraries() = ParseMaps();
    }

    const Entry& find(intptr_t address) {
        // printf("find: %ld, size %d\n", address, Libraries().size());
        // HACK
        // static once_flag f;
        // call_once(f, []() {
        //     Libraries() = ParseMaps();
        //     });
        if (Libraries().empty()) {
            Libraries() = ParseMaps();
        }
        auto it = lower_bound(Libraries().begin(), Libraries().end(), address);
        return (it == Libraries().end())? *(Libraries().end() - 1) : *it;
    }
} //namespace Utils

// #include <iostream>
// int main () {
//     ProcMaps::InitLibraryMappings();

//     auto vec = ProcMaps::Libraries();
//     for (auto v : vec) {
//         cout << hex << "begin: " << v.start_ << ", end: " << v.end_ << ", path: " << v.path_ << '\n';
//     }

//     const auto what = 0x609500;
//     auto v = ProcMaps::find(what);
//     cout << hex << what<<" is located in: begin: " << v.start_ << ", end: " << v.end_ << ", path: " << v.path_ << '\n';
//     return 1;
// }
