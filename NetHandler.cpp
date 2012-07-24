#include "NetHandler.h"
#include "RPiPlayer.h"
#include "android/os/Bundle.h"
#include "AvcMediaAssembler.h"
#include <stdio.h>

using namespace android::os;

NetHandler::NetHandler() {
}

NetHandler::~NetHandler() {
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case START_MEDIA_SOURCE: {
		sp<Bundle> bundle = message->getData();
		mPlayer = bundle->getObject<RPiPlayer>("Player");
		mRtspMediaSource = new RtspMediaSource(this);
		if (!mRtspMediaSource->start(bundle->getString("Url"))) {
			mPlayer->stop();
		}
		break;
	}
	case START_VIDEO_TRACK: {
		if (message->arg1 == AVC_VIDEO_TYPE) {
			mRtpVideoSource = new RtpMediaSource(RTP_VIDEO_SOURCE_PORT);
			mRtpVideoSource->start(new AvcMediaAssembler(mRtpVideoSource->getMediaQueue(),
					mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_VIDEO_BUFFER)));
			sp<Message> msg = mRtspMediaSource->obtainMessage(RtspMediaSource::START_VIDEO_TRACK);
			msg->arg1 = RTP_VIDEO_SOURCE_PORT;
			msg->sendToTarget();
		}
		break;
	}
	case STOP_MEDIA_SOURCE: {
		mRtspMediaSource->stop();
		if (mRtpVideoSource != NULL) {
			mRtpVideoSource->stop();
		}
		sp<Bundle> bundle = message->getData();
		sp<Message> reply = bundle->getObject<Message>("Reply");
		reply->sendToTarget();
		break;
	}
	}
}
