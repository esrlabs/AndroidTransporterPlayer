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

#include "AacMediaAssembler.h"
#include "AacDecoder.h"
#include <mindroid/os/Message.h>
#include <mindroid/util/Buffer.h>

using namespace mindroid;

AacMediaAssembler::AacMediaAssembler(sp< List< sp<Buffer> > > queue, sp<AacDecoder> aacDecoder) :
		mQueue(queue),
		mAacDecoder(aacDecoder) {
}

AacMediaAssembler::~AacMediaAssembler() {
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

	if (buffer->size() < 5) {
		mQueue->erase(mQueue->begin());
		++mSeqNumber;
		return PACKET_FAILURE;
	}

	mQueue->erase(mQueue->begin());
	mSeqNumber++;

	mAacDecoder->processBuffer(buffer);

	return OK;
}

uint32_t AacMediaAssembler::fixPacketLoss() const {
	return (*mQueue->begin())->getId();
}
