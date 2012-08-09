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

#include "AacMediaAssembler.h"
#include <mindroid/os/Message.h>
#include <mindroid/util/Buffer.h>
#include <mindroid/os/Clock.h>
#include <string.h>
#include <stdio.h>
#include "Bitstream.h"

using namespace mindroid;

AacMediaAssembler::AacMediaAssembler(sp< List< sp<Buffer> > > queue, const sp<Message>& notifyBuffer, const String& config) :
		mQueue(queue),
		mNotifyBuffer(notifyBuffer) {
	mAacDecoder = aacDecoder_Open(TT_MP4_RAW, /* layers */ 1);
	assert(mAacDecoder != NULL);
	assert(AAC_DEC_OK == aacDecoder_SetParam(mAacDecoder, AAC_PCM_OUTPUT_INTERLEAVED, 1));

	UINT configBufferSize = (UINT) (config.size() / 2);
	UCHAR configBuffer[configBufferSize];
	for(UINT i = 0; i < configBufferSize; i++) {
		String val = config.substr(i*2, i*2 + 2);
		configBuffer[i] = (UCHAR) strtol(val.c_str(), NULL, 16);
	}
    UCHAR* configBuffers[1] = { configBuffer };
    UINT configBufferSizes[1] = { configBufferSize };
	assert(AAC_DEC_OK == aacDecoder_ConfigRaw(mAacDecoder, configBuffers, configBufferSizes));
}

AacMediaAssembler::~AacMediaAssembler() {
	aacDecoder_Close(mAacDecoder);
}

MediaAssembler::Status AacMediaAssembler::assembleMediaData() {
	if (mQueue->empty()) {
		return OK;
	}

	sp<Buffer> buffer = *mQueue->begin();

	if (!mInitSeqNumber) {
		List< sp<Buffer> >::iterator itr = mQueue->begin();
		while (itr != mQueue->end()) {
			if ((uint32_t)(*itr)->getId() > mSeqNumber) {
				break;
			}

			itr = mQueue->erase(itr);
		}

		if (mQueue->empty()) {
			return OK;
		}
	}

	if (mInitSeqNumber) {
		mSeqNumber = (uint32_t) buffer->getId();
		mInitSeqNumber = false;
	} else {
		if ((uint32_t)buffer->getId() != mSeqNumber + 1) {
			return SEQ_NUMBER_FAILURE;
		}
	}

	if (buffer->size() < 1) {
		mQueue->erase(mQueue->begin());
		++mSeqNumber;
		return PACKET_FAILURE;
	}

	sp<Buffer> decoded = decodeAacPacket(buffer);
	if (decoded != NULL) {
		sp<Message> msg = mNotifyBuffer->dup();
		sp<Bundle> bundle = msg->metaData();
		bundle->putObject("Buffer", decoded);
		msg->sendToTarget();
	}

	mQueue->erase(mQueue->begin());
	mSeqNumber++;

	return OK;
}

uint32_t AacMediaAssembler::getNextSeqNum() const {
	return (*mQueue->begin())->getId();
}

sp<Buffer> AacMediaAssembler::decodeAacPacket(sp<Buffer> buffer) {
	if(buffer->size() <= AAC_HEADER_LENGTH) {
		printf("Invalid AAC-Packet: %d bytes\n", buffer->size());
		return NULL;
	}

	BitstreamReader bsAccHeader(buffer->data(), 2);
	uint16_t aacHeaderLength = bsAccHeader.get<16>();
	BitstreamReader bsAuHeader(buffer->data() + 2, 2);
	uint16_t auSize = bsAuHeader.get<13>();
	uint8_t auIndex = bsAuHeader.get<3>();
	if(aacHeaderLength != 16 || auSize != (buffer->size() - AAC_HEADER_LENGTH) || auIndex != 0) {
		printf("Invalid AAC-Packet: %d bytes (aac-header: %d bits / au-size %d bytes / au-index: %d)\n",
				buffer->size(),
				aacHeaderLength,
				auSize,
				auIndex);
		return NULL;
	}

	sp<Buffer> decoded = new Buffer(AAC_OUTPUT_LENGTH);

    UCHAR* aacInputs[1] = { (UCHAR*) (buffer->data() + AAC_HEADER_LENGTH) };
    UINT aacInputSizes[1] = { (UINT) (buffer->size() - AAC_HEADER_LENGTH) };
    UINT aacBytesValid[1] = { (UINT) (buffer->size() - AAC_HEADER_LENGTH) };

	AAC_DECODER_ERROR result;
	while (aacBytesValid[0] > 0) {

		result = aacDecoder_Fill(mAacDecoder, aacInputs, aacInputSizes, aacBytesValid);
		if (result != AAC_DEC_OK) {
			printf("Error on aacDecoder_Fill: 0x%X\n", result);
			return NULL;
		}

		result = aacDecoder_DecodeFrame(mAacDecoder, mAacOutput, AAC_OUTPUT_LENGTH, /* flags */ 0);
		if (result == AAC_DEC_OK) {
			memcpy(decoded->data(), (uint8_t*) mAacOutput, AAC_OUTPUT_LENGTH);
		} else if (result != AAC_DEC_NOT_ENOUGH_BITS && result != AAC_DEC_TRANSPORT_SYNC_ERROR) {
			printf("Error on aacDecoder_DecodeFrame: 0x%X (after %d bytes)\n", result, (aacInputSizes[0] - aacBytesValid[0]));
			return NULL;
		}
	}

	return decoded;
}
