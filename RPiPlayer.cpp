#include "RPiPlayer.h"
#include <errno.h>
#include "Bundle.h"

using namespace android::os;

RPiPlayer::RPiPlayer() {
	mNetLooper =  new LooperThread<NetHandler>();
	mNetLooper->start();
}

RPiPlayer::~RPiPlayer() {
	mNetLooper->getLooper()->quit();
	mNetLooper->join();
}

void RPiPlayer::start(android::lang::String url) {
	setupMediaSource(url);
	Looper::loop();
}

void RPiPlayer::stop() {
}

void RPiPlayer::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SET_RTSP_MEDIA_SOURCE:
		mRtspMediaSource = (RtspMediaSource*) message->obj;
		break;
	case MEDIA_SOURCE_NOTIFY:
//		sp<Buffer> accessUnit;
//		int32_t result = mRtspMediaSource->dequeueBuffer((MediaSourceType)message->arg1, &accessUnit);
//		if (result == 0) {
//			// TODO: process video data
//		} else if (-EWOULDBLOCK) {
//			// TODO: make Message a Ref and use a special allocator for message pooling
//			sendMessageDelayed(message, 10);
//		}
		break;
	}
}

bool RPiPlayer::setupMediaSource(const android::lang::String& url) {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::SETUP_MEDIA_SOURCE);
	message->obj = new Bundle(this, url, obtainMessage(SET_RTSP_MEDIA_SOURCE));
	message->sendToTarget();
	return true;
}
