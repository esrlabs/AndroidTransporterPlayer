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

#include "AacDecoder.h"
#include "Utils.h"
#include <mindroid/os/Message.h>
#include <mindroid/util/Buffer.h>

using namespace mindroid;

AacDecoder::AacDecoder(const String& codecConfig, const sp<mindroid::Message>& notifyBuffer) :
		mNotifyBuffer(notifyBuffer) {
	mAacDecoder = aacDecoder_Open(TT_MP4_RAW, 1);
	aacDecoder_SetParam(mAacDecoder, AAC_PCM_OUTPUT_INTERLEAVED, 1);

	sp<Buffer> aacCodecConfig = Utils::hexStringToByteArray(codecConfig);
	UCHAR* configBuffers[2];
	UINT configBufferSizes[2] = {0};
	configBuffers[0] = aacCodecConfig->data();
	configBufferSizes[0] = aacCodecConfig->size();
	aacDecoder_ConfigRaw(mAacDecoder, configBuffers, configBufferSizes);
}

AacDecoder::~AacDecoder() {
	aacDecoder_Close(mAacDecoder);
}

void AacDecoder::processBuffer(sp<Buffer> buffer) {
	sp<Buffer> audioBuffer = decodeBuffer(buffer);
	if (audioBuffer != NULL) {
		sp<Message> msg = mNotifyBuffer->dup();
		sp<Bundle> bundle = msg->metaData();
		bundle->putObject("Buffer", audioBuffer);
		msg->sendToTarget();
	}
}

sp<Buffer> AacDecoder::decodeBuffer(sp<Buffer> buffer) {
	if(buffer->size() <= AAC_HEADER_SIZE) {
		return NULL;
	}

	UCHAR* aacInputBuffers[2];
	UINT aacInputBufferSizes[2] = {0};
	UINT numValidBytes[2] = {0};

	sp<Buffer> pcmAudioBuffer = new Buffer(PCM_AUDIO_BUFFER_SIZE);

	aacInputBuffers[0] = (UCHAR*) (buffer->data() + AAC_HEADER_SIZE);
	aacInputBufferSizes[0] = buffer->size() - AAC_HEADER_SIZE;
	numValidBytes[0] = buffer->size() - AAC_HEADER_SIZE;

	AAC_DECODER_ERROR result;
	result = aacDecoder_Fill(mAacDecoder, aacInputBuffers, aacInputBufferSizes, numValidBytes);
	if (result == AAC_DEC_OK) {
		result = aacDecoder_DecodeFrame(mAacDecoder, (INT_PCM*) pcmAudioBuffer->data(), PCM_AUDIO_BUFFER_SIZE, 0);
		if (result == AAC_DEC_OK) {
			return pcmAudioBuffer;
		}
	}

	return NULL;
}
