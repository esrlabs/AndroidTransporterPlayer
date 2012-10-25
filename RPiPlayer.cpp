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

#include "RPiPlayer.h"
#include <mindroid/os/Bundle.h>
#include <mindroid/util/Buffer.h>
#include <mindroid/os/Clock.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

using namespace mindroid;

RPiPlayer::RPiPlayer() :
		mAudioClient(NULL),
		mAudioRenderer(NULL),
		mAudioBuffer(NULL),
		mBufferingAudio(false),
		mNumAudioStretchSamples(0),
		mVideoClient(NULL),
		mVideoDecoder(NULL),
		mVideoScheduler(NULL),
		mVideoRenderer(NULL),
		mClock(NULL),
		mPortSettingsChanged(false),
		mFirstVideoPacket(true),
		mShutdown(false),
        mHelpBuffer(new Buffer(OMX_AUDIO_BUFFER_SIZE)),
		mFile(fopen("out.raw", "wb"))
{
	mAudioBuffers = new List< sp<Buffer> >();
	mVideoBuffers = new List< sp<Buffer> >();
	mOmxAudioInputBuffers = new List<OMX_BUFFERHEADERTYPE*>();
	mOmxAudioEmptyBuffers = new List<OMX_BUFFERHEADERTYPE*>();
	mNetLooper =  new LooperThread<NetHandler>();
	mNetLooper->start();
}


void RPiPlayer::writeToFile(uint8_t* data, size_t size) {
	fwrite(data, size, 1, mFile);
}


RPiPlayer::~RPiPlayer() {
	fclose(mFile);
}

bool RPiPlayer::start(String url) {
	bcm_host_init();

	if (OMX_Init() != OMX_ErrorNone) {
		return false;
	}
	assert(initOMXAudio() == 0);
	assert(setAudioSink("local"));
	assert(initOMXVideo() == 0);

	assert(startMediaSource(url));
	return true;
}

void RPiPlayer::stop() {
	stopMediaSource();
}


class ByteRate {
public:
    ByteRate(int delta) :
            mBytes(0), mDelta(delta), mStartTime(0) {
    }

    void addData(size_t bytes) {
        if (!mStartTime) {
            mStartTime = Clock::monotonicTime();
        }

        mBytes += bytes;
        static int count = 1;
        if (count++ % mDelta == 0) {
            double current = Clock::monotonicTime();
            double dt = current - mStartTime;
            double deltaBytes = mBytes * 1000000000.0;
            printf("Rate: %f frames/s total: %u\n", (deltaBytes / dt), mBytes);
        }
    }
    uint64_t mBytes;
    int mDelta;
    double mStartTime;
};

void RPiPlayer::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case NOTIFY_QUEUE_AUDIO_BUFFER: {
		sp<Bundle> bundle = message->metaData();
		sp<Buffer> buffer = bundle->getObject<Buffer>("Buffer");
		//		static ByteRate br(1000);
		//		br.addData(buffer->size()/4);
		//		buffer.clear();
		mAudioBuffers->push_back(buffer);
		onFillAndPlayAudioBuffers();
		break;
	}
	case NOTIFY_PLAY_AUDIO_BUFFER: {
		onPlayAudioBuffers();
		break;
	}
	case NOTIFY_QUEUE_VIDEO_BUFFER: {
		sp<Bundle> bundle = message->metaData();
		sp<Buffer> accessUnit = bundle->getObject<Buffer>("Access-Unit");
		mVideoBuffers->push_back(accessUnit);
		onPlayVideoBuffers();
		break;
	}
	case NOTIFY_PLAY_VIDEO_BUFFER: {
		onPlayVideoBuffers();
		break;
	}
	case STOP_MEDIA_SOURCE_DONE: {
		mNetLooper->getLooper()->quit();
		mNetLooper->join();
		mNetLooper = NULL;
		Looper::myLooper()->quit();
//		OMX_Deinit();
//		finalizeOMXAudio();
//		finalizeOMXVideo();
		break;
	}
	case NOTIFY_AUDIO_OMX_EMPTY_BUFFER_DONE: {
		OMX_BUFFERHEADERTYPE* omxBuffer = ilclient_get_input_buffer(mAudioRenderer, 100, 0);
		assert(omxBuffer != NULL);
		mOmxAudioEmptyBuffers->push_back(omxBuffer);
		onFillAndPlayAudioBuffers();
		break;
	}
	}
}

bool RPiPlayer::startMediaSource(const String& url) {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::START_MEDIA_SOURCE);
	sp<Bundle> bundle = message->metaData();
	bundle->putObject("Player", this);
	bundle->putString("Url", url);
	message->sendToTarget();
	return true;
}

void RPiPlayer::stopMediaSource() {
	if (!mShutdown) {
		mShutdown = true;
		sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::STOP_MEDIA_SOURCE);
		sp<Bundle> bundle = message->metaData();
		bundle->putObject("Reply", obtainMessage(STOP_MEDIA_SOURCE_DONE));
		message->sendToTarget();
	}
}


void RPiPlayer::onFillAndPlayAudioBuffers() {
	uint32_t curNumSamples = numOmxOwnedAudioSamples();
	if (0 == curNumSamples) {
		if (!mBufferingAudio) {
			printf("rebuffering\n");
			mBufferingAudio = true;
		}
	}

	if (!minNumAudioSamplesAvailable()) {
		return;
	}

	while (!mOmxAudioEmptyBuffers->empty()) {
		uint32_t helpBufferOffset = 0;
		List< sp<Buffer> >::iterator itr = mAudioBuffers->begin();
		mHelpBuffer->setRange(0, OMX_AUDIO_BUFFER_SIZE - mNumAudioStretchSamples * BYTES_PER_SAMPLE);
		while (itr != mAudioBuffers->end() && helpBufferOffset < mHelpBuffer->size()) {
			size_t size = RPiPlayer::min((*itr)->size(), mHelpBuffer->size() - helpBufferOffset );

			memcpy(mHelpBuffer->data() + helpBufferOffset, (*itr)->data(), size);
			helpBufferOffset += size;
			if (size == (*itr)->size()) {
				itr = mAudioBuffers->erase(itr);
			} else {
				(*itr)->setRange((*itr)->offset() + size , (*itr)->size() - size);
			}
		}

		static uint32_t stretchCount = 0;
		stretchCount += mNumAudioStretchSamples;

		//static uint32_t count = 0;
		//if ((count++ % 100) == 0) {
		//	printf("%u\n", stretchCount);
		//}

		OMX_BUFFERHEADERTYPE* omxBuffer = *mOmxAudioEmptyBuffers->begin();
		mOmxAudioEmptyBuffers->erase(mOmxAudioEmptyBuffers->begin());
		unsigned char* pBuffer = omxBuffer->pBuffer;

		uint32_t omxAudioBufferOffset = 0;
		if (mNumAudioStretchSamples > 0 && helpBufferOffset == OMX_AUDIO_BUFFER_SIZE) {
			// stretch
			size_t blockSize = helpBufferOffset / mNumAudioStretchSamples;
			for (size_t i=0; i<mNumAudioStretchSamples; i++) {
				// copy data
				memcpy(pBuffer, mHelpBuffer->data(), blockSize);
				mHelpBuffer->advance(blockSize-BYTES_PER_SAMPLE);
				pBuffer += blockSize;

				// copy stretch sample
				memcpy(pBuffer, mHelpBuffer->data(), BYTES_PER_SAMPLE);
				mHelpBuffer->advance(BYTES_PER_SAMPLE);
				pBuffer += BYTES_PER_SAMPLE;
			}
		} else {
			// dont stretch
			memcpy(pBuffer, mHelpBuffer->data(), helpBufferOffset);
			pBuffer += helpBufferOffset;
		}
		omxBuffer->nOffset = 0;
		omxBuffer->nFilledLen = pBuffer-omxBuffer->pBuffer;

		mOmxAudioInputBuffers->push_back(omxBuffer);
		onPlayAudioBuffers();

		if (!minNumAudioSamplesAvailable()) {
			break;
		}
	}
}

void RPiPlayer::onPlayAudioBuffers() {
	List<OMX_BUFFERHEADERTYPE*>::iterator itr = mOmxAudioInputBuffers->begin();
	while (itr != mOmxAudioInputBuffers->end()) {
		OMX_BUFFERHEADERTYPE* omxBuffer = *itr;
		itr = mOmxAudioInputBuffers->erase(itr);
		writeToFile(omxBuffer->pBuffer, omxBuffer->nFilledLen);
		OMX_ERRORTYPE result = OMX_EmptyThisBuffer(ILC_GET_HANDLE(mAudioRenderer), omxBuffer);
		assert(result == OMX_ErrorNone);
		calcNumAudioStretchSamples();
	}
}

void RPiPlayer::onPlayVideoBuffers() {
	OMX_BUFFERHEADERTYPE* omxBuffer;
	size_t omxBufferFillLevel;

	while (!mVideoBuffers->empty()) {
		omxBuffer = ilclient_get_input_buffer(mVideoDecoder, 130, 0);
		if (omxBuffer == NULL) {
			break;
		}
		sp<Buffer> accessUnit = *mVideoBuffers->begin();
		unsigned char* pBuffer = omxBuffer->pBuffer;

		size_t size = accessUnit->size() > omxBuffer->nAllocLen ? omxBuffer->nAllocLen : accessUnit->size();
		memcpy(pBuffer, accessUnit->data(), size);
		omxBufferFillLevel = size;

		if (size == accessUnit->size()) {
			mVideoBuffers->erase(mVideoBuffers->begin());
		} else {
			accessUnit->setRange(accessUnit->offset() + size, accessUnit->size() - size);
		}

		if (!mPortSettingsChanged  &&
			((omxBufferFillLevel > 0 && ilclient_remove_event(mVideoDecoder, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
			 (omxBufferFillLevel == 0 && ilclient_wait_for_event(mVideoDecoder, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
																 ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {
			if (ilclient_setup_tunnel(mTunnel, 0, 0) != 0) {
				return;
			}

			ilclient_change_component_state(mVideoScheduler, OMX_StateExecuting);

			if (ilclient_setup_tunnel(mTunnel + 1, 0, 1000) != 0) {
				return;
			}

			ilclient_change_component_state(mVideoRenderer, OMX_StateExecuting);

			mPortSettingsChanged = true;
		}

		if (omxBufferFillLevel == 0) {
			return;
		}

		omxBuffer->nOffset = 0;
		omxBuffer->nFilledLen = omxBufferFillLevel;
		omxBufferFillLevel = 0;

		if (mFirstVideoPacket) {
			omxBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
			mFirstVideoPacket = false;
		} else {
			omxBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
		}

		if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(mVideoDecoder), omxBuffer) != OMX_ErrorNone) {
			assert(false);
		}
	}
}

void RPiPlayer::onEmptyBufferDone(void* args, COMPONENT_T* component) {
	RPiPlayer* self = (RPiPlayer*) args;
	if (component == self->mAudioRenderer) {
		self->obtainMessage(NOTIFY_AUDIO_OMX_EMPTY_BUFFER_DONE)->sendToTarget();
	} else if (component == self->mVideoDecoder) {
		self->obtainMessage(NOTIFY_PLAY_VIDEO_BUFFER)->sendToTarget();
	} else {
		assert(false);
	}
}

uint32_t RPiPlayer::numOmxOwnedAudioSamples() {
	OMX_PARAM_U32TYPE param;
	OMX_ERRORTYPE error;

	memset(&param, 0, sizeof(OMX_PARAM_U32TYPE));
	param.nSize = sizeof(OMX_PARAM_U32TYPE);
	param.nVersion.nVersion = OMX_VERSION;
	param.nPortIndex = 100;
	error = OMX_GetConfig(ILC_GET_HANDLE(mAudioRenderer), OMX_IndexConfigAudioRenderingLatency, &param);
	assert(error == OMX_ErrorNone);

	return param.nU32;
}

uint32_t RPiPlayer::numAudioSamplesAvailable() {
	uint32_t totalBufferSize = 0;
	List< sp<Buffer> >::iterator itr = mAudioBuffers->begin();
	while (itr != mAudioBuffers->end()) {
		totalBufferSize += (*itr)->size();
		++itr;
	}
	return totalBufferSize;
}

bool RPiPlayer::minNumAudioSamplesAvailable() {
	uint32_t totalBufferSize = numAudioSamplesAvailable();
	if (mBufferingAudio) {
		if (totalBufferSize < NUM_OMX_AUDIO_BUFFERS  * OMX_AUDIO_BUFFER_SIZE * SOFTWARE_BUFFER_FACTOR) {
			return false;
		}
		printf("buffering done\n");
		mBufferingAudio = false;
	}
	return true;
}

void RPiPlayer::calcNumAudioStretchSamples() {
	uint32_t curNumSamples = numOmxOwnedAudioSamples();
	if (curNumSamples < 3500 && mNumAudioStretchSamples < 8) {
		mNumAudioStretchSamples += 4; // OMX_AUDIO_BUFFER_SIZE must be divisible by mNumAudioStretchSamples
	} else if (curNumSamples > 4000 && mNumAudioStretchSamples > 0) {
		mNumAudioStretchSamples -= 4; // OMX_AUDIO_BUFFER_SIZE must be divisible by mNumAudioStretchSamples
	}

	static int32_t lastTimeNumSamples = 0;
	static int32_t diffNumSamples = 0;
	static uint32_t count = 0;
	diffNumSamples += (curNumSamples - lastTimeNumSamples);
	lastTimeNumSamples = curNumSamples;
	if (count++ == 1000) {
		printf("OMX Buffer Size: %d Samples (%d ms) => Drift: %d Samples, Stretch Level: %d Samples\n",
			   curNumSamples,
			   curNumSamples / (SAMPLE_RATE / 1000),
			   diffNumSamples,
			   mNumAudioStretchSamples);
		diffNumSamples = 0;
		count = 0;
	}
}

int RPiPlayer::initOMXAudio() {
	memset(mAudioComponentList, 0, sizeof(mAudioComponentList));

	if ((mAudioClient = ilclient_init()) == NULL) {
		return -1;
	}

	ilclient_create_component(mAudioClient, &mAudioRenderer, const_cast<char *>("audio_render"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_ENABLE_INPUT_BUFFERS | ILCLIENT_DISABLE_ALL_PORTS));

	mAudioComponentList[0] = mAudioRenderer;

	OMX_PARAM_PORTDEFINITIONTYPE portDefinitions;
	memset(&portDefinitions, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	portDefinitions.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	portDefinitions.nVersion.nVersion = OMX_VERSION;
	portDefinitions.nPortIndex = 100;

	OMX_ERRORTYPE result;
	result = OMX_GetParameter(ILC_GET_HANDLE(mAudioRenderer), OMX_IndexParamPortDefinition, &portDefinitions);
	assert(result == OMX_ErrorNone);

	// Buffer sizes must be 16 byte aligned for VCHI
	portDefinitions.nBufferSize = (OMX_AUDIO_BUFFER_SIZE + 15) & ~15;
	portDefinitions.nBufferCountActual = NUM_OMX_AUDIO_BUFFERS;

	result = OMX_SetParameter(ILC_GET_HANDLE(mAudioRenderer), OMX_IndexParamPortDefinition, &portDefinitions);
	assert(result == OMX_ErrorNone);

	OMX_AUDIO_PARAM_PCMMODETYPE pcmParams;
	memset(&pcmParams, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	pcmParams.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
	pcmParams.nVersion.nVersion = OMX_VERSION;
	pcmParams.nPortIndex = 100;
	pcmParams.nChannels = NUM_CHANNELS;
	pcmParams.eNumData = OMX_NumericalDataSigned;
	pcmParams.eEndian = OMX_EndianLittle;
	pcmParams.nSamplingRate = SAMPLE_RATE;
	pcmParams.bInterleaved = OMX_TRUE;
	pcmParams.nBitPerSample = BITS_PER_SAMPLE;
	pcmParams.ePCMMode = OMX_AUDIO_PCMModeLinear;

	switch(NUM_CHANNELS) {
	case 1:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelCF;
		break;
	case 2:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
		pcmParams.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
		break;
	case 4:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
		pcmParams.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
		pcmParams.eChannelMapping[2] = OMX_AUDIO_ChannelLR;
		pcmParams.eChannelMapping[3] = OMX_AUDIO_ChannelRR;
		break;
	case 8:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
		pcmParams.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
		pcmParams.eChannelMapping[2] = OMX_AUDIO_ChannelCF;
		pcmParams.eChannelMapping[3] = OMX_AUDIO_ChannelLFE;
		pcmParams.eChannelMapping[4] = OMX_AUDIO_ChannelLR;
		pcmParams.eChannelMapping[5] = OMX_AUDIO_ChannelRR;
		pcmParams.eChannelMapping[6] = OMX_AUDIO_ChannelLS;
		pcmParams.eChannelMapping[7] = OMX_AUDIO_ChannelRS;
		break;
	}

	result = OMX_SetParameter(ILC_GET_HANDLE(mAudioRenderer), OMX_IndexParamAudioPcm, &pcmParams);
	assert(result == OMX_ErrorNone);

	ilclient_change_component_state(mAudioRenderer, OMX_StateIdle);
	if (ilclient_enable_port_buffers(mAudioRenderer, 100, NULL, NULL, NULL) < 0) {
		ilclient_change_component_state(mAudioRenderer, OMX_StateLoaded);
		ilclient_cleanup_components(mAudioComponentList);
		return -2;
	}

	ilclient_change_component_state(mAudioRenderer, OMX_StateExecuting);

	ilclient_set_empty_buffer_done_callback(mAudioClient, onEmptyBufferDone, this);

	OMX_BUFFERHEADERTYPE* omxBuffer;
	while ((omxBuffer = ilclient_get_input_buffer(mAudioRenderer, 100, 0)) != NULL) {
		mOmxAudioEmptyBuffers->push_back(omxBuffer);
	}
	assert(mOmxAudioEmptyBuffers->size() == NUM_OMX_AUDIO_BUFFERS);
	return 0;
}

bool RPiPlayer::setAudioSink(const char* sinkName) {
	OMX_CONFIG_BRCMAUDIODESTINATIONTYPE audioSink;
	if (sinkName && strlen(sinkName) < sizeof(audioSink.sName)) {
		memset(&audioSink, 0, sizeof(audioSink));
		audioSink.nSize = sizeof(OMX_CONFIG_BRCMAUDIODESTINATIONTYPE);
		audioSink.nVersion.nVersion = OMX_VERSION;
		strcpy((char*)audioSink.sName, sinkName);

		OMX_ERRORTYPE result;
		result = OMX_SetConfig(ILC_GET_HANDLE(mAudioRenderer), OMX_IndexConfigBrcmAudioDestination, &audioSink);
		return result == OMX_ErrorNone;
	}
	return false;
}

void RPiPlayer::finalizeOMXAudio() {
	ilclient_change_component_state(mAudioRenderer, OMX_StateIdle);

	OMX_ERRORTYPE result;
	result = OMX_SendCommand(ILC_GET_HANDLE(mAudioRenderer), OMX_CommandStateSet, OMX_StateLoaded, NULL);
	assert(result == OMX_ErrorNone);

	ilclient_disable_port_buffers(mAudioRenderer, 100, mAudioBuffer, NULL, NULL);
	ilclient_change_component_state(mAudioRenderer, OMX_StateLoaded);
	ilclient_cleanup_components(mAudioComponentList);

	ilclient_destroy(mAudioClient);
}

int RPiPlayer::initOMXVideo() {
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;

	memset(mVideoComponentList, 0, sizeof(mVideoComponentList));
	memset(mTunnel, 0, sizeof(mTunnel));

	if ((mVideoClient = ilclient_init()) == NULL) {
		return false;
	}

	// create video_decode
	if (ilclient_create_component(mVideoClient, &mVideoDecoder, const_cast<char *>("video_decode"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0) {
		return -3;
	}
	mVideoComponentList[0] = mVideoDecoder;

	// create video_render
	if (ilclient_create_component(mVideoClient, &mVideoRenderer, const_cast<char *>("video_render"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -4;
	}
	mVideoComponentList[1] = mVideoRenderer;

	// create clock
	if (ilclient_create_component(mVideoClient, &mClock, const_cast<char *>("clock"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -5;
	}
	mVideoComponentList[2] = mClock;

	memset(&cstate, 0, sizeof(cstate));
	cstate.nSize = sizeof(cstate);
	cstate.nVersion.nVersion = OMX_VERSION;
	cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
	cstate.nWaitMask = 1;
	if (OMX_SetParameter(ILC_GET_HANDLE(mClock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone) {
		return -6;
	}

	// create video_scheduler
	if (ilclient_create_component(mVideoClient, &mVideoScheduler, const_cast<char *>("video_scheduler"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -7;
	}
	mVideoComponentList[3] = mVideoScheduler;

	set_tunnel(mTunnel, mVideoDecoder, 131, mVideoScheduler, 10);
	set_tunnel(mTunnel + 1, mVideoScheduler, 11, mVideoRenderer, 90);
	set_tunnel(mTunnel + 2, mClock, 80, mVideoScheduler, 12);

	// setup clock tunnel first
	if (ilclient_setup_tunnel(mTunnel + 2, 0, 0) != 0) {
		return -8;
	} else {
		ilclient_change_component_state(mClock, OMX_StateExecuting);
	}

	ilclient_change_component_state(mVideoDecoder, OMX_StateIdle);

	memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	format.nVersion.nVersion = OMX_VERSION;
	format.nPortIndex = 130;
	format.eCompressionFormat = OMX_VIDEO_CodingAVC;

	if (OMX_SetParameter(ILC_GET_HANDLE(mVideoDecoder), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
			ilclient_enable_port_buffers(mVideoDecoder, 130, NULL, NULL, NULL) == 0) {
		ilclient_change_component_state(mVideoDecoder, OMX_StateExecuting);

		ilclient_set_empty_buffer_done_callback(mVideoClient, onEmptyBufferDone, this);
		return 0;
	} else {
		return -9;
	}
}

void RPiPlayer::finalizeOMXVideo() {
	ilclient_disable_tunnel(mTunnel);
	ilclient_disable_tunnel(mTunnel + 1);
	ilclient_disable_tunnel(mTunnel + 2);
	ilclient_teardown_tunnels(mTunnel);

	ilclient_state_transition(mVideoComponentList, OMX_StateIdle);
	ilclient_state_transition(mVideoComponentList, OMX_StateLoaded);

	ilclient_cleanup_components(mVideoComponentList);

	ilclient_destroy(mVideoClient);
}
