/*
 * Copyright (C) 2012 E.S.R.Labs GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NetHandler.h"
#include "RPiPlayer.h"
#include "mindroid/os/Bundle.h"
#include "PcmMediaAssembler.h"
#include "AvcMediaAssembler.h"
#include <stdio.h>
#include <sys/resource.h>

using namespace mindroid;

NetHandler::NetHandler() {
}

NetHandler::~NetHandler() {
}

void NetHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case START_MEDIA_SOURCE: {
		setpriority(PRIO_PROCESS, 0, -17);
		sp<Bundle> bundle = message->metaData();
		mPlayer = bundle->getObject<RPiPlayer>("Player");
		mRtspMediaSource = new RtspMediaSource(this);
		if (!mRtspMediaSource->start(bundle->getString("Url"))) {
			mPlayer->stop();
		}
		break;
	}
	case START_AUDIO_TRACK: {
		if (message->arg1 == PCM_AUDIO_TYPE) {
			mRtpAudioSource = new RtpMediaSource(RTP_AUDIO_SOURCE_PORT);
			mRtpAudioSource->start(new PcmMediaAssembler(mRtpAudioSource->getMediaQueue(),
					mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_AUDIO_BUFFER)));
			sp<Message> msg = mRtspMediaSource->obtainMessage(RtspMediaSource::START_AUDIO_TRACK);
			msg->arg1 = RTP_AUDIO_SOURCE_PORT;
			msg->sendToTarget();
		}
		break;
	}
	case STOP_AUDIO_TRACK: {
		obtainMessage(STOP_MEDIA_SOURCE)->sendToTarget();
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
	case STOP_VIDEO_TRACK: {
		obtainMessage(STOP_MEDIA_SOURCE)->sendToTarget();
		break;
	}
	case STOP_MEDIA_SOURCE: {
		sp<Bundle> bundle = message->metaData();
		sp<Message> reply = bundle->getObject<Message>("Reply");
		mRtspMediaSource->stop(reply);
		if (mRtpAudioSource != NULL) {
			mRtpAudioSource->stop();
		}
		if (mRtpVideoSource != NULL) {
			mRtpVideoSource->stop();
		}
		break;
	}
	case MEDIA_SOURCE_HAS_NO_STREAMS:
	case MEDIA_SOURCE_HAS_QUIT:
		mPlayer->stop();
		break;
	}
}
