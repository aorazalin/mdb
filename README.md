# mdb

Mini C++ debugger for Linux with basic debugging operations:
* Stepping (step in, step out, step over)
* Reading & writing variables
* Setting function/source line/memory breakpoints
* Stack unwinding
* Reading functions/lines/registers

## Installation
After cloning the repository, run cmake:
```
cmake -DCMAKE_CXX_COMPILER="$(which clang++)" -DCMAKE_C_COMPILER="$(which clang)" -S <source_folder> -B <build_folder>
```

## How to debug
You need to compile your code in [clang](https://clang.llvm.org/) with the following flags to remove different optimizations: `-g`, `-O0`, `-fno-omit-frame-pointer`. For details, see [this](https://clang.llvm.org/docs/CommandGuide/clang.html).

Then, simply run debugger like this:
```
/path/to/mdb_executable your_executable
```

## How to use
See `handleCommand` function in `debugger.cc` for details.

## Acknowledgment
* Huge thanks to [Tartan Llama](https://github.com/TartanLlama) for the
[tutorial](https://blog.tartanllama.xyz/)
