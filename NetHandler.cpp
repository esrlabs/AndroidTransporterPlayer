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
#include "Utils.h"
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
			mRtpAudioSource = new RtpMediaSource(RTP_AUDIO_SOURCE_PORT);
			mRtpAudioSource->start(new PcmMediaAssembler(mRtpAudioSource->getMediaQueue(),
					mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_AUDIO_BUFFER)));
			sp<Message> msg = mRtspMediaSource->obtainMessage(RtspMediaSource::START_AUDIO_TRACK);
			msg->arg1 = RTP_AUDIO_SOURCE_PORT;
			msg->sendToTarget();
		} else if (audioType == AAC_AUDIO_TYPE_1 || audioType == AAC_AUDIO_TYPE_2) {
			mRtpAudioSource = new RtpMediaSource(RTP_AUDIO_SOURCE_PORT);
			mRtpAudioSource->start(new AacMediaAssembler(mRtpAudioSource->getMediaQueue(),
					new AacDecoder(message->metaData()->getString("CodecConfig"), mPlayer->obtainMessage(RPiPlayer::NOTIFY_QUEUE_AUDIO_BUFFER))));
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
		uint32_t videoType;
		message->metaData()->fillUInt32("Type", videoType);
		String transportProtocol;
		message->metaData()->fillString("TransportProtocol", transportProtocol);
		String serverHostName;
		message->metaData()->fillString("ServerHostName", serverHostName);
		uint16_t serverPort;
		message->metaData()->fillUInt16("ServerPorts", serverPort);

		String profileId;
		message->metaData()->fillString("ProfileId", profileId);
		String spropParams;
		message->metaData()->fillString("SpropParams", spropParams);
		buildCodecSpecificData(profileId, spropParams);

		if (videoType == AVC_VIDEO_TYPE_1 || videoType == AVC_VIDEO_TYPE_2) {
			if (transportProtocol == "UDP") {
				mRtpVideoSource = new RtpMediaSource(RTP_VIDEO_SOURCE_PORT);
			} else {
				mRtpVideoSource = new RtpMediaSource(serverHostName, 1742);
			}
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

sp<Buffer> NetHandler::buildCodecSpecificData(String profileId, String spropParams) {
	sp<Buffer> profileLevelId = Utils::hexStringToByteArray(profileId);

//	size_t numSeqParameterSets = 0;
//	size_t totalSeqParameterSetSize = 0;
//	size_t numPicParameterSets = 0;
//	size_t totalPicParameterSetSize = 0;
//
//	sp< List< sp<Buffer> > > paramSets = new List< sp<Buffer> >();
//
//	size_t start = 0;
//	for (;;) {
//		ssize_t commaPos = spropParams.indexOf(",", start);
//		size_t end = (commaPos < 0) ? spropParams.size() : commaPos;
//
//		String nalString(spropParams, start, end - start);
//		sp<Buffer> nal = Utils::decodeBase64(nalString);
//
//		uint8_t nalType = nal->data()[0] & 0x1f;
//		if (numSeqParameterSets == 0) {
//		} else if (numPicParameterSets > 0) {
//		}
//		if (nalType == 7) {
//			++numSeqParameterSets;
//			totalSeqParameterSetSize += nal->size();
//		} else  {
//			++numPicParameterSets;
//			totalPicParameterSetSize += nal->size();
//		}
//
//		paramSets->push_back(nal);
//
//		if (commaPos < 0) {
//			break;
//		}
//
//		start = commaPos + 1;
//	}
//
//	size_t csdSize =
//		1 + 3 + 1 + 1
//		+ 2 * numSeqParameterSets + totalSeqParameterSetSize
//		+ 1 + 2 * numPicParameterSets + totalPicParameterSetSize;
//
//	sp<Buffer> csd = new Buffer(csdSize);
//	uint8_t *out = csd->data();
//
//	*out++ = 0x01;  // configurationVersion
//	memcpy(out, profileLevelId->data(), 3);
//	out += 3;
//	*out++ = (0x3f << 2) | 1;  // lengthSize == 2 bytes
//	*out++ = 0xe0 | numSeqParameterSets;
//
//	for (size_t i = 0; i < numSeqParameterSets; ++i) {
//		sp<Buffer> nal = paramSets.editItemAt(i);
//
//		*out++ = nal->size() >> 8;
//		*out++ = nal->size() & 0xff;
//
//		memcpy(out, nal->data(), nal->size());
//
//		out += nal->size();
//
//		if (i == 0) {
//			FindAVCDimensions(nal, width, height);
//		}
//	}
//
//	*out++ = numPicParameterSets;
//
//	for (size_t i = 0; i < numPicParameterSets; ++i) {
//		sp<Buffer> nal = paramSets.editItemAt(i + numSeqParameterSets);
//
//		*out++ = nal->size() >> 8;
//		*out++ = nal->size() & 0xff;
//
//		memcpy(out, nal->data(), nal->size());
//
//		out += nal->size();
//	}
//
//	// hexdump(csd->data(), csd->size());
//
//	return csd;

	return NULL;
}
