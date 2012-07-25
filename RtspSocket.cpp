#include "RtspSocket.h"
#include <sys/socket.h>
#include "mindroid/util/List.h"

using namespace std;
using namespace mindroid;

RtspSocket::RtspSocket() :
	Socket() {
}

RtspSocket::RtspSocket(const char* host, uint16_t port) :
	Socket() {
	connect(host, port);
}

String RtspSocket::readLine() {
	String line;
	int32_t result = 0;
	char character;
	bool CR = false;
	bool LF = false;
	while (!CR || !LF) {
		result = readFully((uint8_t*)&character, 1);
		if (result > 0) {
			if (character == '\r') {
				CR = true;
			} else if (CR && character == '\n') {
				LF = true;
			} else {
				CR = false;
				LF = false;
			}
			line += character;
		} else {
			break;
		}
	}
	return result > 0 ? line.trim() : String(NULL);
}

RtspHeader* RtspSocket::readPacketHeader() {
	RtspHeader* rtspHeader = new RtspHeader();
	String line;
	do {
		line = readLine();
		if (line.size() > 0) {
			if (rtspHeader->empty()) {
				sp< List<String> > resultCode = line.split(" ");
				if (resultCode->size() < 2) {
					delete rtspHeader;
					return NULL;
				} else {
					List<String>::iterator itr = resultCode->begin();
					if (itr->trim() != "RTSP/1.0") {
						delete rtspHeader;
						return NULL;
					} else {
						++itr;
						(*rtspHeader)[String("ResultCode")] = itr->trim();
					}
				}
			} else {
				ssize_t pos = line.indexOf(":");
				String key, value;
				if (pos >= 0) {
					key = line.substr(0, pos).trim().toLowerCase();
					value = line.substr(pos + 1).trim();
				}
				if (!value.isNull()) {
					(*rtspHeader)[key] = value;
				}
			}
		}
	} while (!line.isNull() && line.size() > 0);

	if (line.isNull()) {
		delete rtspHeader;
	}

	return !line.isNull() ? rtspHeader : NULL;
}
