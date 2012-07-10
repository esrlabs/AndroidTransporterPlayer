#include "RPiPlayer.h"
#include <errno.h>

using namespace android::os;

RPiPlayer::RPiPlayer() {
	mNetLooper =  new LooperThread<NetHandler>();
}

RPiPlayer::~RPiPlayer() {
	mNetLooper->getLooper()->quit();
	mNetLooper->join();
}

void RPiPlayer::start() {
	mNetLooper->start();
	Looper::prepare();
	Looper::loop();
}

void RPiPlayer::handleMessage(Message& message) {
	switch (message.what) {
	case SET_DATA_SOURCE:
		mRtspMediaSource = (RtspMediaSource*) message.obj;
		break;
	case MEDIA_SOURCE_NOTIFY:
		sp<Buffer> accessUnit;
		int32_t result = mRtspMediaSource->dequeueBuffer((MediaSourceType)message.arg1, &accessUnit);
		if (result == 0) {
			// TODO: process video data
		} else if (-EWOULDBLOCK) {
			// TODO: make Message a Ref and use a special allocator for message pooling
			sendMessageDelayed(obtainMessage(message.what), 10);
		}
		break;
	}
}

bool RPiPlayer::setupMediaSource() {
	Message& message = mNetLooper->getHandler()->obtainMessage(NetHandler::SETUP_MEDIA_SOURCE);
	message.obj = this;
	message.sendToTarget();
	return true;
}
