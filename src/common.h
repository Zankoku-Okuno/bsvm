#ifndef COMMON_H
#define COMMON_H


#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// TODO I'm using a bunch of stuff from
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html to make sure
// that the target platform is relatively "sane". It works for me because I'm
// running gcc, but your mileage may vary.


_Static_assert(CHAR_BIT == 8, "bytes are not 8 bits on the target machine");

typedef uint8_t byte;


#if __SIZEOF_POINTER__ == 4
#elif __SIZEOF_POINTER__ == 8
#else
  _Static_assert(false, "only 32- and 64-bit architectures are supported");
#endif
_Static_assert(__SIZEOF_PTRDIFF_T__ == __SIZEOF_POINTER__
              , "ptrdiff_t is not the same size as void*\n"
                "perhaps you are targeting a platform with a non-flat memory space?");

typedef union word word;
union word {
  uintptr_t bits;
  intptr_t sbits;
  word* wptr;
  byte* bptr;
  ptrdiff_t offset;
  struct {
    # if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    byte low;
    byte _high7[sizeof(uintptr_t) - 1];
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    byte _high7[sizeof(uintptr_t) - 1];
    byte low;
    #else
    #error "bruh, what are you running this on‽"
    #endif
  } byte;
};
_Static_assert(sizeof(word) == sizeof(uintptr_t), "the compiler inserted padding bits into the word type");

// Dunno how to test that integral types have no padding bits, use 2's
// complement for signed numbers, that the order of the bits is consistent
// across integral types, or that overflow is handled by wrapping. Luckily, I
// don't think I have to support the PDP-1.

#if __SIZEOF_POINTER__ == 4
typedef uint64_t ulong;
typedef int64_t slong;
#else
typedef __uint128_t ulong;
typedef __int128_t slong;
#endif


#endif
