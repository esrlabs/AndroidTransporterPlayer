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
 * Refactorings by E.S.R.Labs GmbH
 */

#ifndef PCMMEDIAASSEMBLER_H_
#define PCMMEDIAASSEMBLER_H_

#include "MediaAssembler.h"
#include <mindroid/util/List.h>

namespace mindroid {
class Message;
class Buffer;
}

using mindroid::sp;

class PcmMediaAssembler :
	public MediaAssembler
{
public:
	PcmMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyBuffer);
	virtual ~PcmMediaAssembler();

	virtual Status assembleMediaData();
	virtual uint32_t getNextSeqNum() const;

private:
	void swapPcmDataEndianess(const sp<mindroid::Buffer>& buffer);

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyBuffer;
};

#endif /* PCMMEDIAASSEMBLER_H_ */
