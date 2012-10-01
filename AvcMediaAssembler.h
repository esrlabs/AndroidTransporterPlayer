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

#ifndef AVCMEDIAASSEMBLER_H_
#define AVCMEDIAASSEMBLER_H_

#include "MediaAssembler.h"
#include "mindroid/util/List.h"

namespace mindroid {
class Message;
class Buffer;
}

using mindroid::sp;

class AvcMediaAssembler :
	public MediaAssembler
{
public:
	AvcMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyAccessUnit);
	virtual ~AvcMediaAssembler();

	virtual Status assembleMediaData();
	virtual uint32_t fixPacketLoss() const;

private:
	static const uint8_t F_BIT = 1 << 7;
	static const uint8_t FU_START_BIT = 1 << 7;
	static const uint8_t FU_END_BIT = 1 << 6;

	void processSingleNalUnit(sp<mindroid::Buffer> nalUnit);
	Status processFragNalUnit();

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyAccessUnit;
};

#endif /* AVCMEDIAASSEMBLER_H_ */
