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
    const uint8_t* data;
    unsigned dataLength;

private:
    unsigned pos;

public:
    // a read exception invalidates the read position
    class ReadOutside { };
    class DataOutOfRange { };

    BinaryReader(const void* data_, unsigned size) throw () : data(static_cast<const uint8_t*>(data_)), dataLength(size), pos(0) { }
    BinaryReader(ConstDataBlockRef block) throw () : data(static_cast<const uint8_t*>(block.data())), dataLength(block.size()), pos(0) { }

    void setPosition(unsigned position) throw () { numAssert2(position < dataLength, position, dataLength); pos = position; }
    unsigned getPosition() const throw () { return pos; }
    bool hasMore() const throw () { return pos < dataLength; }

     uint8_t U8 () throw (ReadOutside);
      int8_t S8 () throw (ReadOutside) { return static_cast<int8_t >(U8 ()); }
    uint16_t U16() throw (ReadOutside);
     int16_t S16() throw (ReadOutside) { return static_cast<int16_t>(U16()); }
    uint32_t U32() throw (ReadOutside);
     int32_t S32() throw (ReadOutside) { return static_cast<int32_t>(U32()); }
    uint64_t U64() throw (ReadOutside);
     int64_t S64() throw (ReadOutside) { return static_cast<int64_t>(U64()); }

    float  flt() throw (ReadOutside);
    double dbl() throw (ReadOutside);

     uint8_t U8 ( uint8_t minBound,  uint8_t maxBound) throw (ReadOutside, DataOutOfRange);
      int8_t S8 (  int8_t minBound,   int8_t maxBound) throw (ReadOutside, DataOutOfRange);
    uint16_t U16(uint16_t minBound, uint16_t maxBound) throw (ReadOutside, DataOutOfRange);
     int16_t S16( int16_t minBound,  int16_t maxBound) throw (ReadOutside, DataOutOfRange);
    uint32_t U32(uint32_t minBound, uint32_t maxBound) throw (ReadOutside, DataOutOfRange);
     int32_t S32( int32_t minBound,  int32_t maxBound) throw (ReadOutside, DataOutOfRange);
    uint64_t U64(uint64_t minBound, uint64_t maxBound) throw (ReadOutside, DataOutOfRange);
     int64_t S64( int64_t minBound,  int64_t maxBound) throw (ReadOutside, DataOutOfRange);

    float  flt(float  minBound, float  maxBound) throw (ReadOutside, DataOutOfRange);
    double dbl(double minBound, double maxBound) throw (ReadOutside, DataOutOfRange);

    std::string constLengthStr(unsigned length) throw (ReadOutside);
    std::string str() throw (ReadOutside);

    ConstDataBlockRef block(unsigned length) throw (ReadOutside);
};

class BinaryWriter {
protected:
    uint8_t* data;
    unsigned capacity;

    void reserve(unsigned capacityRequired) throw () { if (capacityRequired > capacity) reallocate(capacityRequired); }
    virtual void reallocate(unsigned capacityRequired) throw () { numAssert2(0, capacity, capacityRequired); }

private:
    unsigned pos;

public:
    BinaryWriter(void* buffer, unsigned bufSize) throw () : data(static_cast<uint8_t*>(buffer)), capacity(bufSize), pos(0) { }
    BinaryWriter(DataBlockRef block) throw () : data(static_cast<uint8_t*>(block.data())), capacity(block.size()), pos(0) { }
    virtual ~BinaryWriter() throw () { }

          uint8_t* accessData()       throw () { return data; }
    const uint8_t* accessData() const throw () { return data; }

    void setPosition(unsigned position) throw () { numAssert2(position <= capacity, position, capacity); pos = position; }
    unsigned getPosition() const throw () { return pos; }
    unsigned getCapacity() const throw () { return capacity; }

    operator      DataBlockRef()       throw () { return      DataBlockRef(data, pos); }
    operator ConstDataBlockRef() const throw () { return ConstDataBlockRef(data, pos); }

    void uncheckedU8 ( uint8_t wData) throw ();
    void uncheckedS8 (  int8_t wData) throw () { uncheckedU8 (static_cast< uint8_t>(wData)); }
    void uncheckedU16(uint16_t wData) throw ();
    void uncheckedS16( int16_t wData) throw () { uncheckedU16(static_cast<uint16_t>(wData)); }
    void uncheckedU32(uint32_t wData) throw ();
    void uncheckedS32( int32_t wData) throw () { uncheckedU32(static_cast<uint32_t>(wData)); }

    void U8 (unsigned wData) throw ();
    void S8 (  signed wData) throw ();
    void U16(unsigned wData) throw ();
    void S16(  signed wData) throw ();
    void U32(unsigned wData) throw ();
    void S32(  signed wData) throw ();
    void U64(uint64_t wData) throw ();
    void S64( int64_t wData) throw () { U64(static_cast<uint64_t>(wData)); }

    void flt(float  wData) throw ();
    void dbl(double wData) throw ();

    void U8 (unsigned wData,  uint8_t minBound,  uint8_t maxBound) throw ();
    void S8 (  signed wData,   int8_t minBound,   int8_t maxBound) throw ();
    void U16(unsigned wData, uint16_t minBound, uint16_t maxBound) throw ();
    void S16(  signed wData,  int16_t minBound,  int16_t maxBound) throw ();
    void U32(unsigned wData, uint32_t minBound, uint32_t maxBound) throw ();
    void S32(  signed wData,  int32_t minBound,  int32_t maxBound) throw ();
    void U64(uint64_t wData, uint64_t minBound, uint64_t maxBound) throw ();
    void S64( int64_t wData,  int64_t minBound,  int64_t maxBound) throw ();

    void flt(float  wData, float  minBound, float  maxBound) throw ();
    void dbl(double wData, double minBound, double maxBound) throw ();

    void constLengthStr(const std::string& wData, unsigned length) throw ();
    void str(const std::string& wData) throw ();

    void block(ConstDataBlockRef wData) throw ();
};

template<unsigned size> class BinaryBuffer : public BinaryWriter {
    uint8_t buffer[size];

public:
    BinaryBuffer() throw () : BinaryWriter(buffer, size) { }
};

class ExpandingBinaryBuffer : public BinaryWriter, private NoCopying {
    virtual void reallocate(unsigned capacityRequired) throw ();

public:
    ExpandingBinaryBuffer() throw ();
    ~ExpandingBinaryBuffer() throw ();
};

#endif
