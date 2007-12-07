/*
 *  binaryaccess.cpp
 *
 *  Copyright (C) 2007 - Niko Ritari
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <limits>

#include "binaryaccess.h"

uint8_t BinaryAccess::readU8() throw (ReadOverflow) {
    if (readPos + 1 > dataLength)
        throw ReadOverflow();
    return data[readPos++];
}

uint16_t BinaryAccess::readU16() throw (ReadOverflow) {
    if (readPos + 2 > dataLength)
        throw ReadOverflow();
    readPos += 2;
    return (uint16_t(data[readPos - 2]) << 8) | uint16_t(data[readPos - 1]);
}

uint32_t BinaryAccess::readU32() throw (ReadOverflow) {
    if (readPos + 4 > dataLength)
        throw ReadOverflow();
    readPos += 4;
    return (uint32_t(data[readPos - 4]) << 24) | (uint32_t(data[readPos - 3]) << 16) | (uint32_t(data[readPos - 2]) << 8) | uint32_t(data[readPos - 1]);
}

uint64_t BinaryAccess::readU64() throw (ReadOverflow) {
    if (readPos + 8 > dataLength)
        throw ReadOverflow();
    readPos += 8;
    return (uint64_t(data[readPos - 8]) << 56) | (uint64_t(data[readPos - 7]) << 48) | (uint64_t(data[readPos - 6]) << 40) | (uint64_t(data[readPos - 5]) << 32)
         | (uint64_t(data[readPos - 4]) << 24) | (uint64_t(data[readPos - 3]) << 16) | (uint64_t(data[readPos - 2]) <<  8) |  uint64_t(data[readPos - 1]);
}

float BinaryAccess::readFloat() throw (ReadOverflow) {
    STATIC_ASSERT(sizeof(uint32_t) == sizeof(float));
    union { uint32_t i; float f; } conversionHack;
    conversionHack.i = readU32();
    return conversionHack.f;
}

double BinaryAccess::readDouble() throw (ReadOverflow) {
    STATIC_ASSERT(sizeof(uint64_t) == sizeof(double));
    union { uint64_t i; double d; } conversionHack;
    conversionHack.i = readU64();
    return conversionHack.d;
}

std::string BinaryAccess::readConstLengthString(unsigned length) throw (ReadOverflow) {
    if (readPos + length > dataLength)
        throw ReadOverflow();
    readPos += length;
    #if CHAR_BIT == 8
     return std::string(reinterpret_cast<const char*>(data + readPos - length), length);
    #else
     std::string s;
     for (unsigned p = readPos - length; p < readPos; ++p)
         s += static_cast<char>(data[p]);
     return s;
    #endif
}

std::string BinaryAccess::readString() throw (ReadOverflow) {
    std::string s;
    for (;;) {
        const uint8_t byte = readU8();
        if (byte == 0)
            return s;
        s += static_cast<char>(byte);
    }
}

void BinaryAccess::writeUncheckedU8(uint8_t wData) throw () {
    nAssert(dataLength + 1 <= capacity);
    data[dataLength] = wData;
    ++dataLength;
}

void BinaryAccess::writeUncheckedU16(uint16_t wData) throw () {
    nAssert(dataLength + 2 <= capacity);
    data[dataLength    ] = wData >> 8;
    data[dataLength + 1] = wData;
    dataLength += 2;
}

void BinaryAccess::writeUncheckedU32(uint32_t wData) throw () {
    nAssert(dataLength + 4 <= capacity);
    data[dataLength    ] = wData >> 24;
    data[dataLength + 1] = wData >> 16;
    data[dataLength + 2] = wData >>  8;
    data[dataLength + 3] = wData;
    dataLength += 4;
}

void BinaryAccess::writeU64(uint64_t wData) throw () {
    nAssert(dataLength + 8 <= capacity);
    data[dataLength    ] = wData >> 56;
    data[dataLength + 1] = wData >> 48;
    data[dataLength + 2] = wData >> 40;
    data[dataLength + 3] = wData >> 32;
    data[dataLength + 4] = wData >> 24;
    data[dataLength + 5] = wData >> 16;
    data[dataLength + 6] = wData >>  8;
    data[dataLength + 7] = wData;
    dataLength += 8;
}

void BinaryAccess::writeFloat(float wData) throw () {
    STATIC_ASSERT(sizeof(uint32_t) == sizeof(float));
    union { uint32_t i; float f; } conversionHack;
    conversionHack.f = wData;
    writeUncheckedU32(conversionHack.i);
}

void BinaryAccess::writeDouble(double wData) throw () {
    STATIC_ASSERT(sizeof(uint64_t) == sizeof(double));
    union { uint64_t i; double d; } conversionHack;
    conversionHack.d = wData;
    writeU64(conversionHack.i);
}

void BinaryAccess::writeConstLengthString(const std::string& wData, unsigned length) throw () {
    numAssert2(wData.length() == length, wData.length(), length);
    numAssert(dataLength + length <= capacity, length);
    for (unsigned i = 0; i < length; ++i)
        data[dataLength + i] = wData[i];
    dataLength += length;
}

void BinaryAccess::writeString(const std::string& wData) throw () {
    nAssert(wData.find_first_of('\0') == std::string::npos);
    writeConstLengthString(wData, wData.length());
    writeUncheckedU8(0);
}

#define DEFINE_BOUND_METHODS(BasicType, RealType, name, uncheckedWriteMethod)                                                                          \
    RealType BinaryAccess::readBound##name(RealType minBound, RealType maxBound) throw (ReadOverflow, DataOutOfRange) {  \
        const RealType ret = read##name();                                                                                                             \
        if (ret < minBound || ret > maxBound)                                                                                                          \
            throw DataOutOfRange();                                                                                                                    \
        return ret;                                                                                                                                    \
    }                                                                                                                                                  \
    void BinaryAccess::writeBound##name(BasicType wData, RealType minBound, RealType maxBound) throw () {                \
        numAssert3(wData >= minBound && wData <= maxBound, wData, minBound, maxBound);                                                                 \
        uncheckedWriteMethod(static_cast<RealType>(wData));                                                                                            \
    }

#define DEFINE_METHODS_WITH_CHECKED(BasicType, RealType, name)                                                                                         \
    DEFINE_BOUND_METHODS(BasicType, RealType, name, writeUnchecked##name)                                                                              \
    void BinaryAccess::write##name(BasicType wData) throw () {                                                           \
        writeBound##name(wData, std::numeric_limits<RealType>::min(), std::numeric_limits<RealType>::max());                                           \
    }

#define DEFINE_METHODS_WITHOUT_CHECKED(BasicType, RealType, name)                                                                                      \
    DEFINE_BOUND_METHODS(BasicType, RealType, name, write##name)

DEFINE_METHODS_WITH_CHECKED   (unsigned,  uint8_t, U8 )
DEFINE_METHODS_WITH_CHECKED   (  signed,   int8_t, S8 )
DEFINE_METHODS_WITH_CHECKED   (unsigned, uint16_t, U16)
DEFINE_METHODS_WITH_CHECKED   (  signed,  int16_t, S16)
DEFINE_METHODS_WITH_CHECKED   (unsigned, uint32_t, U32)
DEFINE_METHODS_WITH_CHECKED   (  signed,  int32_t, S32)
DEFINE_METHODS_WITHOUT_CHECKED(uint64_t, uint64_t, U64)
DEFINE_METHODS_WITHOUT_CHECKED( int64_t,  int64_t, S64)

DEFINE_METHODS_WITHOUT_CHECKED(float , float , Float )
DEFINE_METHODS_WITHOUT_CHECKED(double, double, Double)

#undef DEFINE_BOUND_METHODS
#undef DEFINE_METHODS_WITHOUT_CHECKED
#undef DEFINE_METHODS_WITH_CHECKED
