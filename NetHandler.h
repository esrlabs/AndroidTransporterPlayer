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

#ifndef NETHANDLER_H_
#define NETHANDLER_H_

#include "mindroid/os/Handler.h"
#include "mindroid/os/LooperThread.h"
#include "RtspMediaSource.h"
#include "RtpMediaSource.h"

class RPiPlayer;

using mindroid::sp;

class NetHandler :
	public mindroid::Handler
{
public:
	static const uint32_t START_MEDIA_SOURCE = 0;
	static const uint32_t STOP_MEDIA_SOURCE = 1;
	static const uint32_t START_VIDEO_TRACK = 2;
	static const uint32_t STOP_VIDEO_TRACK = 3;
	static const uint32_t START_AUDIO_TRACK = 4;
	static const uint32_t STOP_AUDIO_TRACK = 5;

	NetHandler();
	~NetHandler();
	virtual void handleMessage(const sp<mindroid::Message>& message);

private:
	static const uint16_t RTP_AUDIO_SOURCE_PORT = 56096;
	static const uint16_t RTP_VIDEO_SOURCE_PORT = 56098;
	static const uint8_t PCM_AUDIO_TYPE = 10;
	static const uint8_t AVC_VIDEO_TYPE = 96;

	sp<RPiPlayer> mPlayer;
	sp<RtspMediaSource> mRtspMediaSource;
	sp<RtpMediaSource> mRtpAudioSource;
	sp<RtpMediaSource> mRtpVideoSource;
};

#endif /* NETHANDLER_H_ */
