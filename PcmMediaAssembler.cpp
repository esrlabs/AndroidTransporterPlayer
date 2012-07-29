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
	const uint8_t* data = buffer->data();
	size_t size = buffer->size();

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

uint32_t PcmMediaAssembler::getNextSeqNum() const {
	return (*mQueue->begin())->getId();
}

void PcmMediaAssembler::swapPcmDataEndianess(const sp<mindroid::Buffer>& buffer) {
	uint16_t* ptr = (uint16_t*) buffer->data();
	for (size_t i = 0; i < buffer->size(); i += 2) {
		*ptr = (*ptr >> 8) | ((*ptr	<< 8) & 0xFF00);
		ptr++;
	}
}
