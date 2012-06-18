#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include "RtspSocket.h"
#include "android/lang/String.h"
#include "android/net/DatagramSocket.h"
extern "C" {
#include "bcm_host.h"
#include "ilclient.h"
}

using namespace android::os;
using namespace android::lang;
using namespace android::net;

static TUNNEL_T tunnel[4];
static COMPONENT_T* list[5];
static ILCLIENT_T* client;
static COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *omx_clock = NULL;

static int initOMX() {
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	unsigned char* data = NULL;

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
	if (ilclient_create_component(client, &video_decode, "video_decode", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0) {
		return -3;
	}
	list[0] = video_decode;

	// create video_render
	if (ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		return -4;
	}
	list[1] = video_render;

	// create clock
	if (ilclient_create_component(client, &omx_clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
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
	if (ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
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

static void finalizeOMX() {
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

int main(int argc, char**argv)
{
	char* strIpAddress = NULL;
	char* strPort = "9000";

	if (argc < 2) {
		printf("Usage: <IP-Address> [<Port>]\n");
		return -1;
	}
	strIpAddress = argv[1];
	if (argc == 3) {
		strPort = argv[2];
	}
	uint16_t port = atoi(strPort);
	
	setpriority(PRIO_PROCESS, 0, -19);

	if (initOMX() != 0) {
		printf("OMX init failed!\n");
		return -1;
	}

	sp<RtspSocket> mSocket = new RtspSocket();
	if (!mSocket->connect(strIpAddress, port)) {
		printf("Cannot connect to server %s:%d\n", strIpAddress, port);
		return -1;
	}

	uint8_t rtspResponse[4096];

	String optionsMessage = String::format("OPTIONS rtsp://%s:%d/Test.sdp RTSP/1.0\r\nCSeq: 1\r\n\r\n", strIpAddress, strPort);
	mSocket->write(optionsMessage.c_str(), optionsMessage.size());
	int32_t size = mSocket->readPacket(rtspResponse, 4096);
	rtspResponse[size] = '\0';
	printf("OPTIONS: %s\n", rtspResponse);

	String describeMessage = String::format("DESCRIBE rtsp://%s:%d/Test.sdp RTSP/1.0\r\nCSeq: 2\r\n\r\n", strIpAddress, strPort);
	mSocket->write(describeMessage.c_str(), describeMessage.size());
	size = mSocket->readPacket(rtspResponse, 4096);
	rtspResponse[size] = '\0';
	printf("DESCRIBE: %s\n", rtspResponse);

	sp<DatagramSocket> rtpSocket = new DatagramSocket(56098);
	sp<DatagramSocket> rtcpSocket = new DatagramSocket(56099);

	String setupMessage = String::format("SETUP rtsp://%s:%d/Test.sdp RTSP/1.0\r\nCSeq: 3\r\nTransport: RTP/AVP;unicast;client_port=56098-56099\r\n\r\n", strIpAddress, strPort);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
	size = mSocket->readPacket(rtspResponse, 4096);
	rtspResponse[size] = '\0';
	printf("SETUP: %s\n", rtspResponse);
	char* session = "Session: ";
	session = strstr((char*) rtspResponse, session);
	session += strlen("Session: ");
	int i = 0;
	while (*(session + i) != '\r') {
		i++;	
	}
	String sessionId(session, i);

	String playMessage = String::format("PLAY rtsp://%s:%d/Test.sdp RTSP/1.0\r\nCSeq: 4\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n", strIpAddress, strPort, sessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
	size = mSocket->readPacket(rtspResponse, 4096);
	rtspResponse[size] = '\0';
	printf("PLAY: %s\n", rtspResponse);


	uint8_t data[4096];
	printf("Waiting for H.264 data...\n");
	size_t totalFUCount = 0;
	uint8_t nalHeader;
	bool startUnit = true;
	int port_settings_changed = 0;

	struct timespec now;
	uint32_t t1, t2, dT;
	uint32_t time1, time2;

	unsigned int data_len = 0;

	while (true) {
		int size = rtpSocket->recv(data, 4096);
		if (size <= 0) {
			break;
		} else if (size < 12) {
			continue;
		}
		if ((data[0] >> 6) != 2) {
			// Only RTPv2 is supported.
			return -1;
		}
		if (data[0] & 0x20) {
			// Padding present.
			size_t paddingSize = data[size - 1];
			if (paddingSize + 12 > size) {
				// If we removed this much padding we'd end up with something
				// that's too short to be a valid RTP header.
				continue;
			}

			size -= paddingSize;
		}
		int numCSRCs = data[0] & 0x0F;
		size_t payloadOffset = 12 + 4 * numCSRCs;
		if (size < payloadOffset) {
			// Not enough data to fit the basic header and all the CSRC entries.
			continue;
		}
		if (data[0] & 0x10) {
			// Header eXtension present.
			if (size < payloadOffset + 4) {
				// Not enough data to fit the basic header, all CSRC entries
				// and the first 4 bytes of the extension header.
				continue;
			}

			const uint8_t* extensionData = &data[payloadOffset];
			size_t extensionLength = 4 * (extensionData[2] << 8 | extensionData[3]);
			if (size < payloadOffset + 4 + extensionLength) {
				continue;
			}
			payloadOffset += 4 + extensionLength;
		}

		//buffer->setRange(payloadOffset, size - payloadOffset);

		bool nalUnitBegins = false;
		bool singleNalUnit = false;
		uint8_t* h264Data = NULL;
		unsigned nalType = data[payloadOffset] & 0x1F;
		bool printDT = false;
		int status = 0;

		if (nalType >= 1 && nalType <= 23) {
			h264Data = &data[payloadOffset];
			singleNalUnit = true;
			nalUnitBegins = true;
//			printf("Single NAL unit with %d bytes of data\n", size - payloadOffset);
			clock_gettime(CLOCK_REALTIME, &now);
			t1 = now.tv_sec * 1000LL + now.tv_nsec / 1000000;
			printDT = true;
			uint32_t nri = (data[payloadOffset] >> 5) & 3;
			printf("NAL type: %d %d\n", nalType, nri);
//			if (nalType != 7 && nalType != 8) {
//				continue;
//			}
		} else if (nalType == 28) {
			// FU-A
			h264Data = &data[payloadOffset + 2];
//			printf("Fragmented NAL unit with %d bytes of data\n", size - payloadOffset);
			totalFUCount++;
			if (startUnit || (data[payloadOffset + 1] & 0x80)) {
//				printf("New NAL unit begins...\n");
				uint32_t nalType = data[payloadOffset + 1] & 0x1f;
				uint32_t nri = (data[payloadOffset + 0] >> 5) & 3;
				nalHeader = (nri << 5) | nalType;
				nalUnitBegins = true;
				startUnit = false;
//				if (nalType == 7 || nalType == 8) {
					printf("FU NAL type: %d %d\n", nalType, nri);
//				}
			}
			if (data[payloadOffset + 1] & 0x40) {
//				printf("Fragmented NAL unit complete with %d fragments\n", totalFUCount);
				totalFUCount = 0;
				clock_gettime(CLOCK_REALTIME, &now);
				t1 = now.tv_sec * 1000LL + now.tv_nsec / 1000000;
				printDT = true;

//				time1 = now.tv_sec * 1000LL + now.tv_nsec / 1000000;
//				uint32_t deltaT = time1 - time2;
//				printf("dT: %dms\n", deltaT);
//				fflush(stdout);
//				time2 = time1;
			}
		} else if (nalType == 24) {
			printf("Oops\n");
			continue;
		} else {
			printf("Unknown NAL Type: %d\n", nalType);
			continue;
		}
		if (printDT) {
			dT = t1 - t2;
			t2 = t1;
//			printf("dT: %dms\n", dT);
//			fflush(stdout);
		}

		OMX_BUFFERHEADERTYPE *buf;
		int first_packet = 1;

		clock_gettime(CLOCK_REALTIME, &now);
		uint32_t startTime = now.tv_sec * 1000LL + now.tv_nsec / 1000000;
		if ((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
		{
			clock_gettime(CLOCK_REALTIME, &now);
			uint32_t stopTime = now.tv_sec * 1000LL + now.tv_nsec / 1000000;
			uint32_t diffTime = stopTime - startTime;
			if (diffTime > 2) {
				printf("ilclient_get_input_buffer: %dms\n", diffTime);
			}
			// feed data and wait until we get port settings changed
			//printf("Buffer: %x\n", buf);
			unsigned char *dest = buf->pBuffer;

			uint32_t offset = 0;
			if (nalUnitBegins) {
				memcpy(dest, "\x00\x00\x00\x01", 4);
				offset += 4;
				if (!singleNalUnit) {
					memcpy(dest + offset, &nalHeader, 1);
					offset += 1;
//					printf("FU NAL type: %d\n", nalHeader & 0x1f);
//					printf("Added NAL unit header: 0x%02X\n", nalHeader);
				}
			}
			uint32_t dataSize;
			if (singleNalUnit) {
				dataSize = size - payloadOffset;
			} else {
				dataSize = size - payloadOffset - 2;
			}
			memcpy(dest + offset, h264Data, dataSize);
			data_len += offset + dataSize;

			if(port_settings_changed == 0  &&
			((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
			 (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
													   ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
			{
				port_settings_changed = 1;

				if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
				{
				   status = -7;
				   break;
				}

				ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

				// now setup tunnel to video_render
				if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
				{
				   status = -12;
				   break;
				}

				ilclient_change_component_state(video_render, OMX_StateExecuting);
			}
			if(!data_len)
				break;

			buf->nFilledLen = data_len;
			data_len = 0;

			buf->nOffset = 0;
			if(first_packet)
			{
				buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
				first_packet = 0;
			}
			else
				buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

			if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
			{
				status = -6;
				break;
			}
		}

//		printf("Done\n");
//
//		buf->nFilledLen = 0;
//		buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;
//
//		if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
//		status = -20;
//
//		// wait for EOS from render
//		ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,
//						  ILCLIENT_BUFFER_FLAG_EOS, 10000);
//
//		// need to flush the renderer to allow video_decode to disable its input port
//		ilclient_flush_tunnels(tunnel, 0);
//
//		ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
//
//		printf("%d\n", size);
	}

	finalizeOMX();

	return 0;
}
