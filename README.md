# c_specx

Common systems programming extensions for C. This is my personal C standard library — allocators,
data structures, concurrency primitives, I/O streams, and a unit testing framework. I use it across
my C projects so I'm not rewriting the same foundations every time.

---

## Building

### With a preset (recommended)

Presets are defined in `CMakePresets.json`. The main ones are:

| Preset | Description |
|---|---|
| `debug` | Debug build, tests and assertions enabled |
| `release` | Optimized build |
| `package-release` | Release build configured for CPack packaging |

```sh
cmake --preset debug
cmake --build --preset debug
```

### Without a preset

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `ENABLE_NDEBUG` | `OFF` | Disable debug assertions |
| `ENABLE_NTEST` | `OFF` | Strip test code from the build |
| `ENABLE_NLOG` | `OFF` | Disable logging |
| `ENABLE_ASAN` | `OFF` | Enable AddressSanitizer |
| `ENABLE_GPROF` | `OFF` | Enable gprof profiling |
| `ENABLE_STRIP` | `OFF` | Strip symbols from the output binary |
| `ENABLE_PORTABLE` | `OFF` | Build for portability across machines |

---

## Running tests

Tests are compiled into the library by default (when `ENABLE_NTEST` is off). Build the test
executable and run it:

```sh
cmake --preset debug
cmake --build --preset debug
./build/debug/c_specx_test
```

Run a specific suite:

```sh
./build/debug/c_specx_test --suite core
```

Run tests matching a substring:

```sh
./build/debug/c_specx_test randu32r cbuffer_len
```

---

## Installing

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release
```

The default install prefix is `/usr`. To override:

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build --preset release
cmake --install build/release
```

This installs:
- Headers to `$PREFIX/include/`
- Library to `$PREFIX/lib/`
- Docs to `$PREFIX/share/doc/c_specx/`

---

## Using in another project

### Via CMake install

After installing, link against `c_specx` in your `CMakeLists.txt`:

```cmake
find_package(c_specx REQUIRED)
target_link_libraries(my_target PRIVATE c_specx)
```

### Via subdirectory or FetchContent

```cmake
add_subdirectory(thirdparty/c_specx)
target_link_libraries(my_target PRIVATE c_specx)
```

---

## Including headers

Include everything:

```c
#include "c_specx.h"
```

Or include only what you need:

```c
#include "c_specx/memory/chunk_alloc.h"
#include "c_specx/ds/cbuffer.h"
#include "c_specx/concurrency/latch.h"
#include "c_specx/dev/assert.h"
#include "c_specx/dev/testing.h"
#include "c_specx/core/random.h"
#include "c_specx/intf/stream.h"
```

Header groups:

| Path | Contents |
|---|---|
| `c_specx/memory/` | Chunk allocator, slab allocator, malloc plan, linear allocator |
| `c_specx/ds/` | Circular buffer, block array, double buffer, Robin Hood hash table |
| `c_specx/concurrency/` | Latch, S/X latch, granularity lock |
| `c_specx/core/` | Random number generation, hashing, strings, numbers |
| `c_specx/dev/` | Assertions, testing framework, error handling |
| `c_specx/intf/` | Stream interface, platform I/O, logging |

---

## License

Apache 2.0 — see [LICENSE.md](LICENSE.md).
