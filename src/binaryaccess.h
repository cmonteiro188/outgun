/*
 *  binaryaccess.h
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

#ifndef BINARYACCESS_H_INC
#define BINARYACCESS_H_INC

#include <stdint.h>
#include <string>

#include "nassert.h"
#include "utility.h"

class BinaryAccess {
    uint8_t* const data;
    const unsigned capacity;
    unsigned dataLength, readPos;

public:
    // a read exception invalidates the read position
    class ReadOverflow { };
    class DataOutOfRange { };

    BinaryAccess(void* buffer, unsigned bufSize) throw () : data(static_cast<uint8_t*>(buffer)), capacity(bufSize), dataLength(0), readPos(0) { }

          uint8_t* accessData()       throw () { return data; }
    const uint8_t* accessData() const throw () { return data; }

    unsigned getCapacity() const throw () { return capacity; }
    unsigned getDataLength() const throw () { return dataLength; }

    operator      DataBlockRef()       throw () { return      DataBlockRef(accessData(), getDataLength()); }
    operator ConstDataBlockRef() const throw () { return ConstDataBlockRef(accessData(), getDataLength()); }

    void setDataLength(unsigned length) throw () { numAssert(length <= capacity, length); dataLength = length; }

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

template<unsigned size> class BinaryBuffer : public BinaryAccess {
    uint8_t buffer[size];

public:
    BinaryBuffer() : BinaryAccess(buffer, size) { }
};

#endif
