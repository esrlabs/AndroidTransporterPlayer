#include "NetHandler.h"
#include "RPiPlayer.h"
#include "Bundle.h"
#include "RtpAvcAssembler.h"
#include <stdio.h>

using namespace android::os;

NetHandler::NetHandler() {
}

NetHandler::~NetHandler() {
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SETUP_MEDIA_SOURCE: {
		Bundle* bundle = (Bundle*) message->obj;
		mPlayer = bundle->arg1;
		mRtspMediaSource = new RtspMediaSource();
		mRtspMediaSource->start(bundle->arg2, obtainMessage(SETUP_MEDIA_SOURCE_DONE));
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
			mRtpAudioSource = new RtpMediaSource();
			mRtpVideoSource = new RtpMediaSource();
			mRtpVideoSource->start(RTP_VIDEO_SOURCE_PORT, new RtpAvcAssembler(mRtpVideoSource->getBufferQueue(),
					mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_VIDEO_BUFFER)));
			mRtspMediaSource->setupVideoTrack(RTP_VIDEO_SOURCE_PORT, obtainMessage(SETUP_VIDEO_TRACK_DONE));
		}
		break;
	}
	case SETUP_VIDEO_TRACK_DONE: {
		if (message->arg1 == 0) {
			mRtspMediaSource->playVideoTrack(obtainMessage(PLAY_VIDEO_TRACK_DONE));
		}
		break;
	}
	case PLAY_VIDEO_TRACK_DONE: {
		break;
	}
	case STOP_MEDIA_SOURCE: {
		mRtspMediaSource->stop();
		mRtpAudioSource->stop();
		mRtpVideoSource->stop();
		Bundle* bundle = (Bundle*) message->obj;
		sp<Message> reply = bundle->reply;
		reply->sendToTarget();
		delete bundle;
		break;
	}
	}
}
