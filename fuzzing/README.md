# Fuzzing

Basic transaction fuzzer for the IoTeX application.

## Build

Fuzzing is performed by [libFuzzer](https://llvm.org/docs/LibFuzzer.html).
Building fuzzer requires [CMake](https://cmake.org/), [Clang](https://clang.llvm.org/) and Protobuf compiler..

Under Ubuntu, these dependencies can be installed with:

```shell
sudo apt install clang cmake protobuf-compiler
```

Once installation is finished, simply run:

```shell
./build.sh
```

## Running fuzzer

To start the fuzzer, run:

```shell
./run.sh
```

To view code coverage, run:

```shell
./coverage.sh
```
