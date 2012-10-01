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

#include "PcmMediaAssembler.h"
#include <mindroid/os/Message.h>
#include <mindroid/util/Buffer.h>
#include <mindroid/os/Clock.h>
#include <string.h>
#include <stdio.h>

using namespace mindroid;

PcmMediaAssembler::PcmMediaAssembler(sp< List< sp<Buffer> > > queue, const sp<Message>& notifyBuffer) :
	mQueue(queue),
	mNotifyBuffer(notifyBuffer) {
}

PcmMediaAssembler::~PcmMediaAssembler() {
}

MediaAssembler::Status PcmMediaAssembler::assembleMediaData() {
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

	swapPcmDataEndianess(buffer);

	sp<Message> msg = mNotifyBuffer->dup();
	sp<Bundle> bundle = msg->metaData();
	bundle->putObject("Buffer", buffer);
	msg->sendToTarget();

	mQueue->erase(mQueue->begin());
	mSeqNumber++;

	return OK;
}

uint32_t PcmMediaAssembler::fixPacketLoss() const {
	return (*mQueue->begin())->getId();
}

void PcmMediaAssembler::swapPcmDataEndianess(const sp<mindroid::Buffer>& buffer) {
	uint16_t* ptr = (uint16_t*) buffer->data();
	for (size_t i = 0; i < buffer->size(); i += 2) {
		*ptr = (*ptr >> 8) | ((*ptr << 8) & 0xFF00);
		ptr++;
	}
}
