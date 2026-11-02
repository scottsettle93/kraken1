# How a ring buffer works

A ring buffer is a queue that reuses a fixed block of storage. After an operation
reaches the last array element, its position wraps back to the first. The array
stays in place; only the positions move. This makes a ring useful for transferring
a stream of data between a producer and a consumer without allocating memory or
shifting existing elements on each operation.

Consider a byte queue backed by `std::array<std::uint8_t, N>`, where `N >= 2`:

- **Storage** contains `N` physical slots, numbered `0` through `N - 1`.
- **Write position**, `write`, identifies where the producer will insert the next byte.
- **Read position**, `read`, identifies the oldest unread byte.
- **Usable capacity** is `N - 1` bytes because this design always leaves one slot unused.

Initially, both positions are zero. Define the next position as
`next(i) = (i + 1) % N`: modulo brings position `N - 1` back to zero. Thus, an
eight-slot array has seven usable slots. The unused slot is not a particular
array element; it moves as the positions advance. This follows the convention
described in the [Linux circular-buffer documentation](https://docs.kernel.org/core-api/circular-buffers.html).

## Writing, reading, and wrapping

First consider operations executed one at a time, without interrupts or threads:

1. **Write:** Compute `next(write)`. If it equals `read`, report full and leave the
   queue unchanged. Otherwise, store the byte at `storage[write]`, then advance
   `write` to that next position.
2. **Read:** If `read == write`, report empty and leave the queue unchanged.
   Otherwise, copy the byte from `storage[read]`, then advance `read` using
   `next(read)`.

These operations preserve first-in, first-out (FIFO) order: the first byte
successfully written is the first one read. Each operation does constant work
for a byte, regardless of how many bytes are queued. Reading does not need to
erase the array element; advancing `read` makes that slot available for reuse.
Old values in unused slots are irrelevant.

Here is an eight-slot ring after writing `A` through `F`, reading `A` through `E`,
then writing `G`, `H`, and `I`:

```text
Physical index:    0   1   2   3   4   5   6   7
                 +---+---+---+---+---+---+---+---+
Stored bytes:    | I | . | . | . | . | F | G | H |
                 +---+---+---+---+---+---+---+---+
write = 1; read = 5; '.' means unused

Logical read order:  F -> G -> H -> I
Physical positions:  5 -> 6 -> 7 -> 0
```

Writing three more bytes fills positions `1`, `2`, and `3`, leaving `write = 4`.
The ring is now full: `next(4)` is `5`, equal to `read`. Position `4` remains
unused. Reading `F` advances `read` to `6`, permitting another write. Reading
every remaining byte eventually makes `read == write`, indicating empty.

Reserving one slot distinguishes these two states without a separate count or
full flag. If all `N` slots were filled using only these wrapped positions,
`write` would catch `read`, making full indistinguishable from empty. Other ring
designs track additional state to use every slot; their full/empty rules differ.

## Sharing the ring with an interrupt

The mechanics above are not, by themselves, a concurrency implementation. With
one producer and one consumer (SPSC), only the producer advances `write`, and
only the consumer advances `read`. Each also observes the other's position.

The producer must finish storing a byte before publishing its new write
position. The consumer must observe that publication before reading the byte,
and finish copying it before publishing a read position that permits reuse.
A C++ design can express these handoffs using atomic indices with release
stores and acquire loads of the other side's index. Before using this in an
interrupt handler, verify that the selected types and operations are lock-free
and supported there by the target toolchain; atomic operations can otherwise
require runtime routines. [GCC's atomic documentation](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html)
explains ordering and target-dependent support.

Declaring indices `volatile` does not provide synchronization or publish ordinary
array writes in the required order, as [GCC documents](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html).
Alternatively, a single-core main loop can perform each queue operation in a
short, platform-supported critical section that masks the producer interrupt
and supplies the required compiler ordering. Restore the previous interrupt
state afterward. Neither approach should be assumed portable to every embedded
target.

## Practical scenario: receiving UART data

A UART receive interrupt handler can copy incoming bytes into the ring while
the main loop removes them for parsing. The ring absorbs temporary bursts or
delays in the main loop. Fixed storage bounds memory use, and constant work per
byte helps keep the handler short. Initialize the ring before enabling the
interrupt, and keep parsing outside it. The handler is the sole producer; the
main loop is the sole consumer, using the synchronization described above.

If arrivals outpace processing long enough, the ring fills. A larger ring only
extends the time available; it cannot compensate indefinitely for a slower
consumer. For example, at 115,200 baud with one start bit, eight data bits, and
one stop bit, the line carries 11,520 bytes per second. A 256-slot ring holds
255 bytes, about 22 milliseconds of arrivals when initially empty and not
being drained. The UART's own receive storage can also overrun if interrupts
are serviced too late, even when the software ring has space.

Two useful policies are:

- **Apply backpressure:** If both endpoints support RTS/CTS flow control, request
  a pause before the ring fills, reserving room for bytes already in transit.
- **Handle loss explicitly:** If pausing is unavailable, reject the newest byte
  when full, record overflow safely, and let the parser recover at a valid frame
  boundary. This preserves unread bytes and index ownership. Never wait inside
  the interrupt for the main loop to free space: the interrupted consumer cannot
  make progress.
