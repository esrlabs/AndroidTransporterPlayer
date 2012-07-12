#include "RtspMediaSource.h"
#include "RtspSocket.h"
#include <errno.h>
#include <stdio.h>

using namespace android::os;
using namespace android::lang;

RtspMediaSource::RtspMediaSource(const sp<Handler>& player, const String& url) :
	mPlayer(player),
	mUrl(url) {
}

RtspMediaSource::~RtspMediaSource() {
}

void RtspMediaSource::start(sp<Message> reply) {
	if (!mUrl.startsWith("rtsp://")) {
		reply->arg1 = -1;
		reply->sendToTarget();
	}
	String serverAddress = mUrl.substr(strlen("rtsp://"));
	ssize_t separatorIndex = serverAddress.indexOf(":");
	if (separatorIndex > 0) {
		mHost = serverAddress.substr(0, separatorIndex);
		mPort = serverAddress.substr(separatorIndex + 1);
	} else {
		mHost = serverAddress;
		mPort = "1234";
	}
	mSocket = new RtspSocket();
	if (!mSocket->connect(mHost.c_str(), atoi(mPort.c_str()))) {
		reply->arg1 = -1;
		reply->sendToTarget();
	}
	reply->arg1 = 0;
	reply->sendToTarget();
}

void RtspMediaSource::stop() {
}

void RtspMediaSource::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	}
}

int32_t RtspMediaSource::dequeueBuffer(MediaSourceType type , android::os::sp<Buffer>* accessUnit) {
	sp<RtpMediaSource> mediaSource = getMediaSource(type);

	if (mediaSource == NULL) {
		return -EWOULDBLOCK;
	}

	if (mediaSource->empty()) {
		return -EWOULDBLOCK;
	}

	return mediaSource->dequeueAccessUnit(accessUnit);
}

void RtspMediaSource::describe() {
	String describeMessage = String::format("DESCRIBE rtsp://%s:%s/Test.sdp RTSP/1.0\r\nCSeq: 1\r\n\r\n", mHost.c_str(), mPort.c_str());
	mSocket->write(describeMessage.c_str(), describeMessage.size());
	RtspHeader* rtspHeader = mSocket->readPacket();
	printf("\nDESCRIBE:\n");
	uint32_t contentLength;
	RtspHeader::iterator itr = rtspHeader->begin();
	while (itr != rtspHeader->end()) {
		printf("%s: %s\n", itr->first.c_str(), itr->second.c_str());
		if (itr->first == "Content-Length") {
			contentLength = atoi(itr->second.c_str());
			uint8_t data[contentLength];
			mSocket->readFully(data, contentLength);
			printf("%s\n", String((char*)data, contentLength).c_str());
		}
		++itr;
	}
	delete rtspHeader;
}
