#include "RtpPcmAssembler.h"
#include "Buffer.h"
#include "android/os/Message.h"
#include "android/os/Clock.h"
#include <string.h>

using namespace android::os;
using namespace android::util;

RtpPcmAssembler::RtpPcmAssembler(android::util::List< sp<Buffer> >& queue, const sp<android::os::Message>& notifyAccessUnit) :
		mQueue(queue),
		mNotifyAccessUnit(notifyAccessUnit) {
}

RtpPcmAssembler::~RtpPcmAssembler() {
}

void RtpPcmAssembler::processMediaData() {
	if (!mQueue.empty()) {
		sp<Buffer> buffer = *mQueue.begin();
		mQueue.erase(mQueue.begin());

		const uint8_t* data = buffer->data();
		size_t size = buffer->size();
		sp<Buffer> accessUnit = new Buffer(size);
		memcpy(accessUnit->data(), data, size);

		sp<Message> msg = mNotifyAccessUnit->dup();
		msg->obj = new sp<Buffer>(accessUnit);
		msg->sendToTarget();
	}
}
