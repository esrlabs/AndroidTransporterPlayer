#include "NetHandler.h"
#include "RPiPlayer.h"
#include "Bundle.h"
#include <stdio.h>

using namespace android::os;

NetHandler::NetHandler() {
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SETUP_MEDIA_SOURCE: {
		Bundle* bundle = (Bundle*) message->obj;
		mPlayer = bundle->arg1;
		mRtspMediaSource = new RtspMediaSource();
		mRtspMediaSource->start(bundle->arg2, obtainMessage(SETUP_MEDIA_SOURCE_DONE));
		sp<Message> reply = bundle->reply;
		reply->obj = mRtspMediaSource.getPointer();
		reply->sendToTarget();
		delete bundle;
		break;
	}
	case SETUP_MEDIA_SOURCE_DONE: {
		if (message->arg1 == 0) {
			mRtspMediaSource->describeService(obtainMessage(DESCRIBE_SERVICE_DONE));
		}
		break;
	}
	case DESCRIBE_SERVICE_DONE: {
		if (message->arg1 == 0) {
			//TODO: the DESCRIBE_SERVICE_DONE message has to contain the SDP service desc within a bundle.
			mRtpVideoSource = new RtpMediaSource();
			mRtpVideoSource->start(RTP_VIDEO_SOURCE_PORT, mPlayer->obtainMessage(RPiPlayer::MEDIA_SOURCE_NOTIFY));
			mRtspMediaSource->setupTrack(obtainMessage(SETUP_TRACK_DONE));
		}
		break;
	}
	case SETUP_TRACK_DONE: {
		if (message->arg1 == 0) {
			mRtspMediaSource->playTrack(obtainMessage(PLAY_TRACK_DONE));
		}
		break;
	}
	case PLAY_TRACK_DONE:
		break;
	}
}
