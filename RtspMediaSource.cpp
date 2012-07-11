#include "RtspMediaSource.h"
#include <errno.h>

using namespace android::os;

RtspMediaSource::RtspMediaSource(const sp<Handler>& player) :
	mPlayer(player) {
}

RtspMediaSource::~RtspMediaSource() {
}

void RtspMediaSource::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	}
}

int32_t RtspMediaSource::dequeueBuffer(MediaSourceType type , android::os::sp<Buffer>* accessUnit) {
	sp<RtpMediaSource> mediaSource = getMediaSource(type);

	if (mediaSource == NULL) {
		return -EWOULDBLOCK;
	}

	if (mediaSource->empty()) {
		return -EWOULDBLOCK;
	}

	return mediaSource->dequeueAccessUnit(accessUnit);
}
