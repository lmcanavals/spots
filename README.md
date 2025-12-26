# Spots

## Install dependencies (creates build/ folder)

You might ignore `uvx` if `conan` in installed system wide.

```sh
uvx conan install . --output-folder=build --build=missing
```

## Configure CMake using the Conan toolchain

```sh
cmake -S . -B build \
-DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
-DCMAKE_BUILD_TYPE=Release
```

## Compile the thing

The `-j8` uses 8 threads to compile, check number of actual processing units and
RAM when the code is too large.

```sh
cmake --build build -j8
```

## Run or install, whatever

```sh
build/spots -h
```
