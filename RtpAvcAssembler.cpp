#include "RtpAvcAssembler.h"
#include "RtpMediaSource.h"
#include "Buffer.h"
#include "android/os/Message.h"
#include "android/os/Clock.h"
#include <string.h>
#include <stdio.h>

using namespace android::os;

RtpAvcAssembler::RtpAvcAssembler(List< sp<Buffer> >& queue, const sp<Message>& accessUnitNotifyMessage) :
		mQueue(queue),
		mAccessUnitNotifyMessage(accessUnitNotifyMessage),
		mSeqNumber(0),
		mInitSeqNumber(true),
		mFirstSeqNumberFailureTime(0) {
}

RtpAvcAssembler::~RtpAvcAssembler() {
}

void RtpAvcAssembler::processMediaData() {
    Status status;

    while (true) {
        status = assembleMediaData();

        if (status == OK) {
        	mFirstSeqNumberFailureTime = 0;
			break;
		} else if (status == SEQ_NUMBER_FAILURE) {
            if (mFirstSeqNumberFailureTime != 0) {
                if (Clock::monotonicTime() - mFirstSeqNumberFailureTime > TIME_PERIOD_20MS) {
                	mFirstSeqNumberFailureTime = 0;
                	// We lost that packet. Empty the NAL unit queue.
                	sp<Buffer> buffer = *mQueue.begin();
                	mSeqNumber = buffer->getMetaData();
                    continue;
                }
            } else {
            	mFirstSeqNumberFailureTime = Clock::monotonicTime();
            }
            break;
        } else {
        	mFirstSeqNumberFailureTime = 0;
        }
    }
}

RtpAvcAssembler::Status RtpAvcAssembler::assembleMediaData() {
	if (mQueue.empty()) {
		return OK;
	}

	sp<Buffer> buffer = *mQueue.begin();
	const uint8_t* data = buffer->data();
	size_t size = buffer->size();

	if (size < 1 || (data[0] & F_BIT)) {
		mQueue.erase(mQueue.begin());
		mSeqNumber++;
		return PACKET_FAILURE;
	}

	if (!mInitSeqNumber) {
		List< sp<Buffer> >::iterator itr = mQueue.begin();
		while (itr != mQueue.end()) {
			if ((uint32_t)(*itr)->getMetaData() > mSeqNumber) {
				break;
			}

			itr = mQueue.erase(itr);
		}

		if (mQueue.empty()) {
			return OK;
		}
	}

	if (mInitSeqNumber) {
		mSeqNumber = (uint32_t) buffer->getMetaData();
		mInitSeqNumber = false;
	} else {
		if ((uint32_t)buffer->getMetaData() != mSeqNumber + 1) {
			return SEQ_NUMBER_FAILURE;
		}
	}

	unsigned nalUnitType = data[0] & 0x1F;
	if (nalUnitType >= 1 && nalUnitType <= 23) {
		processSingleNalUnit(buffer);
		mQueue.erase(mQueue.begin());
		mSeqNumber++;
		return OK;
	} else if (nalUnitType == 28) {
		// FU-A
		return processFragNalUnit();
	} else if (nalUnitType == 24) {
		// STAP-A
		assert(false);
		mQueue.erase(mQueue.begin());
		mSeqNumber++;
		return OK;
	} else {
		printf("Unknown NAL unit type: %d\n", nalUnitType);
		mQueue.erase(mQueue.begin());
		mSeqNumber++;
		return OK;
	}
	return OK;
}

void RtpAvcAssembler::processSingleNalUnit(sp<Buffer> nalUnit) {
	sp<Buffer> accessUnit = new Buffer(nalUnit->size() + 4);
	size_t offset = 0;
	memcpy(accessUnit->data(), "\x00\x00\x00\x01", 4);
	offset += 4;
	memcpy(accessUnit->data() + offset, nalUnit->data(), nalUnit->size());

	sp<Message> msg = mAccessUnitNotifyMessage->dup();
	msg->obj = new sp<Buffer>(accessUnit);
	msg->sendToTarget();
}

RtpAvcAssembler::Status RtpAvcAssembler::processFragNalUnit() {
	sp<Buffer> buffer = *mQueue.begin();
	const uint8_t *data = buffer->data();
	size_t size = buffer->size();

	if (size < 2) {
		mQueue.erase(mQueue.begin());
		mSeqNumber++;
		assert(false);
		return PACKET_FAILURE;
	}

	if (!(data[1] & FU_START_BIT)) {
		mQueue.erase(mQueue.begin());
		mSeqNumber++;
		return PACKET_FAILURE;
	}

	uint8_t fuIndicator = data[0];
	uint32_t nalUnitType = data[1] & 0x1F;
	uint32_t nri = (data[0] >> 5) & 3;

	uint32_t curSeqNumber = (uint32_t)buffer->getMetaData();
	bool accessUnitDone = false;
	size_t accessUnitSize = size - 2;
	size_t fuNalUnitCount = 1;

	if (data[1] & FU_END_BIT) {
		accessUnitDone = true;
	} else {
		List< sp<Buffer> >::iterator itr = ++mQueue.begin();
		while (itr != mQueue.end()) {
			const sp<Buffer>& curBuffer = *itr;
			const uint8_t* curData = curBuffer->data();
			size_t curSize = curBuffer->size();

			curSeqNumber++;
			if ((uint32_t)curBuffer->getMetaData() != curSeqNumber) {
				return SEQ_NUMBER_FAILURE;
			}

			// TODO: Correctly handle the following conditions.
			if (curSize < 2 ||
					curData[0] != fuIndicator ||
					(curData[1] & 0x1F) != nalUnitType ||
					(curData[1] & FU_START_BIT)) {

				// Delete the access unit up to but not containing the current FU NAL unit.
				itr = mQueue.begin();
				for (size_t i = 0; i <= fuNalUnitCount; ++i) {
					itr = mQueue.erase(itr);
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

	List< sp<Buffer> >::iterator itr = mQueue.begin();
	for (size_t i = 0; i < fuNalUnitCount; i++) {
		const sp<Buffer>& curBuffer = *itr;
		memcpy(accessUnit->data() + offset, curBuffer->data() + 2, curBuffer->size() - 2);
		offset += (curBuffer->size() - 2);
		itr = mQueue.erase(itr);
	}
	accessUnit->setRange(0, accessUnitSize);

	sp<Message> msg = mAccessUnitNotifyMessage->dup();
	msg->obj = new sp<Buffer>(accessUnit);
	msg->sendToTarget();

	return OK;
}
