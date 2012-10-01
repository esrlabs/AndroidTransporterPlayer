/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Additions and refactorings by E.S.R.Labs GmbH
 */

#include "BitReader.h"
#include <assert.h>

BitReader::BitReader(const uint8_t* data, size_t size) :
		mData(data),
		mSize(size),
		mReservoir(0),
		mNumBitsAvailable(0) {
}

void BitReader::fillReservoir() {
    assert(mSize >= 0);

    mReservoir = 0;
    size_t i;
    for (i = 0; mSize > 0 && i < 4; ++i) {
        mReservoir = (mReservoir << 8) | *mData;

        ++mData;
        --mSize;
    }

    mNumBitsAvailable = 8 * i;
    mReservoir <<= 32 - mNumBitsAvailable;
}

uint32_t BitReader::getBits(size_t numBits) {
    assert(numBits <= 32);

    uint32_t result = 0;
    while (numBits > 0) {
        if (mNumBitsAvailable == 0) {
            fillReservoir();
        }

        size_t n = numBits;
        if (n > mNumBitsAvailable) {
            n = mNumBitsAvailable;
        }

        result = (result << n) | (mReservoir >> (32 - n));
        mReservoir <<= n;
        mNumBitsAvailable -= n;
        numBits -= n;
    }

    return result;
}

void BitReader::skipBits(size_t numBits) {
    while (numBits > 32) {
        getBits(32);
        numBits -= 32;
    }

    if (numBits > 0) {
        getBits(numBits);
    }
}

void BitReader::putBits(uint32_t value, size_t numBits) {
    assert(numBits <= 32);

    while (mNumBitsAvailable + numBits > 32) {
        mNumBitsAvailable -= 8;
        --mData;
        ++mSize;
    }

    mReservoir = (mReservoir >> numBits) | (value << (32 - numBits));
    mNumBitsAvailable += numBits;
}

size_t BitReader::numBitsAvailable() const {
    return mSize * 8 + mNumBitsAvailable;
}

const uint8_t* BitReader::data() const {
    return mData - (mNumBitsAvailable + 7) / 8;
}
