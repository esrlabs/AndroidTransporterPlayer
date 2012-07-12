#include "RtspMediaSource.h"
#include "Bundle.h"
#include <stdio.h>

using namespace android::os;
using namespace android::lang;
using namespace android::net;

RtspMediaSource::RtspMediaSource() {
}

RtspMediaSource::~RtspMediaSource() {
}

void RtspMediaSource::setupMediaSource(const sp<Handler>& netHandler, const String& url, const sp<Message>& reply) {
	sp<Message> message = obtainMessage(SETUP_MEDIA_SOURCE);
	message->obj = new Bundle(netHandler, url, reply);
	message->sendToTarget();
}

void RtspMediaSource::describeService() {
	sp<Message> message = obtainMessage(DESCRIBE_SERVICE);
	message->sendToTarget();
}

void RtspMediaSource::handleMessage(const sp<Message>& message) {
	switch(message->what) {
	case SETUP_MEDIA_SOURCE: {
		Bundle* bundle = (Bundle*) message->obj;
		mNetHandler = bundle->arg1;
		mUrl = bundle->arg2;
		sp<Message> reply = bundle->reply;
		delete bundle;

		if (!mUrl.startsWith("rtsp://")) {
			reply->arg1 = -1;
			reply->sendToTarget();
		}
		String mediaSource = mUrl.substr(strlen("rtsp://"));
		ssize_t separatorIndex = mediaSource.indexOf(":");
		if (separatorIndex > 0) {
			mHost = mediaSource.substr(0, separatorIndex);
			mPort = mediaSource.substr(separatorIndex + 1);
		} else {
			mHost = mediaSource;
			mPort = "1234";
		}
		mSocket = new RtspSocket();
		if (!mSocket->connect(mHost.c_str(), atoi(mPort.c_str()))) {
			reply->arg1 = -1;
			reply->sendToTarget();
		}
		reply->arg1 = 0;
		reply->sendToTarget();

		break;
	}
	case DESCRIBE_SERVICE: {
		String describeMessage = String::format("DESCRIBE rtsp://%s:%s/Test.sdp RTSP/1.0\r\nCSeq: 1\r\n\r\n", mHost.c_str(), mPort.c_str());
		int res = mSocket->write(describeMessage.c_str(), describeMessage.size());
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
		break;
	}
	}
}
