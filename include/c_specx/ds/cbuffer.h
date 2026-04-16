/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#pragma once

#include "c_specx/dev/assert.h"
#include "c_specx/dev/signatures.h"
#include "c_specx/memory/bytes.h"

/// @file cbuffer.h
/// @brief Circular (ring) buffer over a caller-supplied backing array.
///
/// A cbuffer wraps a fixed-size byte array and maintains head/tail pointers
/// to implement a FIFO queue. No heap allocation is performed — the caller
/// owns the backing memory and decides its lifetime.
///
/// Layout:
/// @code
/// [-------------++++++++++++++++------------]
///  ^tail          ^head
/// @endcode
/// Data occupies [tail, head). When the buffer is full, head == tail and
/// isfull is true.

/// @brief Circular buffer handle.
///
/// Initialize with cbuffer_create() or cbuffer_create_with(). All fields are
/// managed internally — do not modify them directly.
struct cbuffer
{
  u8 *data;    ///< Pointer to the caller-supplied backing array
  u32 cap;     ///< Total capacity of the backing array in bytes
  u32 head;    ///< Write cursor — next byte is written here
  u32 tail;    ///< Read cursor — next byte is read from here
  bool isfull; ///< True when head == tail and the buffer is full (not empty)
};

/// @brief Creates a cbuffer over an existing array with zero initial length.
/// @param data  Pointer to the backing array
/// @param cap   Size of the backing array in bytes
/// @return      Initialized cbuffer with head == tail == 0 and isfull == false
#define cbuffer_create_from(data) cbuffer_create (data, sizeof data)

/// @brief Creates a cbuffer over an existing array, treating it as full.
/// @param data  Pointer to the backing array (already filled)
#define cbuffer_create_full_from(data) \
  cbuffer_create_with (data, sizeof data, sizeof data)

/// @brief Creates a cbuffer from a C string, treating the string bytes as data.
/// @param cstr  Null-terminated string to wrap (length is strlen(cstr))
#define cbuffer_create_from_cstr(cstr) \
  cbuffer_create_with (cstr, strlen (cstr), strlen (cstr))

/// @brief Creates an empty cbuffer over a caller-supplied array.
/// @param data  Pointer to the backing array
/// @param cap   Size of the backing array in bytes
/// @return      Initialized cbuffer with no data
struct cbuffer cbuffer_create (void *data, u32 cap);

/// @brief Creates a cbuffer with an initial data length already present.
/// @param data  Pointer to the backing array (first @p len bytes are considered data)
/// @param cap   Total size of the backing array in bytes
/// @param len   Number of bytes already present in the buffer
/// @return      Initialized cbuffer with head advanced by @p len
struct cbuffer cbuffer_create_with (void *data, u32 cap, u32 len);

////////////////////////////////////////////////////////////
// Utils

/// @brief Returns the number of bytes currently in the buffer.
/// @param b  The cbuffer (must not be NULL)
/// @return   Number of bytes available to read
HEADER_FUNC u32
cbuffer_len (const struct cbuffer *b)
{
  u32 len;
  if (b->isfull)
    {
      len = b->cap;
    }
  else if (b->head >= b->tail)
    {
      len = b->head - b->tail;
    }
  else
    {
      len = b->cap - (b->tail - b->head);
    }
  return len;
}

DEFINE_DBG_ASSERT (struct cbuffer, cbuffer, b, {
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (b->data);
  if (b->isfull)
    {
      ASSERT (b->tail == b->head);
    }
  ASSERT (cbuffer_len (b) <= b->cap);
})

/// @brief Returns true if the buffer contains no data.
/// @param b  The cbuffer (must not be NULL)
HEADER_FUNC bool
cbuffer_isempty (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  return (!b->isfull && b->head == b->tail);
}

/// @brief Returns the number of elements of @p size bytes currently in the buffer.
/// @param b     The cbuffer
/// @param size  Element size in bytes — must evenly divide the current length
/// @return      Number of whole elements present
HEADER_FUNC u32
cbuffer_slen (const struct cbuffer *b, const u32 size)
{
  const u32 len = cbuffer_len (b);
  ASSERT (len % size == 0);
  return len / size;
}

/// @brief Returns the number of bytes available for writing.
/// @param b  The cbuffer (must not be NULL)
/// @return   Bytes of free space remaining
HEADER_FUNC u32
cbuffer_avail (const struct cbuffer *b)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  return b->cap - len;
}

/// @brief Returns the number of elements of @p size bytes that can still be written.
/// @param b     The cbuffer (must not be NULL)
/// @param size  Element size in bytes — must evenly divide the current length
/// @return      Number of whole elements that fit in the remaining space
HEADER_FUNC u32
cbuffer_savail (const struct cbuffer *b, const u32 size)
{
  DBG_ASSERT (cbuffer, b);
  const u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  ASSERT (len % size == 0);
  return (b->cap - len) / size;
}

/// @brief Resets the buffer to empty, discarding all data.
/// @param b  The cbuffer to reset
void cbuffer_discard_all (struct cbuffer *b);

/// @brief Returns a bytes view of the next contiguous free region in the backing array.
///
/// Use this to write directly into the buffer and then call cbuffer_fakewrite()
/// to advance the head pointer.
///
/// @param b  The cbuffer
/// @return   Bytes pointing to the next available contiguous free region
struct bytes cbuffer_get_next_avail_bytes (const struct cbuffer *b);

/// @brief Returns a bytes view of the next contiguous data region in the backing array.
///
/// Use this to read directly from the buffer and then call cbuffer_fakeread()
/// to advance the tail pointer.
///
/// @param b  The cbuffer
/// @return   Bytes pointing to the next available contiguous data region
struct bytes cbuffer_get_next_data_bytes (const struct cbuffer *b);

/// @brief Advances the tail pointer by @p bytes, as if that many bytes were read.
///
/// Does not copy data anywhere — use after consuming bytes obtained from
/// cbuffer_get_next_data_bytes() directly.
///
/// @param b      The cbuffer
/// @param bytes  Number of bytes to consume
void cbuffer_fakeread (struct cbuffer *b, u32 bytes);

/// @brief Advances the head pointer by @p bytes, as if that many bytes were written.
///
/// Does not copy data — use after writing directly into the region returned by
/// cbuffer_get_next_avail_bytes().
///
/// @param b      The cbuffer
/// @param bytes  Number of bytes to mark as written
void cbuffer_fakewrite (struct cbuffer *b, u32 bytes);

////////////////////////////////////////////////////////////
// Raw Read / Write

/// @brief Reads up to @p n elements of @p size bytes from the buffer into @p dest.
/// @param dest  Destination buffer to receive the data
/// @param size  Size of each element in bytes
/// @param n     Maximum number of elements to read
/// @param b     Source cbuffer
/// @return      Number of elements actually read (may be less than @p n if buffer runs dry)
u32 cbuffer_read (void *dest, u32 size, u32 n, struct cbuffer *b);

/// @brief Copies up to @p n elements of @p size bytes from the buffer into @p dest without consuming them.
/// @param dest  Destination buffer to receive the data
/// @param size  Size of each element in bytes
/// @param n     Maximum number of elements to copy
/// @param b     Source cbuffer (unchanged)
/// @return      Number of elements actually copied
u32 cbuffer_copy (void *dest, u32 size, u32 n, const struct cbuffer *b);

/// @brief Writes up to @p n elements of @p size bytes from @p src into the buffer.
/// @param src   Source data to write
/// @param size  Size of each element in bytes
/// @param n     Number of elements to write
/// @param b     Destination cbuffer
/// @return      Number of elements actually written (may be less than @p n if buffer is full)
u32 cbuffer_write (const void *src, u32 size, u32 n, struct cbuffer *b);

/// @brief Reads exactly @p n elements — ASSERTs if the buffer does not have enough data.
#define cbuffer_read_expect(dest, size, n, b)       \
  do                                                \
    {                                               \
      u32 __read = cbuffer_read (dest, size, n, b); \
      ASSERT (__read == n);                         \
    }                                               \
  while (0)

/// @brief Writes exactly @p n elements — ASSERTs if the buffer does not have enough space.
#define cbuffer_write_expect(src, size, n, b)          \
  do                                                   \
    {                                                  \
      u32 __written = cbuffer_write (src, size, n, b); \
      ASSERT (__written == n);                         \
    }                                                  \
  while (0)

////////////////////////////////////////////////////////////
// CBuffer to CBuffer

/// @brief Moves up to @p n elements of @p size bytes from @p src into @p dest, consuming from @p src.
/// @param dest  Destination cbuffer
/// @param size  Element size in bytes
/// @param n     Maximum number of elements to move
/// @param src   Source cbuffer — consumed bytes are removed
/// @return      Number of elements actually moved
u32 cbuffer_cbuffer_move (struct cbuffer *dest, u32 size, u32 n,
                          struct cbuffer *src);

/// @brief Copies up to @p n elements of @p size bytes from @p src into @p dest without consuming @p src.
/// @param dest  Destination cbuffer
/// @param size  Element size in bytes
/// @param n     Maximum number of elements to copy
/// @param src   Source cbuffer (unchanged)
/// @return      Number of elements actually copied
u32 cbuffer_cbuffer_copy (struct cbuffer *dest, u32 size, u32 n,
                          const struct cbuffer *src);

/// @brief Moves all available bytes from @p src into @p dest.
#define cbuffer_cbuffer_move_max(dest, src) \
  cbuffer_cbuffer_move (dest, 1, cbuffer_len (src), src)

/// @brief Copies all available bytes from @p src into @p dest without consuming @p src.
#define cbuffer_cbuffer_copy_max(dest, src) \
  cbuffer_cbuffer_copy (dest, 1, cbuffer_len (src), src)

////////////////////////////////////////////////////////////
// IO Read / Write

/// @brief Stage 1 of a non-blocking write to a file: initiates the write from contiguous data.
///
/// Returns the number of bytes written or a negative error code. Does not advance
/// the tail — call cbuffer_write_to_file_2() after a successful stage 1 to consume
/// the bytes.
///
/// @param dest  Destination file
/// @param b     Source cbuffer
/// @param len   Maximum number of bytes to write
/// @param e     Error object
/// @return      Bytes written (>= 0) or negative error code
i32 cbuffer_write_to_file_1 (i_file *dest, const struct cbuffer *b, u32 len,
                             error *e);

/// @brief Stage 1 write that ASSERTs the full @p len bytes were written.
err_t cbuffer_write_to_file_1_expect (i_file *dest, const struct cbuffer *b,
                                      u32 len, error *e);

/// @brief Stage 2 of a non-blocking write: advances the tail by @p nwritten bytes.
/// @param b        The cbuffer to consume from
/// @param nwritten Number of bytes confirmed written by stage 1
void cbuffer_write_to_file_2 (struct cbuffer *b, u32 nwritten);

/// @brief Writes up to @p len bytes from the buffer to a file in one call (stages 1 + 2).
/// @param dest  Destination file
/// @param b     Source cbuffer — consumed bytes are removed
/// @param len   Maximum number of bytes to write
/// @param e     Error object
/// @return      Bytes written (>= 0) or negative error code
i32 cbuffer_write_to_file (i_file *dest, struct cbuffer *b, u32 len, error *e);

/// @brief Stage 1 of a non-blocking read from a file: fills contiguous free space.
///
/// Returns the number of bytes read or a negative error code. Does not advance
/// the head — call cbuffer_read_from_file_2() after a successful stage 1 to
/// commit the bytes.
///
/// @param src  Source file
/// @param b    Destination cbuffer
/// @param len  Maximum number of bytes to read
/// @param e    Error object
/// @return     Bytes read (>= 0) or negative error code
i32 cbuffer_read_from_file_1 (i_file *src, const struct cbuffer *b, u32 len,
                              error *e);

/// @brief Stage 1 read that ASSERTs the full @p len bytes were read.
err_t cbuffer_read_from_file_1_expect (i_file *src, const struct cbuffer *b,
                                       u32 len, error *e);

/// @brief Stage 2 of a non-blocking read: advances the head by @p nread bytes.
/// @param b      The cbuffer to commit into
/// @param nread  Number of bytes confirmed read by stage 1
void cbuffer_read_from_file_2 (struct cbuffer *b, u32 nread);

/// @brief Reads up to @p len bytes from a file into the buffer in one call (stages 1 + 2).
/// @param src  Source file
/// @param b    Destination cbuffer — head is advanced by bytes read
/// @param len  Maximum number of bytes to read
/// @param e    Error object
/// @return     Bytes read (>= 0) or negative error code
i32 cbuffer_read_from_file (i_file *src, struct cbuffer *b, u32 len, error *e);

////////////////////////////////////////////////////////////
// Single Element Read / Write

/// @brief Reads the element at logical index @p idx without consuming it.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param idx   Zero-based logical index from the tail
/// @param b     The cbuffer
/// @return      true if the index was in range and the element was copied, false otherwise
bool cbuffer_get (void *dest, u32 size, u32 idx, const struct cbuffer *b);

/// @brief Appends one element of @p size bytes to the back (head side) of the buffer.
/// @param src   Source element
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is full
bool cbuffer_push_back (const void *src, u32 size, struct cbuffer *b);

/// @brief Prepends one element of @p size bytes to the front (tail side) of the buffer.
/// @param src   Source element
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is full
bool cbuffer_push_front (const void *src, u32 size, struct cbuffer *b);

/// @brief Removes and returns the element at the back (head side) of the buffer.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_pop_back (void *dest, u32 size, struct cbuffer *b);

/// @brief Removes and returns the element at the front (tail side) of the buffer.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_pop_front (void *dest, u32 size, struct cbuffer *b);

/// @brief Copies the element at the back without removing it.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_peek_back (void *dest, u32 size, const struct cbuffer *b);

/// @brief Copies the element at the front without removing it.
/// @param dest  Destination buffer of at least @p size bytes
/// @param size  Element size in bytes
/// @param b     The cbuffer
/// @return      true on success, false if the buffer is empty
bool cbuffer_peek_front (void *dest, u32 size, const struct cbuffer *b);

/// @brief cbuffer_push_back() that ASSERTs on failure.
#define cbuffer_push_back_expect(src, size, b)       \
  do                                                 \
    {                                                \
      bool __ret = cbuffer_push_back (src, size, b); \
      ASSERT (__ret);                                \
    }                                                \
  while (0)

/// @brief cbuffer_push_front() that ASSERTs on failure.
#define cbuffer_push_front_expect(src, size, b)       \
  do                                                  \
    {                                                 \
      bool __ret = cbuffer_push_front (src, size, b); \
      ASSERT (__ret);                                 \
    }                                                 \
  while (0)

/// @brief cbuffer_pop_back() that ASSERTs on failure.
#define cbuffer_pop_back_expect(dest, size, b)       \
  do                                                 \
    {                                                \
      bool __ret = cbuffer_pop_back (dest, size, b); \
      ASSERT (__ret);                                \
    }                                                \
  while (0)

/// @brief cbuffer_pop_front() that ASSERTs on failure.
#define cbuffer_pop_front_expect(dest, size, b)       \
  do                                                  \
    {                                                 \
      bool __ret = cbuffer_pop_front (dest, size, b); \
      ASSERT (__ret);                                 \
    }                                                 \
  while (0)

/// @brief cbuffer_peek_back() that ASSERTs on failure.
#define cbuffer_peek_back_expect(dest, size, b)       \
  do                                                  \
    {                                                 \
      bool __ret = cbuffer_peek_back (dest, size, b); \
      ASSERT (__ret);                                 \
    }                                                 \
  while (0)

/// @brief cbuffer_peek_front() that ASSERTs on failure.
#define cbuffer_peek_front_expect(dest, size, b)       \
  do                                                   \
    {                                                  \
      bool __ret = cbuffer_peek_front (dest, size, b); \
      ASSERT (__ret);                                  \
    }                                                  \
  while (0)

/// @brief Pushes a single byte value to the back — ASSERTs on failure.
/// @param src  Byte value to push (not a pointer)
/// @param b    The cbuffer
#define cbuffer_pushb_back_expect(src, b)           \
  do                                                \
    {                                               \
      u8 _src = src;                                \
      bool __ret = cbuffer_push_back (&_src, 1, b); \
      ASSERT (__ret);                               \
    }                                               \
  while (0)

/// @brief Pushes a single byte value to the front — ASSERTs on failure.
/// @param src  Byte value to push (not a pointer)
/// @param b    The cbuffer
#define cbuffer_pushb_front_expect(src, b)           \
  do                                                 \
    {                                                \
      u8 _src = src;                                 \
      bool __ret = cbuffer_push_front (&_src, 1, b); \
      ASSERT (__ret);                                \
    }                                                \
  while (0)
