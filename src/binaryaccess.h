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

class BinaryReader {
protected:
    const uint8_t* const data;
    const unsigned dataLength;
    unsigned pos;

public:
    // a read exception invalidates the read position
    class ReadOutside { };
    class DataOutOfRange { };

    BinaryReader(const void* data_, unsigned size) throw () : data(static_cast<const uint8_t*>(data_)), dataLength(size), pos(0) { }
    BinaryReader(ConstDataBlockRef block) throw () : data(static_cast<const uint8_t*>(block.data())), dataLength(block.size()), pos(0) { }

    void setPosition(unsigned position) throw () { numAssert2(position < dataLength, position, dataLength); pos = position; }
    unsigned getPosition() const throw () { return pos; }

     uint8_t readU8 () throw (ReadOutside);
      int8_t readS8 () throw (ReadOutside) { return static_cast<int8_t >(readU8 ()); }
    uint16_t readU16() throw (ReadOutside);
     int16_t readS16() throw (ReadOutside) { return static_cast<int16_t>(readU16()); }
    uint32_t readU32() throw (ReadOutside);
     int32_t readS32() throw (ReadOutside) { return static_cast<int32_t>(readU32()); }
    uint64_t readU64() throw (ReadOutside);
     int64_t readS64() throw (ReadOutside) { return static_cast<int64_t>(readU64()); }

    float  readFloat () throw (ReadOutside);
    double readDouble() throw (ReadOutside);

     uint8_t readBoundU8 ( uint8_t minBound,  uint8_t maxBound) throw (ReadOutside, DataOutOfRange);
      int8_t readBoundS8 (  int8_t minBound,   int8_t maxBound) throw (ReadOutside, DataOutOfRange);
    uint16_t readBoundU16(uint16_t minBound, uint16_t maxBound) throw (ReadOutside, DataOutOfRange);
     int16_t readBoundS16( int16_t minBound,  int16_t maxBound) throw (ReadOutside, DataOutOfRange);
    uint32_t readBoundU32(uint32_t minBound, uint32_t maxBound) throw (ReadOutside, DataOutOfRange);
     int32_t readBoundS32( int32_t minBound,  int32_t maxBound) throw (ReadOutside, DataOutOfRange);
    uint64_t readBoundU64(uint64_t minBound, uint64_t maxBound) throw (ReadOutside, DataOutOfRange);
     int64_t readBoundS64( int64_t minBound,  int64_t maxBound) throw (ReadOutside, DataOutOfRange);

    float  readBoundFloat (float  minBound, float  maxBound) throw (ReadOutside, DataOutOfRange);
    double readBoundDouble(double minBound, double maxBound) throw (ReadOutside, DataOutOfRange);

    std::string readConstLengthString(unsigned length) throw (ReadOutside);
    std::string readString() throw (ReadOutside);
};

class BinaryWriter {
    uint8_t* const data;
    const unsigned capacity;
    unsigned pos;

public:
    BinaryWriter(void* buffer, unsigned bufSize) throw () : data(static_cast<uint8_t*>(buffer)), capacity(bufSize), pos(0) { }
    BinaryWriter(DataBlockRef block) throw () : data(static_cast<uint8_t*>(block.data())), capacity(block.size()), pos(0) { }

          uint8_t* accessData()       throw () { return data; }
    const uint8_t* accessData() const throw () { return data; }

    void setPosition(unsigned position) throw () { numAssert2(position <= capacity, position, capacity); pos = position; }
    unsigned getPosition() const throw () { return pos; }
    unsigned getCapacity() const throw () { return capacity; }

    operator      DataBlockRef()       throw () { return      DataBlockRef(data, pos); }
    operator ConstDataBlockRef() const throw () { return ConstDataBlockRef(data, pos); }

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

template<unsigned size> class BinaryBuffer : public BinaryWriter {
    uint8_t buffer[size];

public:
    BinaryBuffer() : BinaryWriter(buffer, size) { }
};

#endif
