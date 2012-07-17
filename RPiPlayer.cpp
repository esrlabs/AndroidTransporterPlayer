#include "RPiPlayer.h"
#include <errno.h>
#include "Bundle.h"
#include "Buffer.h"
#include <stdio.h>
#include <unistd.h>

using namespace android::os;

RPiPlayer::RPiPlayer() :
		video_decode(NULL),
		video_scheduler(NULL),
		video_render(NULL),
		omx_clock(NULL),
		omxBuffer(NULL),
		omxBufferFillSize(0),
		portSettingsChanged(false),
		firstPacket(true) {
	mNetLooper =  new LooperThread<NetHandler>();
	mNetLooper->start();
	mNetLooper->setSchedulingParams(SCHED_OTHER, -17);
}

RPiPlayer::~RPiPlayer() {
	mNetLooper->getLooper()->quit();
	mNetLooper->join();
	finalizeOMX();
}

void RPiPlayer::start(android::lang::String url) {
	setupMediaSource(url);
	initOMX();
}

void RPiPlayer::stop() {
}

void RPiPlayer::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case SET_RTSP_MEDIA_SOURCE:
		mRtspMediaSource = (RtspMediaSource*) message->obj;
		break;
	case MEDIA_SOURCE_NOTIFY:
		sp<Buffer> accessUnit = *((sp<Buffer>*) message->obj);
		delete (sp<Buffer>*) message->obj;
		onMediaSourceNotify(accessUnit);
		break;
	}
}

bool RPiPlayer::setupMediaSource(const android::lang::String& url) {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::SETUP_MEDIA_SOURCE);
	message->obj = new Bundle(this, url, obtainMessage(SET_RTSP_MEDIA_SOURCE));
	message->sendToTarget();
	return true;
}

void RPiPlayer::onMediaSourceNotify(const sp<Buffer>& accessUnit) {
	size_t offset = 0;

	while (offset < accessUnit->size()) {
		if ((omxBuffer = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL) {
			unsigned char* pBuffer = omxBuffer->pBuffer;

			size_t size = ((accessUnit->size() - offset) > omxBuffer->nAllocLen) ? omxBuffer->nAllocLen : (accessUnit->size() - offset);
			memcpy(pBuffer, accessUnit->data() + offset, size);
			omxBufferFillSize = size;
			offset += size;

			if (!portSettingsChanged  &&
					((omxBufferFillSize > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
					 (omxBufferFillSize == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
							 ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {
				portSettingsChanged = true;

				if(ilclient_setup_tunnel(tunnel, 0, 0) != 0) {
					printf("Oops\n");
					return;
				}

				ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

				// now setup tunnel to video_render
				if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0) {
					printf("Oops\n");
					return;
				}

				ilclient_change_component_state(video_render, OMX_StateExecuting);
			}

			if (omxBufferFillSize == 0) {
				printf("Oops\n");
				return;
			}

			omxBuffer->nOffset = 0;
			omxBuffer->nFilledLen = omxBufferFillSize;
			omxBufferFillSize = 0;

			if (firstPacket) {
				omxBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
				firstPacket = false;
			} else {
				omxBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
			}

			if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), omxBuffer) != OMX_ErrorNone) {
				printf("Oops\n");
				return;
			}
		}
	}
}

int RPiPlayer::initOMX() {
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	unsigned char* rtpPacketData = NULL;

	printf("Initializing OMX...\n");

	bcm_host_init();

	memset(list, 0, sizeof(list));
	memset(tunnel, 0, sizeof(tunnel));

	if ((client = ilclient_init()) == NULL) {
		return -1;
	}

	if (OMX_Init() != OMX_ErrorNone) {
		ilclient_destroy(client);
		return -2;
	}

	// create video_decode
	if (ilclient_create_component(client, &video_decode, const_cast<char *>("video_decode"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0) {
		return -3;
	}
	list[0] = video_decode;

	// create video_render
	if (ilclient_create_component(client, &video_render, const_cast<char *>("video_render"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -4;
	}
	list[1] = video_render;

	// create clock
	if (ilclient_create_component(client, &omx_clock, const_cast<char *>("clock"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -5;
	}
	list[2] = omx_clock;

	memset(&cstate, 0, sizeof(cstate));
	cstate.nSize = sizeof(cstate);
	cstate.nVersion.nVersion = OMX_VERSION;
	cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
	cstate.nWaitMask = 1;
	if (OMX_SetParameter(ILC_GET_HANDLE(omx_clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone) {
		return -6;
	}

	// create video_scheduler
	if (ilclient_create_component(client, &video_scheduler, const_cast<char *>("video_scheduler"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -7;
	}
	list[3] = video_scheduler;

	set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
	set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
	set_tunnel(tunnel+2, omx_clock, 80, video_scheduler, 12);

	// setup clock tunnel first
	if (ilclient_setup_tunnel(tunnel+2, 0, 0) != 0) {
		return -8;
	} else {
		ilclient_change_component_state(omx_clock, OMX_StateExecuting);
	}

	ilclient_change_component_state(video_decode, OMX_StateIdle);

	memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	format.nVersion.nVersion = OMX_VERSION;
	format.nPortIndex = 130;
	format.eCompressionFormat = OMX_VIDEO_CodingAVC;

	if (OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
			ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0) {
	  ilclient_change_component_state(video_decode, OMX_StateExecuting);
	  printf("OMX init done.\n");
	  return 0;
	} else {
		printf("OMX init failed!\n");
		return -9;
	}
}

void RPiPlayer::finalizeOMX() {
	ilclient_disable_tunnel(tunnel);
	ilclient_disable_tunnel(tunnel + 1);
	ilclient_disable_tunnel(tunnel + 2);
	ilclient_teardown_tunnels(tunnel);

	ilclient_state_transition(list, OMX_StateIdle);
	ilclient_state_transition(list, OMX_StateLoaded);

	ilclient_cleanup_components(list);

	OMX_Deinit();

	ilclient_destroy(client);
}
