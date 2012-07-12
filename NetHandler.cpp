#include "NetHandler.h"
#include "RPiPlayer.h"
#include "Bundle.h"
#include <stdio.h>

using namespace android::os;

NetHandler::NetHandler() :
	mPlayer(NULL) {
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SET_MEDIA_SOURCE: {
		Bundle* bundle = (Bundle*) message->obj;
		mRtspMediaSource = new RtspMediaSource(bundle->arg1, bundle->arg2);
		delete bundle;
		mRtspMediaSource->start(obtainMessage(RTSP_CLIENT_CONNECTED));
		sp<Message> reply = mPlayer->obtainMessage(RPiPlayer::SET_RTSP_MEDIA_SOURCE);
		reply->obj = mRtspMediaSource.getPointer();
		reply->sendToTarget();
		break;
	}
	case RTSP_CLIENT_CONNECTED: {
		int32_t result = message->arg1;
		if (result == 0) {
			mRtspMediaSource->describe();
		} else {
			printf("Cannot connect to RTSP server\n");
		}
		break;
	}
	}
}
