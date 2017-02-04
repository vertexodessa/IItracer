# iitracer
Automatic function tracer that's using GCC instrumentation to install a hook at functions' enter/exit and Google's Web Tracing Framework to display results in nice manner.

## Requirements:
- libunwind
- CMake 3.1


## Setup
-- add setup instructions

## Screenshot

[![Screenshot](https://raw.githubusercontent.com/vertexodessa/iitracer/master/doc/screenshot.png)](https://github.com/vertexodessa/iitracer)

## Overhead information

On linux laptop with Intel i7 CPU, overhead is ~0.5ms for the first function call(which caches the demangled function name) and ~0.001 ms for subsequent function calls.
