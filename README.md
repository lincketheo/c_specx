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

```sh
cmake --preset debug
cmake --build --preset debug
./build/debug/c_specx_test               # run everything
./build/debug/c_specx_test --suite core  # one suite
./build/debug/c_specx_test randu32r      # substring filter
```

---

## Installing

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release
```

Default install prefix is `/usr`. To override:

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build --preset release
cmake --install build/release
```

---

## Using in another project

### Via CMake install

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

## Examples

### Chunk allocator

```c
#include "c_specx/memory/chunk_alloc.h"

struct chunk_alloc ca;
chunk_alloc_create_default (&ca);

error e = error_create();
char *buf = chunk_malloc (&ca, 64, sizeof *buf, &e);

chunk_alloc_reset_all (&ca); /* reuse without freeing */
chunk_alloc_free_all (&ca);  /* tear down */
```

### Malloc plan

```c
#include "c_specx/memory/malloc_plan.h"

struct malloc_plan p = malloc_plan_create ();

/* Pass 1 — planning: accumulates sizes */
malloc_plan_memcpy (&p, header, header_len);
malloc_plan_memcpy (&p, body,   body_len);

error e = error_create();
malloc_plan_alloc (&p, &e); /* single malloc */

/* Pass 2 — allocing: fills the buffer */
malloc_plan_memcpy (&p, header, header_len);
malloc_plan_memcpy (&p, body,   body_len);
```

### Slab allocator

```c
#include "c_specx/memory/slab_alloc.h"

struct slab_alloc sa;
slab_alloc_init (&sa, sizeof (struct my_node), 64);

error e = error_create();
struct my_node *n = slab_alloc_alloc (&sa, &e);
/* ... use n ... */
slab_alloc_free (&sa, n);
slab_alloc_destroy (&sa);
```

### Circular buffer

```c
#include "c_specx/ds/cbuffer.h"

u8 buf[64];
struct cbuffer b = cbuffer_create (buf, sizeof buf);

cbuffer_pushb_back_expect (0xAB, &b);
cbuffer_pushb_back_expect (0xCD, &b);

u8 out;
cbuffer_pop_front_expect (&out, 1, &b); /* out == 0xAB */
```

### Block array

```c
#include "c_specx/ds/block_array.h"

error e = error_create();
struct block_array *arr = block_array_create (256, &e);

const u8 data[] = { 1, 2, 3, 4 };
block_array_insert (arr, 0, data, sizeof data, &e);

u8 out[4];
block_array_read (arr, stride_all, sizeof out, out);

block_array_free (arr);
```

### Double buffer

```c
#include "c_specx/ds/dbl_buffer.h"

struct dbl_buffer db;
error e = error_create();
dblb_create (&db, sizeof (u32), 16, &e);

const u32 vals[] = { 1, 2, 3 };
dblb_append (&db, vals, 3, &e);

/* zero-copy reserve */
u32 *slot = dblb_append_alloc (&db, 1, &e);
*slot = 4;

dblb_free (&db);
```

### Robin Hood hash table

Instantiate a typed table with three macros and an include. Backing storage is caller-supplied.

```c
#include "c_specx/dev/stdtypes.h"

#define VTYPE  int
#define KTYPE  u32
#define SUFFIX u32
#include "c_specx/ds/robin_hood_ht.h"
#undef VTYPE
#undef KTYPE
#undef SUFFIX

hash_table_u32 ht;
hentry_u32     data[64];
ht_init_u32 (&ht, data, 64);

const hdata_u32 entry = { .key = 42, .value = 99 };
ht_insert_u32 (&ht, entry);

hdata_u32 *found = ht_lookup_u32 (&ht, 42);
/* found->value == 99 */

ht_delete_u32 (&ht, 42);
```

Each instantiation is independent — include the header again with different macros to get a second
typed table.

### Stream

```c
#include "c_specx/intf/stream.h"

const u8 src_data[] = "hello";
u8 dst_data[8] = error_create();

struct stream src, dst;
struct stream_ibuf_ctx ictx;
struct stream_obuf_ctx octx;

stream_ibuf_init (&src, &ictx, src_data, sizeof src_data);
stream_obuf_init (&dst, &octx, dst_data, sizeof dst_data);

error e = error_create();
stream_read (&dst, 1, sizeof src_data, &src, &e);
```

### Struct assertions

```c
#include "c_specx/dev/assert.h"

DEFINE_DBG_ASSERT (struct cbuffer, cbuffer, b, {
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (cbuffer_len (b) <= b->cap);
})

void
cbuffer_push_back (const void *src, u32 size, struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b); /* free in release */
  /* ... */
}
```

### Testing framework

Tests live in the same translation unit as the code they test, compiled out in production via
`ENABLE_NTEST`.

```c
#include "c_specx/dev/testing.h"

#ifndef NTEST
TEST (cbuffer_isempty)
{
  u8 buf[1];
  struct cbuffer b = cbuffer_create (buf, 1);
  test_assert (cbuffer_isempty (&b));

  const u8 v = 0xFF;
  cbuffer_push_back (&v, 1, &b);
  test_assert (!cbuffer_isempty (&b));
}
#endif
```

Register in your test main:

```c
TEST_SUITE (core, 128);
REGISTER  (core, cbuffer_isempty);
```

### Latch

```c
#include "c_specx/concurrency/latch.h"

latch l;
latch_init  (&l);
latch_lock  (&l);
/* critical section */
latch_unlock (&l);
```

### S/X latch

```c
#include "c_specx/concurrency/spx_latch.h"

sx_latch l;
spx_latch_init (&l);

spx_lock_s (&l);       /* shared — multiple readers OK */
/* ... read ... */
spx_upgrade_s_x (&l);  /* promote to exclusive */
/* ... write ... */
spx_unlock_x (&l);
```

### Granularity lock

```c
#include "c_specx/concurrency/gr_lock.h"

struct gr_lock l;
error e = error_create();
gr_lock_init (&l, &e);

gr_lock   (&l, LM_S, &e); /* shared read */
/* ... */
gr_unlock (&l, LM_S);

gr_lock   (&l, LM_X, &e); /* exclusive write */
/* ... */
gr_unlock (&l, LM_X);

gr_lock_destroy (&l);
```

---

## License

Apache 2.0 — see [LICENSE.md](LICENSE.md).
