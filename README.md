# iitracer
Automatic function tracer that's using GCC instrumentation to install a hook at functions' enter/exit and Google's Web Tracing Framework to display results in nice manner.

## Requirements:
- libunwind
- CMake 3.1


## Setup and run
# Build instructions
```bash
# first, build and install Google Tracing Framework
git clone https://github.com/vertexodessa/iitracer.git
cd iitracer
pushd .
cd third-party
./build.sh
popd
# build iitracer and test
mkdir build; cd build
cmake ..
make
sudo make install
# build your application with following flags:
  "-finstrument-functions -finstrument-functions-exclude-file-list=/usr/ -finstrument-functions-exclude-function-list=static_initialization_and_destruction,main"
# Launch your app:
LD_PRELOAD=/usr/local/lib/iitracer.so ./your_app
```

## Screenshot

[![Screenshot](https://raw.githubusercontent.com/vertexodessa/iitracer/master/doc/screenshot.png)](https://raw.githubusercontent.com/vertexodessa/iitracer/master/doc/screenshot.png)

## Overhead information

On linux laptop with Intel i7 CPU, overhead is ~0.5ms for the first function call(which caches the demangled function name) and ~0.001 ms for subsequent function calls.

## TODO:
- Create a script to analyze shortest functions calls and blacklist them
- Refactoring: split to more files
- Is it really necessary to mark every function of the library as "no_instrument"? Maybe just build the library without the -finstrument-functions.
- Consider using read-write locks instead of thread-local storages to avoid memory leaks.
- find and fix BUGS! :)

