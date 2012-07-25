#include "RPiPlayer.h"
#include <errno.h>
#include "mindroid/os/Bundle.h"
#include "mindroid/util/Buffer.h"
#include <stdio.h>
#include <unistd.h>

using namespace mindroid;

static const uint32_t 	SAMPLE_RATE = 43940; //44100;
static const uint32_t 	NUMBER_CHANNELS = 2;
static const uint32_t 	NUMBER_BITS_PER_SAMPLE = 16;
static const uint32_t 	NUMBER_OMX_BUFFERS = 1;
static const uint32_t 	NUMBER_ACCESS_UNITS = 8;
static const uint32_t   NUM_AUDIO_FRAMES = 2205;

RPiPlayer::RPiPlayer() :
		mAudioClient(NULL),
		mAudioRenderer(NULL),
		mAudioBuffer(NULL),
		mFirstPacketAudio(true),
		mVideoClient(NULL),
		mVideoDecoder(NULL),
		mVideoScheduler(NULL),
		mVideoRenderer(NULL),
		mClock(NULL),
		mPortSettingsChanged(false),
		mFirstPacketVideo(true) {
	mAudioAccessUnits = new List< sp<Buffer> >();
	mVideoAccessUnits = new List< sp<Buffer> >();
	mFilledOmxInputBuffers = new List< OMX_BUFFERHEADERTYPE* >();
	mEmptyOmxInputBuffers = new List< OMX_BUFFERHEADERTYPE* >();
	mNetLooper =  new LooperThread<NetHandler>();
	mNetLooper->start();
	mNetLooper->setSchedulingParams(SCHED_OTHER, -17);
}

RPiPlayer::~RPiPlayer() {
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

void RPiPlayer::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case NOTIFY_QUEUE_AUDIO_BUFFER: {
		sp<Buffer> accessUnit = *((sp<Buffer>*) message->obj);
		delete (sp<Buffer>*) message->obj;
		mAudioAccessUnits->push_back(accessUnit);
		//static int count = 0;
		//if (count++ % 10 == 0) {
		//	printf("A: %d E: %d F: %d\n", mAudioAccessUnits.size(), mEmptyOmxInputBuffers.size(), mFilledOmxInputBuffers.size());
		//}
		if (mAudioAccessUnits->size() > NUMBER_ACCESS_UNITS) {
			obtainMessage(NOTIFY_FILL_INPUT_BUFFERS)->sendToTarget();
		}
		break;
	}
	case NOTIFY_PLAY_AUDIO_BUFFER: {
		onPlayAudioBuffer(*mAudioAccessUnits->begin());
		break;
	}
	case NOTIFY_FILL_INPUT_BUFFERS: {
		onFillInputBuffers();
		break;
	}
	case NOTIFY_QUEUE_VIDEO_BUFFER: {
		sp<Bundle> bundle = message->getData();
		sp<Buffer> accessUnit = bundle->getObject<Buffer>("Access-Unit");
		mVideoAccessUnits->push_back(accessUnit);
		obtainMessage(NOTIFY_PLAY_VIDEO_BUFFER)->sendToTarget();
		break;
	}
	case NOTIFY_PLAY_VIDEO_BUFFER: {
		if (!mVideoAccessUnits->empty()) {
			onPlayVideoBuffer(*mVideoAccessUnits->begin());
			mVideoAccessUnits->erase(mVideoAccessUnits->begin());
		}
		break;
	}
	case NOTIFY_INPUT_BUFFER_FILLED: {
		onInputBufferFilled();
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
	case NOTIFY_EMPTY_OMX_BUFFER: {
		OMX_BUFFERHEADERTYPE* omxMessage = static_cast<OMX_BUFFERHEADERTYPE*>(message->obj);
		mEmptyOmxInputBuffers->push_back(omxMessage);
		onFillInputBuffers();
		break;
	}
	}
}

bool RPiPlayer::startMediaSource(const String& url) {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::START_MEDIA_SOURCE);
	sp<Bundle> bundle = new Bundle();
	bundle->putObject("Player", this);
	bundle->putString("Url", url);
	message->setData(bundle);
	message->sendToTarget();
	return true;
}

void RPiPlayer::stopMediaSource() {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::STOP_MEDIA_SOURCE);
	sp<Bundle> bundle = new Bundle();
	bundle->putObject("Reply", obtainMessage(STOP_MEDIA_SOURCE_DONE));
	message->setData(bundle);
	message->sendToTarget();
}

void RPiPlayer::onFillInputBuffers() {
	while (!mAudioAccessUnits->empty() && !mEmptyOmxInputBuffers->empty()) {
		OMX_BUFFERHEADERTYPE* omxBuffer = *mEmptyOmxInputBuffers->begin();
		mEmptyOmxInputBuffers->erase(mEmptyOmxInputBuffers->begin());
		sp<Buffer> accessUnit = *mAudioAccessUnits->begin();
		mAudioAccessUnits->erase(mAudioAccessUnits->begin());

		unsigned char* pBuffer = omxBuffer->pBuffer;
		size_t size = accessUnit->size();
		memcpy(pBuffer, accessUnit->data(), size);
		omxBuffer->nOffset = 0;
		omxBuffer->nFilledLen = size;

		mFilledOmxInputBuffers->push_back(omxBuffer);
		obtainMessage(NOTIFY_INPUT_BUFFER_FILLED)->sendToTarget();
	}
}

void RPiPlayer::onPlayAudioBuffer(const sp<Buffer>& accessUnit) {
	if (mFilledOmxInputBuffers->size() > 0) {
		OMX_ERRORTYPE result;
		List< OMX_BUFFERHEADERTYPE* >::iterator itr = mFilledOmxInputBuffers->begin();
		while (itr != mFilledOmxInputBuffers->end()) {
			OMX_BUFFERHEADERTYPE* omxBuffer = *itr;
			itr = mFilledOmxInputBuffers->erase(itr);
		    result = OMX_EmptyThisBuffer(ILC_GET_HANDLE(mAudioRenderer), omxBuffer);
			if (result != OMX_ErrorNone) {
				printf("OMX_EmptyThisBuffer failed: %d\n", result);
				return;
			}
		}
	}
}

void RPiPlayer::onInputBufferFilled() {
	obtainMessage(NOTIFY_PLAY_AUDIO_BUFFER)->sendToTarget();
}

void RPiPlayer::onPlayVideoBuffer(const sp<Buffer>& accessUnit) {
	printf("RPiPlayer::onPlayVideoBuffer\n");
	static int i = 0;
	i++;
	if (i == 100) {
		stop();
	}
	OMX_BUFFERHEADERTYPE* omxBuffer;
	size_t omxBufferFillLevel;
	size_t offset = 0;
	int blockingMode = 0;

	while (offset < accessUnit->size()) {
		if ((omxBuffer = ilclient_get_input_buffer(mVideoDecoder, 130, blockingMode)) != NULL) {
			unsigned char* pBuffer = omxBuffer->pBuffer;

			size_t size = ((accessUnit->size() - offset) > omxBuffer->nAllocLen) ? omxBuffer->nAllocLen : (accessUnit->size() - offset);
			memcpy(pBuffer, accessUnit->data() + offset, size);
			omxBufferFillLevel = size;
			offset += size;

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

			if (mFirstPacketVideo) {
				omxBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
				mFirstPacketVideo = false;
			} else {
				omxBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
			}

			if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(mVideoDecoder), omxBuffer) != OMX_ErrorNone) {
				return;
			}

			blockingMode = 1;
		} else {
			break;
		}
	}
}

void RPiPlayer::onEmptyBufferDone(void* args, COMPONENT_T* component) {
	RPiPlayer* self = (RPiPlayer*) args;
	if (component == self->mAudioRenderer) {
		OMX_BUFFERHEADERTYPE* omxBuffer = ilclient_get_input_buffer(self->mAudioRenderer, 100, 1);
		assert(omxBuffer);
		sp<Message> m = self->obtainMessage(NOTIFY_EMPTY_OMX_BUFFER);
		m->obj = omxBuffer;
		m->sendToTarget();
	} else if (component == self->mVideoDecoder) {
		self->obtainMessage(NOTIFY_PLAY_VIDEO_BUFFER)->sendToTarget();
	} else {
		assert(false);
	}
}

int RPiPlayer::initOMXAudio() {
	const uint32_t sampleRate = SAMPLE_RATE;
	const uint32_t numChannels = NUMBER_CHANNELS;
	const uint32_t bitsPerSample = NUMBER_BITS_PER_SAMPLE;
	const uint32_t numOmxBuffers = NUMBER_OMX_BUFFERS;
	const uint32_t bufferSize = (NUM_AUDIO_FRAMES * numChannels * bitsPerSample) >> 3;

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

    // Buffers must be 16 byte aligned for VCHI
    portDefinitions.nBufferSize = (bufferSize + 15) & ~15;
    portDefinitions.nBufferCountActual = numOmxBuffers;

    result = OMX_SetParameter(ILC_GET_HANDLE(mAudioRenderer), OMX_IndexParamPortDefinition, &portDefinitions);
    assert(result == OMX_ErrorNone);

    OMX_AUDIO_PARAM_PCMMODETYPE pcmParams;
    memset(&pcmParams, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    pcmParams.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    pcmParams.nVersion.nVersion = OMX_VERSION;
    pcmParams.nPortIndex = 100;
    pcmParams.nChannels = numChannels;
    pcmParams.eNumData = OMX_NumericalDataSigned;
    pcmParams.eEndian = OMX_EndianLittle;
    pcmParams.nSamplingRate = sampleRate;
    pcmParams.bInterleaved = OMX_TRUE;
    pcmParams.nBitPerSample = bitsPerSample;
    pcmParams.ePCMMode = OMX_AUDIO_PCMModeLinear;

    switch(numChannels) {
	case 1:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelCF;
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
	case 4:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
		pcmParams.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
		pcmParams.eChannelMapping[2] = OMX_AUDIO_ChannelLR;
		pcmParams.eChannelMapping[3] = OMX_AUDIO_ChannelRR;
		break;
	case 2:
		pcmParams.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
		pcmParams.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
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
		mEmptyOmxInputBuffers->push_back(omxBuffer);
	}
	assert(mEmptyOmxInputBuffers->size() == numOmxBuffers);
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
