#include "NetHandler.h"
#include "RPiPlayer.h"
#include "Bundle.h"
#include <stdio.h>

using namespace android::os;

NetHandler::NetHandler() :
	mPlayer(NULL),
	mRtspMediaSourceLooper(new LooperThread<RtspMediaSource>()) {
	mRtspMediaSourceLooper->start();
	mRtspMediaSource = mRtspMediaSourceLooper->getHandler();
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SETUP_MEDIA_SOURCE: {
		Bundle* bundle = (Bundle*) message->obj;
		mRtspMediaSource->setupMediaSource(this, bundle->arg2, obtainMessage(SETUP_MEDIA_SOURCE_DONE));
		sp<Message> reply = bundle->reply;
		reply->obj = mRtspMediaSource.getPointer();
		reply->sendToTarget();
		delete bundle;
		break;
	}
	case SETUP_MEDIA_SOURCE_DONE: {
		mRtspMediaSource->describeService();
	}
	}
}
