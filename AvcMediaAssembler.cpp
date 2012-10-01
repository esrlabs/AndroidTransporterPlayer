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

#include "AvcMediaAssembler.h"
#include "RtpMediaSource.h"
#include "mindroid/os/Message.h"
#include "mindroid/os/Clock.h"
#include "mindroid/util/Buffer.h"
#include <string.h>
#include <stdio.h>

using namespace mindroid;

AvcMediaAssembler::AvcMediaAssembler(sp< List< sp<Buffer> > > queue, const sp<Message>& notifyAccessUnit) :
		mQueue(queue),
		mNotifyAccessUnit(notifyAccessUnit) {
}

AvcMediaAssembler::~AvcMediaAssembler() {
}

MediaAssembler::Status AvcMediaAssembler::assembleMediaData() {
	if (mQueue->empty()) {
		return OK;
	}

	sp<Buffer> buffer = *mQueue->begin();
	const uint8_t* data = buffer->data();
	size_t size = buffer->size();

	if (size < 1 || (data[0] & F_BIT)) {
		mQueue->erase(mQueue->begin());
		mSeqNumber++;
		return PACKET_FAILURE;
	}

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

	unsigned nalUnitType = data[0] & 0x1F;
	if (nalUnitType >= 1 && nalUnitType <= 23) {
		processSingleNalUnit(buffer);
		mQueue->erase(mQueue->begin());
		mSeqNumber++;
		return OK;
	} else if (nalUnitType == 28) {
		// FU-A
		return processFragNalUnit();
	} else if (nalUnitType == 24) {
		// STAP-A
		assert(false);
		mQueue->erase(mQueue->begin());
		mSeqNumber++;
		return OK;
	} else {
		printf("Unknown NAL unit type: %d\n", nalUnitType);
		mQueue->erase(mQueue->begin());
		mSeqNumber++;
		return OK;
	}
	return OK;
}

void AvcMediaAssembler::processSingleNalUnit(sp<Buffer> nalUnit) {
	sp<Buffer> accessUnit = new Buffer(nalUnit->size() + 4);
	size_t offset = 0;
	memcpy(accessUnit->data(), "\x00\x00\x00\x01", 4);
	offset += 4;
	memcpy(accessUnit->data() + offset, nalUnit->data(), nalUnit->size());

	sp<Message> msg = mNotifyAccessUnit->dup();
	sp<Bundle> bundle = msg->metaData();
	bundle->putObject("Access-Unit", accessUnit);
	uint32_t rtpTime = nalUnit->metaData()->getUInt32("RTP-Time", 0);
	bundle->putUInt32("RTP-Time", rtpTime);
	msg->sendToTarget();
}

AvcMediaAssembler::Status AvcMediaAssembler::processFragNalUnit() {
	sp<Buffer> buffer = *mQueue->begin();
	const uint8_t *data = buffer->data();
	size_t size = buffer->size();

	if (size < 2) {
		mQueue->erase(mQueue->begin());
		mSeqNumber++;
		assert(false);
		return PACKET_FAILURE;
	}

	if (!(data[1] & FU_START_BIT)) {
		mQueue->erase(mQueue->begin());
		mSeqNumber++;
		return PACKET_FAILURE;
	}

	uint8_t fuIndicator = data[0];
	uint32_t nalUnitType = data[1] & 0x1F;
	uint32_t nri = (data[0] >> 5) & 3;

	uint32_t curSeqNumber = (uint32_t)buffer->getId();
	bool accessUnitDone = false;
	size_t accessUnitSize = size - 2;
	size_t fuNalUnitCount = 1;

	if (data[1] & FU_END_BIT) {
		accessUnitDone = true;
	} else {
		List< sp<Buffer> >::iterator itr = ++mQueue->begin();
		while (itr != mQueue->end()) {
			const sp<Buffer>& curBuffer = *itr;
			const uint8_t* curData = curBuffer->data();
			size_t curSize = curBuffer->size();

			curSeqNumber++;
			if ((uint32_t)curBuffer->getId() != curSeqNumber) {
				return SEQ_NUMBER_FAILURE;
			}

			// TODO: Correctly handle the following conditions.
			if (curSize < 2 ||
					curData[0] != fuIndicator ||
					(curData[1] & 0x1F) != nalUnitType ||
					(curData[1] & FU_START_BIT)) {

				// Delete the access unit up to but not containing the current FU NAL unit.
				itr = mQueue->begin();
				for (size_t i = 0; i <= fuNalUnitCount; ++i) {
					itr = mQueue->erase(itr);
				}

				mSeqNumber = curSeqNumber;
				return PACKET_FAILURE;
			}

			accessUnitSize += curSize - 2;
			fuNalUnitCount++;

			if (curData[1] & FU_END_BIT) {
				accessUnitDone = true;
				break;
			}

			++itr;
		}
	}

	if (!accessUnitDone) {
		return OK;
	}

	mSeqNumber = curSeqNumber;

	// H.264 start code and NAL unit type header
	accessUnitSize += 5;
	sp<Buffer> accessUnit(new Buffer(accessUnitSize));
	size_t offset = 0;
	memcpy(accessUnit->data(), "\x00\x00\x00\x01", 4);
	offset += 4;
	accessUnit->data()[offset] = (nri << 5) | nalUnitType;
	offset++;

	List< sp<Buffer> >::iterator itr = mQueue->begin();
	uint32_t rtpTime = (*itr)->metaData()->getUInt32("RTP-Time", 0);
	for (size_t i = 0; i < fuNalUnitCount; i++) {
		const sp<Buffer>& curBuffer = *itr;
		memcpy(accessUnit->data() + offset, curBuffer->data() + 2, curBuffer->size() - 2);
		offset += (curBuffer->size() - 2);
		itr = mQueue->erase(itr);
	}
	accessUnit->setRange(0, accessUnitSize);

	sp<Message> msg = mNotifyAccessUnit->dup();
	sp<Bundle> bundle = msg->metaData();
	bundle->putObject("Access-Unit", accessUnit);
	bundle->putUInt32("RTP-Time", rtpTime);
	msg->sendToTarget();

	return OK;
}

uint32_t AvcMediaAssembler::fixPacketLoss() const {
	sp<Buffer> lastBuffer = *(--mQueue->end());
	mQueue->clear();
	mQueue->push_back(lastBuffer);
	return (*mQueue->begin())->getId();
}
