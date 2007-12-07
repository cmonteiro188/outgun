/*
 *  tests/binarybuffer.cpp
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

#include "../binaryaccess.h"
#include "../binders.h"

#include "tests.h"

using namespace std;

void binaryBufferTest() throw () {
    BinaryBuffer<20> b1, b2;

    nAssert(b1.getCapacity() == 20);

    b1.writeU8(200);
    b1.writeS8(-100);
    b1.writeU16(65535);
    b1.writeS16(-1);
    b1.writeU32(0);
    b1.writeS32(-4);
    b1.writeConstLengthString("bl", 2);
    b1.writeString("st");
    nAssert(b1.getDataLength() == 19);

    const uint8_t* data1 = b1.accessData();
    uint8_t* data2 = b2.accessData();
    memcpy(data2, data1, b1.getDataLength());
    b2.setDataLength(b1.getDataLength());

    b1.writeU8(0);
    nAssert(b1.getDataLength() == 20);
    testAssertion(VMFbind1(b1, &BinaryBuffer<20>::writeS8, 0));

    b1.setDataLength(0);
    testAssertion(VMFbind3(b1, &BinaryBuffer<20>::writeBoundS8, -11, -10, -1));
    b1.writeBoundS8(-10, -10, -1);
    b1.writeBoundS8(-1, -10, -1);
    testAssertion(VMFbind3(b1, &BinaryBuffer<20>::writeBoundS8, 0, -10, -1));
    b1.setDataLength(0);
    for (int i = 0; i < 10; ++i)
        b1.writeBoundS8(i, 0, 9);
    const double testFloat = .123456789123456789123456789;
    b1.writeDouble(testFloat);
    b1.writeS8(0);
    nAssert(b1.getDataLength() == 19);
    testAssertion(VMFbind1(b1, &BinaryBuffer<20>::writeS16, 0));
    nAssert(b1.readBoundU8(0, 0) == 0);
    nAssert(b1.readU64() == 0x0102030405060708);
    nAssert(b1.readS8() == 9);
    nAssert(b1.readDouble() == testFloat);
    b1.rewind();
    nAssert(b1.readS8() == 0);

    nAssert(b2.readBoundU8(200, 200) == 200);
    nAssert(b2.readBoundS8(-128, -1) == -100);
    nAssert(b2.readU16() == 65535);
    nAssert(b2.readS16() == -1);
    try {
        b2.readBoundU32(1, 1000);
        nAssert(0);
    } catch (BinaryBuffer<20>::DataOutOfRange) { }
    nAssert(b2.readS32() == -4);
    nAssert(b2.readConstLengthString(2) == "bl");
    nAssert(b2.readString() == "st");
    try {
        b2.readS8();
        nAssert(0);
    } catch (BinaryBuffer<20>::ReadOverflow) { }
}

int main() {
    binaryBufferTest();
    return 0;
}
