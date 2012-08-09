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

#ifndef AACMEDIAASSEMBLER_H_
#define AACMEDIAASSEMBLER_H_

#include "MediaAssembler.h"
#include <mindroid/util/List.h>
#include <mindroid/lang/String.h>

#include "aacdecoder_lib.h"

namespace mindroid {
class Message;
class Buffer;
}

using mindroid::sp;

class AacMediaAssembler :
	public MediaAssembler
{
public:
	AacMediaAssembler(sp< mindroid::List< sp<mindroid::Buffer> > > queue, const sp<mindroid::Message>& notifyBuffer, const mindroid::String& config);
	virtual ~AacMediaAssembler();

	virtual Status assembleMediaData();
	virtual uint32_t getNextSeqNum() const;

private:
	static const UINT AAC_HEADER_LENGTH = 4;
	static const UINT AAC_OUTPUT_LENGTH = 4096;

	sp<mindroid::Buffer> decodeAacPacket(sp<mindroid::Buffer> buffer);

	sp< mindroid::List< sp<mindroid::Buffer> > > mQueue;
	sp<mindroid::Message> mNotifyBuffer;
	HANDLE_AACDECODER mAacDecoder;
    RAM_ALIGN INT_PCM mAacOutput[AAC_OUTPUT_LENGTH];
};

#endif /* AACMEDIAASSEMBLER_H_ */
