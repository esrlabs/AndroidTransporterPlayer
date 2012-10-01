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

#ifndef CSDUTILS_H_
#define CSDUTILS_H_

#include "mindroid/os/Ref.h"

class BitReader;
namespace mindroid {
class String;
class Buffer;
}

using mindroid::sp;

/* Utility functions for codec specific data */
class CsdUtils {
public:
	static sp<mindroid::Buffer> hexStringToByteArray(const mindroid::String& hexString);
	static sp<mindroid::Buffer> decodeBase64String(const mindroid::String& string);
	static void buildAvcCodecSpecificData(const mindroid::String strProfileId, const mindroid::String strSpropParamSet, sp<mindroid::Buffer>* sps, sp<mindroid::Buffer>* pps);

private:
	static void getAvcDimensions(const sp<mindroid::Buffer>& seqParamSet, int32_t* width, int32_t* height);
	static unsigned parseUE(BitReader& br); // See the ITU-T H.264 spec (sec 9.1) for Exp-Golomb code parsing.
};

#endif /* CSDUTILS_H_ */
