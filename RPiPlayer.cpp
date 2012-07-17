#include "RPiPlayer.h"
#include <errno.h>
#include "Bundle.h"
#include "Buffer.h"
#include <stdio.h>
#include <unistd.h>

using namespace android::os;
using namespace android::lang;

RPiPlayer::RPiPlayer() :
		mVideoDecoder(NULL),
		mVideoScheduler(NULL),
		mVideoRenderer(NULL),
		mClock(NULL),
		mBuffer(NULL),
		mBufferFillSize(0),
		mPortSettingsChanged(false),
		mFirstPacket(true) {
	mNetLooper =  new LooperThread<NetHandler>();
	mNetLooper->start();
	mNetLooper->setSchedulingParams(SCHED_OTHER, -17);
}

RPiPlayer::~RPiPlayer() {
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
	case MEDIA_SOURCE_NOTIFY: {
		sp<Buffer> accessUnit = *((sp<Buffer>*) message->obj);
		delete (sp<Buffer>*) message->obj;
		onMediaSourceNotify(accessUnit);
		break;
	}
	case STOP_MEDIA_SOURCE_DONE: {
		mNetLooper->getLooper()->quit();
		printf("MEDIA_SOURCE_DONE\n");
		mNetLooper->join();
		printf("MEDIA_SOURCE_DONE done\n");
		mNetLooper = NULL;
		Looper::myLooper()->quit();
		break;
	}
	}
}

bool RPiPlayer::setupMediaSource(const android::lang::String& url) {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::SETUP_MEDIA_SOURCE);
	message->obj = new Bundle(this, url, NULL);
	message->sendToTarget();
	return true;
}

void RPiPlayer::stopMediaSource() {
	sp<Message> message = mNetLooper->getHandler()->obtainMessage(NetHandler::STOP_MEDIA_SOURCE);
	message->obj = new Bundle(NULL, String(NULL), obtainMessage(STOP_MEDIA_SOURCE_DONE));
	message->sendToTarget();
}

void RPiPlayer::onMediaSourceNotify(const sp<Buffer>& accessUnit) {
	size_t offset = 0;

	while (offset < accessUnit->size()) {
		if ((mBuffer = ilclient_get_input_buffer(mVideoDecoder, 130, 1)) != NULL) {
			unsigned char* pBuffer = mBuffer->pBuffer;

			size_t size = ((accessUnit->size() - offset) > mBuffer->nAllocLen) ? mBuffer->nAllocLen : (accessUnit->size() - offset);
			memcpy(pBuffer, accessUnit->data() + offset, size);
			mBufferFillSize = size;
			offset += size;

			if (!mPortSettingsChanged  &&
					((mBufferFillSize > 0 && ilclient_remove_event(mVideoDecoder, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
					 (mBufferFillSize == 0 && ilclient_wait_for_event(mVideoDecoder, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
							 ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {
				mPortSettingsChanged = true;

				if(ilclient_setup_tunnel(mTunnel, 0, 0) != 0) {
					return;
				}

				ilclient_change_component_state(mVideoScheduler, OMX_StateExecuting);

				// now setup tunnel to video_render
				if(ilclient_setup_tunnel(mTunnel + 1, 0, 1000) != 0) {
					return;
				}

				ilclient_change_component_state(mVideoRenderer, OMX_StateExecuting);
			}

			if (mBufferFillSize == 0) {
				return;
			}

			mBuffer->nOffset = 0;
			mBuffer->nFilledLen = mBufferFillSize;
			mBufferFillSize = 0;

			if (mFirstPacket) {
				mBuffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
				mFirstPacket = false;
			} else {
				mBuffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
			}

			if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(mVideoDecoder), mBuffer) != OMX_ErrorNone) {
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

	memset(mComponentList, 0, sizeof(mComponentList));
	memset(mTunnel, 0, sizeof(mTunnel));

	if ((mClient = ilclient_init()) == NULL) {
		return -1;
	}

	if (OMX_Init() != OMX_ErrorNone) {
		ilclient_destroy(mClient);
		return -2;
	}

	// create video_decode
	if (ilclient_create_component(mClient, &mVideoDecoder, const_cast<char *>("video_decode"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0) {
		return -3;
	}
	mComponentList[0] = mVideoDecoder;

	// create video_render
	if (ilclient_create_component(mClient, &mVideoRenderer, const_cast<char *>("video_render"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -4;
	}
	mComponentList[1] = mVideoRenderer;

	// create clock
	if (ilclient_create_component(mClient, &mClock, const_cast<char *>("clock"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -5;
	}
	mComponentList[2] = mClock;

	memset(&cstate, 0, sizeof(cstate));
	cstate.nSize = sizeof(cstate);
	cstate.nVersion.nVersion = OMX_VERSION;
	cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
	cstate.nWaitMask = 1;
	if (OMX_SetParameter(ILC_GET_HANDLE(mClock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone) {
		return -6;
	}

	// create video_scheduler
	if (ilclient_create_component(mClient, &mVideoScheduler, const_cast<char *>("video_scheduler"), ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -7;
	}
	mComponentList[3] = mVideoScheduler;

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
	  printf("OMX init done.\n");
	  return 0;
	} else {
		printf("OMX init failed!\n");
		return -9;
	}
}

void RPiPlayer::finalizeOMX() {
	ilclient_disable_tunnel(mTunnel);
	ilclient_disable_tunnel(mTunnel + 1);
	ilclient_disable_tunnel(mTunnel + 2);
	ilclient_teardown_tunnels(mTunnel);

	ilclient_state_transition(mComponentList, OMX_StateIdle);
	ilclient_state_transition(mComponentList, OMX_StateLoaded);

	ilclient_cleanup_components(mComponentList);

	OMX_Deinit();

	ilclient_destroy(mClient);
}
