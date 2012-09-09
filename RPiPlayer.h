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

#ifndef RPIPLAYER_H_
#define RPIPLAYER_H_

#include <mindroid/os/LooperThread.h>
#include <mindroid/lang/String.h>
#include <mindroid/util/List.h>
#include "NetHandler.h"
extern "C" {
#include "bcm_host.h"
#include "ilclient.h"
}

namespace mindroid {
class Buffer;
}

using mindroid::sp;

class RPiPlayer :
	public mindroid::Handler
{
public:
	static const uint32_t NOTIFY_QUEUE_AUDIO_BUFFER = 1;
	static const uint32_t NOTIFY_QUEUE_VIDEO_BUFFER = 2;
	static const uint32_t NOTIFY_PLAY_AUDIO_BUFFER = 3;
	static const uint32_t NOTIFY_PLAY_VIDEO_BUFFER = 4;
	static const uint32_t STOP_MEDIA_SOURCE_DONE = 5;
	static const uint32_t NOTIFY_OMX_EMPTY_BUFFER_DONE = 6;

	RPiPlayer();
	virtual ~RPiPlayer();
	bool start(mindroid::String url);
	void stop();

	virtual void handleMessage(const sp<mindroid::Message>& message);

private:
	bool startMediaSource(const mindroid::String& url);
	void stopMediaSource();
	int initOMXAudio();
	bool setAudioSink(const char* sinkName);
	void finalizeOMXAudio();
	int initOMXVideo();
	void finalizeOMXVideo();
	void onPlayAudioBuffers();
	void onFillAndPlayAudioBuffers();
	void onPlayVideoBuffers();
	static void onEmptyBufferDone(void* args, COMPONENT_T* component);
	bool minNumAudioSamplesAvailable();
	void calcNumAudioStretchSamples();
	uint32_t numOmxOwnedAudioSamples();

	sp< mindroid::LooperThread<NetHandler> > mNetLooper;
	sp< mindroid::List< sp<mindroid::Buffer> > > mAudioBuffers;
	sp< mindroid::List< sp<mindroid::Buffer> > > mVideoBuffers;

	// Audio
	static const uint32_t SAMPLE_RATE = 43932; // Hz, should be 44100Hz but the RPi hardware is too fast.
	static const uint32_t NUM_CHANNELS = 2;
	static const uint32_t BITS_PER_SAMPLE = 16;
	static const uint32_t BYTES_PER_SAMPLE = BITS_PER_SAMPLE / 8;
	static const uint32_t NUM_OMX_AUDIO_BUFFERS = 8;
	static const uint32_t OMX_AUDIO_BUFFER_SIZE = 4096; // 2048 samples -> 46ms
	static const uint32_t MIN_FILLED_AUDIO_BUFFERS_AT_START = 3;
	ILCLIENT_T* mAudioClient;
	COMPONENT_T* mAudioRenderer;
	COMPONENT_T* mAudioComponentList[2];
	OMX_BUFFERHEADERTYPE* mAudioBuffer;
	bool mFirstAudioPacket;
	sp< mindroid::List< OMX_BUFFERHEADERTYPE* > > mOmxAudioInputBuffers;
	sp< mindroid::List< OMX_BUFFERHEADERTYPE* > > mOmxAudioEmptyBuffers;
	size_t mNumAudioStretchSamples;

	// Video
	ILCLIENT_T* mVideoClient;
	TUNNEL_T mTunnel[4];
	COMPONENT_T* mVideoComponentList[5];
	COMPONENT_T* mVideoDecoder;
	COMPONENT_T* mVideoScheduler;
	COMPONENT_T* mVideoRenderer;
	COMPONENT_T* mClock;
	bool mPortSettingsChanged;
	bool mFirstVideoPacket;
};

#endif /* RPIPLAYER_H_ */
