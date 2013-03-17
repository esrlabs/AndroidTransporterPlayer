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
#include "PcmMediaAssembler.h"
#include "AacMediaAssembler.h"
#include "AacDecoder.h"
#include "AvcMediaAssembler.h"
#include "CsdUtils.h"
#include "mindroid/os/Bundle.h"
#include "mindroid/lang/String.h"
#include "mindroid/util/Buffer.h"
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
		uint32_t audioType;
		message->metaData()->fillUInt32("Type", audioType);
		if (audioType == PCM_AUDIO_TYPE) {
			mRtpAudioSource = new RtpMediaSource(new RtpMediaSource::UdpNetReceiver(RTP_AUDIO_SOURCE_PORT));
			mRtpAudioSource->start(new PcmMediaAssembler(mRtpAudioSource->getMediaQueue(),
					mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_AUDIO_BUFFER)));

			sp<Message> msg = mRtspMediaSource->obtainMessage(RtspMediaSource::PLAY_AUDIO_TRACK);
			msg->sendToTarget();
		} else if (audioType == AAC_AUDIO_TYPE_1 || audioType == AAC_AUDIO_TYPE_2) {
			mRtpAudioSource = new RtpMediaSource(new RtpMediaSource::UdpNetReceiver(RTP_AUDIO_SOURCE_PORT));
			mRtpAudioSource->start(new AacMediaAssembler(mRtpAudioSource->getMediaQueue(),
					new AacDecoder(message->metaData()->getString("CodecConfig"), mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_AUDIO_BUFFER))));

			sp<Message> msg = mRtspMediaSource->obtainMessage(RtspMediaSource::PLAY_AUDIO_TRACK);
			msg->sendToTarget();
		}
		break;
	}
	case STOP_AUDIO_TRACK: {
		obtainMessage(STOP_MEDIA_SOURCE)->sendToTarget();
		break;
	}
	case START_VIDEO_TRACK: {
		uint32_t videoType;
		message->metaData()->fillUInt32("Type", videoType);
		sp<String> transportProtocol;
		message->metaData()->fillString("TransportProtocol", transportProtocol);
		sp<String> serverIpAddress;
		message->metaData()->fillString("ServerIpAddress", serverIpAddress);
		uint16_t serverPort;
		message->metaData()->fillUInt16("ServerPorts", serverPort);
		sp<String> profileId;
		message->metaData()->fillString("ProfileId", profileId);
		sp<String> spropParams;
		message->metaData()->fillString("SpropParams", spropParams);

		if (videoType == AVC_VIDEO_TYPE_1 || videoType == AVC_VIDEO_TYPE_2) {
			sp<Buffer> sps;
			sp<Buffer> pps;
			CsdUtils::buildAvcCodecSpecificData(profileId, spropParams, &sps, &pps);

			sp<Message> spsMessage = mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_VIDEO_BUFFER);
			sp<Bundle> spsBundle = spsMessage->metaData();
			spsBundle->putObject("Access-Unit", sps);
			spsMessage->sendToTarget();

			sp<Message> ppsMessage = mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_VIDEO_BUFFER);
			sp<Bundle> ppsBundle = ppsMessage->metaData();
			ppsBundle->putObject("Access-Unit", pps);
			ppsMessage->sendToTarget();

			if (transportProtocol->equals("UDP")) {
				mRtpVideoSource = new RtpMediaSource(new RtpMediaSource::UdpNetReceiver(RTP_VIDEO_SOURCE_PORT));
			} else {
				mRtpVideoSource = new RtpMediaSource(new RtpMediaSource::TcpNetReceiver(serverIpAddress, serverPort));
			}
			mRtpVideoSource->start(new AvcMediaAssembler(mRtpVideoSource->getMediaQueue(), mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_VIDEO_BUFFER)));

			sp<Message> msg = mRtspMediaSource->obtainMessage(RtspMediaSource::PLAY_VIDEO_TRACK);
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
