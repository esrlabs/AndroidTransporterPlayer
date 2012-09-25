/*
 * Copyright (C) 2012 E.S.R.Labs GmbH
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
 */

#ifndef AACDECODER_H_
#define AACDECODER_H_

#include <mindroid/os/Ref.h>
#include <mindroid/lang/String.h>
#include "aacdecoder_lib.h"

namespace mindroid {
class Message;
class Buffer;
}

using mindroid::sp;

class AacDecoder :
		public mindroid::Ref {
public:
	AacDecoder(const mindroid::String& codecConfig, const sp<mindroid::Message>& notifyBuffer);
	virtual ~AacDecoder();
	void processBuffer(sp<mindroid::Buffer> buffer);

private:
	static const uint32_t AAC_HEADER_SIZE = 4;
	static const uint32_t PCM_AUDIO_BUFFER_SIZE = 4096;

	sp<mindroid::Buffer> decodeBuffer(sp<mindroid::Buffer> buffer);

	sp<mindroid::Message> mNotifyBuffer;
	HANDLE_AACDECODER mAacDecoder;
};

#endif /* AACDECODER_H_ */
