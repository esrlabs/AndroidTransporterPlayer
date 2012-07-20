#include "NetHandler.h"
#include "RPiPlayer.h"
#include "android/os/Bundle.h"
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
		sp<Bundle> bundle = message->getData();
		mPlayer = bundle->getObject<Handler>("Player");
		mRtspMediaSource = new RtspMediaSource();
		mRtspMediaSource->start(bundle->getString("Url"), obtainMessage(SETUP_MEDIA_SOURCE_DONE));
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
//			mRtpAudioSource = new RtpMediaSource();
			mRtpVideoSource = new RtpMediaSource(RTP_VIDEO_SOURCE_PORT);
			mRtpVideoSource->start(new RtpAvcAssembler(mRtpVideoSource->getMediaQueue(),
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
//		mRtpAudioSource->stop();
		mRtpVideoSource->stop();
		sp<Bundle> bundle = message->getData();
		sp<Message> reply = bundle->getObject<Message>("Reply");
		reply->sendToTarget();
		break;
	}
	}
}
