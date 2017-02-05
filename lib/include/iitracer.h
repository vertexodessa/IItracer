
#pragma once

extern "C" {
void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));
}

void SaveTraceData(const char* filename) __attribute__((no_instrument_function));
