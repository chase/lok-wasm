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

#pragma once

#include "sal/types.h"
#include <comphelper/comphelperdllapi.h>
#include <utility>

namespace comphelper
{

// Hash pairs of 32-bit or 64-bit numbers.
COMPHELPER_DLLPUBLIC size_t HashInts32(sal_uInt32 value1, sal_uInt32 value2);
COMPHELPER_DLLPUBLIC size_t HashInts64(sal_uInt64 value1, sal_uInt64 value2);

template <typename T1, typename T2> inline size_t HashInts(T1 value1, T2 value2)
{
    // This condition is expected to be compile-time evaluated and optimised away
    // in release builds.
    if (sizeof(T1) > sizeof(sal_uInt32) || (sizeof(T2) > sizeof(sal_uInt32)))
        return HashInts64(value1, value2);

    return HashInts32(static_cast<sal_uInt32>(value1), static_cast<sal_uInt32>(value2));
}

// A templated hasher for pairs of integer types. Example:
//
//   using MyPair = std::pair<int32_t, int32_t>;
//   std::unordered_set<MyPair, base::IntPairHash<MyPair>> set;
template <typename T> struct IntPairHash;

template <typename Type1, typename Type2> struct IntPairHash<std::pair<Type1, Type2>>
{
    size_t operator()(std::pair<Type1, Type2> value) const
    {
        return HashInts(value.first, value.second);
    }
};

}
