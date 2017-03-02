#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace ProcMaps {
struct Entry {
    Entry() = default;
    Entry(intptr_t start, intptr_t end, const char* path);
    ~Entry() = default;
    bool operator <(const Entry& rhs);
    bool operator <(const intptr_t& rhs);

    // data
    intptr_t start_;
    intptr_t end_;
    std::string path_;
};

void InitLibraryMappings();
std::vector<Entry>& Libraries();
/* static std::vector<Entry>  ParseMaps(char const* path = "/proc/self/maps"); */
const Entry& find(intptr_t address);

} //namespace Utils
