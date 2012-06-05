#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Socket.h"
#include "DatagramSocket.h"
extern "C"
{
#include "bcm_host.h"
#include "ilclient.h"
}

using namespace android::net;

ILCLIENT_T *client;
TUNNEL_T tunnel[4];
static COMPONENT_T *list[5];
COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
int packet_size = 16<<10; // 16KB
unsigned int data_len = 0;
int status = 0;

static int initOMX() {
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;
	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	unsigned char *data = NULL;
	int find_start_codes = 0;

	bcm_host_init();

	memset(list, 0, sizeof(list));
	memset(tunnel, 0, sizeof(tunnel));

	printf("1\n");

	if((client = ilclient_init()) == NULL)
	{
		return -3;
	}

	printf("2\n");

	if(OMX_Init() != OMX_ErrorNone)
	{
		ilclient_destroy(client);
		return -4;
	}

	printf("3\n");

	// create video_decode
	if(ilclient_create_component(client, &video_decode, "video_decode", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0) {
		status = -14;
		return status;
	}
	list[0] = video_decode;

	printf("4\n");

	// create video_render
	if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		status = -14;
		return status;
	}
	list[1] = video_render;

	printf("5\n");

	// create clock
	if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		status = -14;
		return status;
	}
	list[2] = clock;

	printf("6\n");

	memset(&cstate, 0, sizeof(cstate));
	cstate.nSize = sizeof(cstate);
	cstate.nVersion.nVersion = OMX_VERSION;
	cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
	cstate.nWaitMask = 1;
	if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone) {
		status = -13;
	}

	printf("7\n");

	// create video_scheduler
	if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
		status = -14;
	}
	list[3] = video_scheduler;

	printf("8\n");

	set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
	set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
	set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

	// setup clock tunnel first
	if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0) {
		status = -15;
	} else {
		ilclient_change_component_state(clock, OMX_StateExecuting);
	}

	printf("9\n");

	if(status == 0) {
		ilclient_change_component_state(video_decode, OMX_StateIdle);
	}

	memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	format.nVersion.nVersion = OMX_VERSION;
	format.nPortIndex = 130;
	format.eCompressionFormat = OMX_VIDEO_CodingAVC;

	printf("10\n");

	if(status == 0 &&
		OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
		ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
	{
	  OMX_BUFFERHEADERTYPE *buf;
	  int port_settings_changed = 0;
	  int first_packet = 1;

	  ilclient_change_component_state(video_decode, OMX_StateExecuting);
	  printf("11\n");
	  return 0;
	}
	else
	{
		printf("12\n");
		return -17;
	}
}

static int finalizeOMX() {
	ilclient_disable_tunnel(tunnel);
	ilclient_disable_tunnel(tunnel+1);
	ilclient_disable_tunnel(tunnel+2);
	ilclient_teardown_tunnels(tunnel);

	ilclient_state_transition(list, OMX_StateIdle);
	ilclient_state_transition(list, OMX_StateLoaded);

	ilclient_cleanup_components(list);

	OMX_Deinit();

	ilclient_destroy(client);
}

static int32_t readFully(Socket* socket, uint8_t* data, const uint32_t size) {
	uint32_t dataSize = 0;
	int32_t result = 0;
	while (dataSize < size) {
		result = ::recv(socket->mSocketId, reinterpret_cast<char*>(data + dataSize), size - dataSize, 0);
		if (result > 0) {
			dataSize += result;
			if (data[dataSize - 4] == '\r' && data[dataSize - 3] == '\n' && data[dataSize - 2] == '\r' && data[dataSize - 1] == '\n') {
				break;
			}
		} else {
			break;
		}
	}
	return (result > 0) ? dataSize : -1;
}

int main(int argc, char**argv)
{
	uint8_t buffer[4096];
	
	if (initOMX() != 0) {
		printf("Oops: OMX error\n");
		return -1;
	}

	Socket* mSocket = new Socket();
	if (!mSocket->connect("192.168.1.146", 1234)) {
		printf("Error\n");
		return -1;
	}

	char* optionsMessage = "OPTIONS rtsp://192.168.1.146:1234/Test.sdp RTSP/1.0\r\nCSeq: 1\r\n\r\n";
	mSocket->write(optionsMessage, strlen(optionsMessage));
	memset(buffer, 0, 4096);
	int32_t size = readFully(mSocket, buffer, 4096);
	printf("OPTIONS: %s\n", buffer);

	char* describeMessage = "DESCRIBE rtsp://192.168.1.146:1234/Test.sdp RTSP/1.0\r\nCSeq: 2\r\n\r\n";
	mSocket->write(describeMessage, strlen(describeMessage));	
	memset(buffer, 0, 4096);
	size = readFully(mSocket, buffer, 4096);
	printf("DESCRIBE: %s\n", buffer);

	DatagramSocket* rtpSocket = new DatagramSocket(56098);
	DatagramSocket* rtcpSocket = new DatagramSocket(56099);

	char* setupMessage = "SETUP rtsp://192.168.1.146:1234/Test.sdp/trackID=0 RTSP/1.0\r\nCSeq: 3\r\nTransport: RTP/AVP;unicast;client_port=56098-56099\r\n\r\n";
	mSocket->write(setupMessage, strlen(setupMessage));	
	memset(buffer, 0, 4096);
	size = readFully(mSocket, buffer, 4096);
	printf("SETUP: %s\n", buffer);
	char* session = "Session: ";
	session = strstr((char*) buffer, session);
	session += strlen("Session: ");
	int i = 0;
	while (*(session + i) != '\r') {
		i++;	
	}
	char* sessionId = new char[i + 1];
	memcpy(sessionId, session, i);
	sessionId[i] = '\0';

	char* playMessage = new char[1024];
	strcpy(playMessage, "PLAY rtsp://192.168.1.146:1234/Test.sdp RTSP/1.0\r\nCSeq: 4\r\nRange: npt=0.000-\r\nSession: ");
	strcat(playMessage, sessionId);
	strcat(playMessage, "\r\n\r\n");
	mSocket->write(playMessage, strlen(playMessage));	
	memset(buffer, 0, 4096);
	size = readFully(mSocket, buffer, 4096);
	printf("PLAY: %s\n", buffer);

	uint8_t data[4096];
	printf("Waiting for data...\n");
	size_t totalFUCount = 0;
	uint8_t nalHeader;
	bool startUnit = true;
	int port_settings_changed = 0;
	FILE *in;
	in = fopen("test.h264", "rb");
	printf("file: %x\n", in);

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
		if (nalType >= 1 && nalType <= 23) {
			h264Data = &data[payloadOffset];
			singleNalUnit = true;
			nalUnitBegins = true;
			printf("Single NAL unit with %d bytes of data\n", size - payloadOffset);
		} else if (nalType == 28) {
			// FU-A
			h264Data = &data[payloadOffset + 2];
//			printf("Fragmented NAL unit with %d bytes of data\n", size - payloadOffset);
			totalFUCount++;
			if (startUnit || (data[payloadOffset + 1] & 0x80)) {
				printf("New NAL unit begins...\n");
				uint32_t nalType = data[payloadOffset + 1] & 0x1f;
				uint32_t nri = (data[payloadOffset + 0] >> 5) & 3;
				nalHeader = (nri << 5) | nalType;
				nalUnitBegins = true;
				startUnit = false;
			}
			if (data[payloadOffset + 1] & 0x40) {
				printf("Fragmented NAL unit complete with %d fragments\n", totalFUCount);
				totalFUCount = 0;
			}
		} else if (nalType == 24) {
			printf("Oops\n");
		}

		OMX_BUFFERHEADERTYPE *buf;
		int first_packet = 1;
		if ((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
		{
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
//			printf("Size: %d\n", dataSize);
			//data_len += fread(dest, 1, size - payloadOffset - 2, in);

			if(port_settings_changed == 0  &&
			((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
			 (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
													   ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
			{
				port_settings_changed = 1;

				printf("13\n");
				if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
				{
				   status = -7;
				   break;
				}

				printf("14\n");
				ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

				printf("15\n");
				// now setup tunnel to video_render
				if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
				{
				   status = -12;
				   break;
				}

				printf("16\n");
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
}
