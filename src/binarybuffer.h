/*
 *  binarybuffer.h
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

#ifndef BINARYBUFFER_H_INC
#define BINARYBUFFER_H_INC

#include <limits>
#include <stdint.h>
#include <string>

#include "nassert.h"

template<unsigned size> class BinaryBuffer {
    uint8_t data[size];
    unsigned dataLength, readPos;

public:
    // a read exception invalidates the read position
    class ReadOverflow { };
    class DataOutOfRange { };

    BinaryBuffer() throw () : dataLength(0), readPos(0) { }

          uint8_t* accessData()       throw () { return data; }
    const uint8_t* accessData() const throw () { return data; }

    static unsigned getSize() throw () { return size; }
    unsigned getDataLength() const throw () { return dataLength; }

    void setDataLength(unsigned length) throw () { numAssert(length <= size, length); dataLength = length; }

    void rewind() throw () { readPos = 0; }

     uint8_t readU8 () throw (ReadOverflow);
      int8_t readS8 () throw (ReadOverflow) { return static_cast<int8_t >(readU8 ()); }
    uint16_t readU16() throw (ReadOverflow);
     int16_t readS16() throw (ReadOverflow) { return static_cast<int16_t>(readU16()); }
    uint32_t readU32() throw (ReadOverflow);
     int32_t readS32() throw (ReadOverflow) { return static_cast<int32_t>(readU32()); }
    uint64_t readU64() throw (ReadOverflow);
     int64_t readS64() throw (ReadOverflow) { return static_cast<int64_t>(readU64()); }

    float  readFloat () throw (ReadOverflow);
    double readDouble() throw (ReadOverflow);

     uint8_t readBoundU8 ( uint8_t minBound,  uint8_t maxBound) throw (ReadOverflow, DataOutOfRange);
      int8_t readBoundS8 (  int8_t minBound,   int8_t maxBound) throw (ReadOverflow, DataOutOfRange);
    uint16_t readBoundU16(uint16_t minBound, uint16_t maxBound) throw (ReadOverflow, DataOutOfRange);
     int16_t readBoundS16( int16_t minBound,  int16_t maxBound) throw (ReadOverflow, DataOutOfRange);
    uint32_t readBoundU32(uint32_t minBound, uint32_t maxBound) throw (ReadOverflow, DataOutOfRange);
     int32_t readBoundS32( int32_t minBound,  int32_t maxBound) throw (ReadOverflow, DataOutOfRange);
    uint64_t readBoundU64(uint64_t minBound, uint64_t maxBound) throw (ReadOverflow, DataOutOfRange);
     int64_t readBoundS64( int64_t minBound,  int64_t maxBound) throw (ReadOverflow, DataOutOfRange);

    float  readBoundFloat (float  minBound, float  maxBound) throw (ReadOverflow, DataOutOfRange);
    double readBoundDouble(double minBound, double maxBound) throw (ReadOverflow, DataOutOfRange);

    std::string readConstLengthString(unsigned length) throw (ReadOverflow);
    std::string readString() throw (ReadOverflow);

    void writeUncheckedU8 ( uint8_t wData) throw ();
    void writeUncheckedS8 (  int8_t wData) throw () { writeUncheckedU8 (static_cast< uint8_t>(wData)); }
    void writeUncheckedU16(uint16_t wData) throw ();
    void writeUncheckedS16( int16_t wData) throw () { writeUncheckedU16(static_cast<uint16_t>(wData)); }
    void writeUncheckedU32(uint32_t wData) throw ();
    void writeUncheckedS32( int32_t wData) throw () { writeUncheckedU32(static_cast<uint32_t>(wData)); }

    void writeU8 (unsigned wData) throw ();
    void writeS8 (  signed wData) throw ();
    void writeU16(unsigned wData) throw ();
    void writeS16(  signed wData) throw ();
    void writeU32(unsigned wData) throw ();
    void writeS32(  signed wData) throw ();
    void writeU64(uint64_t wData) throw ();
    void writeS64( int64_t wData) throw () { writeU64(static_cast<uint64_t>(wData)); }

    void writeFloat (float  wData) throw ();
    void writeDouble(double wData) throw ();

    void writeBoundU8 (unsigned wData,  uint8_t minBound,  uint8_t maxBound) throw ();
    void writeBoundS8 (  signed wData,   int8_t minBound,   int8_t maxBound) throw ();
    void writeBoundU16(unsigned wData, uint16_t minBound, uint16_t maxBound) throw ();
    void writeBoundS16(  signed wData,  int16_t minBound,  int16_t maxBound) throw ();
    void writeBoundU32(unsigned wData, uint32_t minBound, uint32_t maxBound) throw ();
    void writeBoundS32(  signed wData,  int32_t minBound,  int32_t maxBound) throw ();
    void writeBoundU64(uint64_t wData, uint64_t minBound, uint64_t maxBound) throw ();
    void writeBoundS64( int64_t wData,  int64_t minBound,  int64_t maxBound) throw ();

    void writeBoundFloat (float  wData, float  minBound, float  maxBound) throw ();
    void writeBoundDouble(double wData, double minBound, double maxBound) throw ();

    void writeConstLengthString(const std::string& wData, unsigned length) throw ();
    void writeString(const std::string& wData) throw ();
};

// template implementation

template<unsigned size> uint8_t BinaryBuffer<size>::readU8() throw (ReadOverflow) {
    if (readPos + 1 > dataLength)
        throw ReadOverflow();
    return data[readPos++];
}

template<unsigned size> uint16_t BinaryBuffer<size>::readU16() throw (ReadOverflow) {
    if (readPos + 2 > dataLength)
        throw ReadOverflow();
    readPos += 2;
    return (uint16_t(data[readPos - 2]) << 8) | uint16_t(data[readPos - 1]);
}

template<unsigned size> uint32_t BinaryBuffer<size>::readU32() throw (ReadOverflow) {
    if (readPos + 4 > dataLength)
        throw ReadOverflow();
    readPos += 4;
    return (uint32_t(data[readPos - 4]) << 24) | (uint32_t(data[readPos - 3]) << 16) | (uint32_t(data[readPos - 2]) << 8) | uint32_t(data[readPos - 1]);
}

template<unsigned size> uint64_t BinaryBuffer<size>::readU64() throw (ReadOverflow) {
    if (readPos + 8 > dataLength)
        throw ReadOverflow();
    readPos += 8;
    return (uint64_t(data[readPos - 8]) << 56) | (uint64_t(data[readPos - 7]) << 48) | (uint64_t(data[readPos - 6]) << 40) | (uint64_t(data[readPos - 5]) << 32)
         | (uint64_t(data[readPos - 4]) << 24) | (uint64_t(data[readPos - 3]) << 16) | (uint64_t(data[readPos - 2]) <<  8) |  uint64_t(data[readPos - 1]);
}

template<unsigned size> float BinaryBuffer<size>::readFloat() throw (ReadOverflow) {
    STATIC_ASSERT(sizeof(uint32_t) == sizeof(float));
    union { uint32_t i; float f; } conversionHack;
    conversionHack.i = readU32();
    return conversionHack.f;
}

template<unsigned size> double BinaryBuffer<size>::readDouble() throw (ReadOverflow) {
    STATIC_ASSERT(sizeof(uint64_t) == sizeof(double));
    union { uint64_t i; double d; } conversionHack;
    conversionHack.i = readU64();
    return conversionHack.d;
}

template<unsigned size> std::string BinaryBuffer<size>::readConstLengthString(unsigned length) throw (ReadOverflow) {
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

template<unsigned size> std::string BinaryBuffer<size>::readString() throw (ReadOverflow) {
    std::string s;
    for (;;) {
        const uint8_t byte = readU8();
        if (byte == 0)
            return s;
        s += static_cast<char>(byte);
    }
}

template<unsigned size> void BinaryBuffer<size>::writeUncheckedU8(uint8_t wData) throw () {
    nAssert(dataLength + 1 <= size);
    data[dataLength] = wData;
    ++dataLength;
}

template<unsigned size> void BinaryBuffer<size>::writeUncheckedU16(uint16_t wData) throw () {
    nAssert(dataLength + 2 <= size);
    data[dataLength    ] = wData >> 8;
    data[dataLength + 1] = wData;
    dataLength += 2;
}

template<unsigned size> void BinaryBuffer<size>::writeUncheckedU32(uint32_t wData) throw () {
    nAssert(dataLength + 4 <= size);
    data[dataLength    ] = wData >> 24;
    data[dataLength + 1] = wData >> 16;
    data[dataLength + 2] = wData >>  8;
    data[dataLength + 3] = wData;
    dataLength += 4;
}

template<unsigned size> void BinaryBuffer<size>::writeU64(uint64_t wData) throw () {
    nAssert(dataLength + 8 <= size);
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

template<unsigned size> void BinaryBuffer<size>::writeFloat(float wData) throw () {
    STATIC_ASSERT(sizeof(uint32_t) == sizeof(float));
    union { uint32_t i; float f; } conversionHack;
    conversionHack.f = wData;
    writeUncheckedU32(conversionHack.i);
}

template<unsigned size> void BinaryBuffer<size>::writeDouble(double wData) throw () {
    STATIC_ASSERT(sizeof(uint64_t) == sizeof(double));
    union { uint64_t i; double d; } conversionHack;
    conversionHack.d = wData;
    writeU64(conversionHack.i);
}

template<unsigned size> void BinaryBuffer<size>::writeConstLengthString(const std::string& wData, unsigned length) throw () {
    numAssert2(wData.length() == length, wData.length(), length);
    numAssert(dataLength + length <= size, length);
    for (unsigned i = 0; i < length; ++i)
        data[dataLength + i] = wData[i];
    dataLength += length;
}

template<unsigned size> void BinaryBuffer<size>::writeString(const std::string& wData) throw () {
    nAssert(wData.find_first_of('\0') == std::string::npos);
    writeConstLengthString(wData, wData.length());
    writeUncheckedU8(0);
}

#define DEFINE_BOUND_METHODS(BasicType, RealType, name, uncheckedWriteMethod)                                                                          \
    template<unsigned size> RealType BinaryBuffer<size>::readBound##name(RealType minBound, RealType maxBound) throw (ReadOverflow, DataOutOfRange) {  \
        const RealType ret = read##name();                                                                                                             \
        if (ret < minBound || ret > maxBound)                                                                                                          \
            throw DataOutOfRange();                                                                                                                    \
        return ret;                                                                                                                                    \
    }                                                                                                                                                  \
    template<unsigned size> void BinaryBuffer<size>::writeBound##name(BasicType wData, RealType minBound, RealType maxBound) throw () {                \
        numAssert3(wData >= minBound && wData <= maxBound, wData, minBound, maxBound);                                                                 \
        uncheckedWriteMethod(static_cast<RealType>(wData));                                                                                            \
    }

#define DEFINE_METHODS_WITH_CHECKED(BasicType, RealType, name)                                                                                         \
    DEFINE_BOUND_METHODS(BasicType, RealType, name, writeUnchecked##name)                                                                              \
    template<unsigned size> void BinaryBuffer<size>::write##name(BasicType wData) throw () {                                                           \
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

#endif
