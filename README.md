# wolf-midi

## Intro

Refer to [QMidi](https://github.com/waddlesplash/QMidi).

Only provides the most basic MIDI file read and write functions.

## Install

```bash
vcpkg install wolf-midi
```

```cmake
find_package(wolf-midi)
target_link_libraries(${PROJECT_NAME} PRIVATE wolf-midi::wolf-midi)
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Options:

- `WOLF_MIDI_BUILD_STATIC` (`OFF` by default): build a static library instead of a shared one.
- `WOLF_MIDI_BUILD_TESTS` (`OFF` by default): build the test executable and register it with CTest.
- `WOLF_MIDI_INSTALL` (`ON` by default): generate the install and CMake package rules.

## Test

Tests are not built by default. Enable them explicitly and run with CTest:

```bash
cmake -S . -B build -DWOLF_MIDI_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Usage

## MIDI File I/O

Quick example:
```c++
MidiFile f;
f.load(" .. some filename .. ");
f.save(" .. some filename .. ");
```
You can get the events using `f.events()` which returns a `std::list<MidiEvent*>*`.