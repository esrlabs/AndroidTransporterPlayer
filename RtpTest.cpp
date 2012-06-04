#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Socket.h"
#include "DatagramSocket.h"

using namespace android::net;

int32_t readFully(Socket* socket, uint8_t* data, const uint32_t size) {
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
	
	Socket* mSocket = new Socket();
	if (!mSocket->connect("192.168.0.112", 1234)) {
		printf("Error\n");
		return -1;
	}

	char* optionsMessage = "OPTIONS rtsp://192.168.0.112:1234/Test.sdp RTSP/1.0\r\nCSeq: 1\r\n\r\n";
	mSocket->write(optionsMessage, strlen(optionsMessage));
	memset(buffer, 0, 4096);
	int32_t size = readFully(mSocket, buffer, 4096);
	printf("OPTIONS: %s\n", buffer);

	char* describeMessage = "DESCRIBE rtsp://192.168.0.112:1234/Test.sdp RTSP/1.0\r\nCSeq: 2\r\n\r\n";
	mSocket->write(describeMessage, strlen(describeMessage));	
	memset(buffer, 0, 4096);
	size = readFully(mSocket, buffer, 4096);
	printf("DESCRIBE: %s\n", buffer);

	DatagramSocket* rtpSocket = new DatagramSocket(56098);
	DatagramSocket* rtcpSocket = new DatagramSocket(56099);

	char* setupMessage = "SETUP rtsp://192.168.0.112:1234/Test.sdp/trackID=0 RTSP/1.0\r\nCSeq: 3\r\nTransport: RTP/AVP;unicast;client_port=56098-56099\r\n\r\n";
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
	strcpy(playMessage, "PLAY rtsp://192.168.0.112:1234/Test.sdp RTSP/1.0\r\nCSeq: 4\r\nRange: npt=0.000-\r\nSession: ");
	strcat(playMessage, sessionId);
	strcat(playMessage, "\r\n\r\n");
	mSocket->write(playMessage, strlen(playMessage));	
	memset(buffer, 0, 4096);
	size = readFully(mSocket, buffer, 4096);
	printf("PLAY: %s\n", buffer);

	printf("Waiting for data...\n");
	while (true) {
		int size = rtpSocket->recv(buffer, 4096);
		printf("%d\n", size);
	}
}
