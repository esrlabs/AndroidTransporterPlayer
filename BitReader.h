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

#ifndef BITREADER_H_
#define BITREADER_H_

#include "mindroid/util/Utils.h"
#include <sys/types.h>
#include <stdint.h>

class BitReader
{
public:
	BitReader(const uint8_t* data, size_t size);
    uint32_t getBits(size_t numBits);
    void skipBits(size_t numBits);
    void putBits(uint32_t value, size_t numBits);
    size_t numBitsAvailable() const;
    const uint8_t* data() const;

private:
    void fillReservoir();

    const uint8_t* mData;
    size_t mSize;
    uint32_t mReservoir; // The reservoir holds the next <= 32 left-aligned bits.
    size_t mNumBitsAvailable;

    NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(BitReader)
};

#endif /* BITREADER_H_ */
