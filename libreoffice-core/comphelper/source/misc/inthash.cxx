// Copyright 2011 The Chromium Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <comphelper/inthash.hxx>

namespace comphelper {

// Implement hashing for pairs of at-most 32 bit integer values.
// When size_t is 32 bits, we turn the 64-bit hash code into 32 bits by using
// multiply-add hashing. This algorithm, as described in
// Theorem 4.3.3 of the thesis "Über die Komplexität der Multiplikation in
// eingeschränkten Branchingprogrammmodellen" by Woelfel, is:
//
//   h32(x32, y32) = (h64(x32, y32) * rand_odd64 + rand16 * 2^16) % 2^64 / 2^32
size_t HashInts32(sal_uInt32 value1, sal_uInt32 value2)
{
    sal_uInt64 value1_64 = value1;
    sal_uInt64 hash64 = (value1_64 << 32) | value2;

    if (sizeof(size_t) >= sizeof(sal_uInt64))
        return static_cast<size_t>(hash64);

    sal_uInt64 odd_random = 481046412LL << 32 | 1025306955LL;
    sal_uInt32 shift_random = 10121U << 16;

    hash64 = hash64 * odd_random + shift_random;
    size_t high_bits = static_cast<size_t>(hash64 >> (8 * (sizeof(sal_uInt64) - sizeof(size_t))));
    return high_bits;
}

// Implement hashing for pairs of up-to 64-bit integer values.
// We use the compound integer hash method to produce a 64-bit hash code, by
// breaking the two 64-bit inputs into 4 32-bit values:
// http://opendatastructures.org/versions/edition-0.1d/ods-java/node33.html#SECTION00832000000000000000
// Then we reduce our result to 32 bits if required, similar to above.
size_t HashInts64(sal_uInt64 value1, sal_uInt64 value2)
{
    sal_uInt32 short_random1 = 842304669U;
    sal_uInt32 short_random2 = 619063811U;
    sal_uInt32 short_random3 = 937041849U;
    sal_uInt32 short_random4 = 3309708029U;

    sal_uInt32 value1a = static_cast<sal_uInt32>(value1 & 0xffffffff);
    sal_uInt32 value1b = static_cast<sal_uInt32>((value1 >> 32) & 0xffffffff);
    sal_uInt32 value2a = static_cast<sal_uInt32>(value2 & 0xffffffff);
    sal_uInt32 value2b = static_cast<sal_uInt32>((value2 >> 32) & 0xffffffff);

    sal_uInt64 product1 = static_cast<sal_uInt64>(value1a) * short_random1;
    sal_uInt64 product2 = static_cast<sal_uInt64>(value1b) * short_random2;
    sal_uInt64 product3 = static_cast<sal_uInt64>(value2a) * short_random3;
    sal_uInt64 product4 = static_cast<sal_uInt64>(value2b) * short_random4;

    sal_uInt64 hash64 = product1 + product2 + product3 + product4;

    if (sizeof(size_t) >= sizeof(sal_uInt64))
        return static_cast<size_t>(hash64);

    sal_uInt64 odd_random = 1578233944LL << 32 | 194370989LL;
    sal_uInt32 shift_random = 20591U << 16;

    hash64 = hash64 * odd_random + shift_random;
    size_t high_bits = static_cast<size_t>(hash64 >> (8 * (sizeof(sal_uInt64) - sizeof(size_t))));
    return high_bits;
}

}
