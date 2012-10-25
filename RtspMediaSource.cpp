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

#include "RtspMediaSource.h"
#include "NetHandler.h"
#include "mindroid/os/Message.h"
#include "mindroid/os/Handler.h"
#include "mindroid/util/Buffer.h"
#include <stdio.h>

using namespace mindroid;

RtspMediaSource::RtspMediaSource(const sp<Handler>& netHandler) :
		mNetHandler(netHandler),
		mCSeq(1),
		mTeardownDone(false) {
	mPendingTracks = new List< sp<Message> >();
}

RtspMediaSource::~RtspMediaSource() {
}

bool RtspMediaSource::start(const String& url) {
	if (!url.startsWith("rtsp://")) {
		return false;
	}

	String mediaSource = url.substr(strlen("rtsp://"));
	ssize_t separatorIndex = mediaSource.indexOf("/");
	if  (separatorIndex > 0) {
		mSdpFile = mediaSource.substr(separatorIndex + 1);
		mediaSource = mediaSource.substr(0, separatorIndex);
	} else {
		return false;
	}

	separatorIndex = mediaSource.indexOf(":");
	if (separatorIndex > 0) {
		mHost = mediaSource.substr(0, separatorIndex);
		mPort = mediaSource.substr(separatorIndex + 1);
	} else {
		mHost = mediaSource;
		mPort = "9000";
	}

	mSocket = new RtspSocket();
	if (mSocket->connect(mHost.c_str(), atoi(mPort.c_str())) != 0) {
		return false;
	}

	mNetReceiver = new NetReceiver(this);
	if (!mNetReceiver->start()) {
		return false;
	}

	describeMediaSource();

	return true;
}

void RtspMediaSource::stop(const sp<mindroid::Message>& reply) {
	if (mNetReceiver != NULL && (mAudioMediaSource.sessionId != NULL || mVideoMediaSource.sessionId != NULL)) {
		teardownMediaSource(reply);
	} else {
		reply->sendToTarget();
	}
}

void RtspMediaSource::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case DESCRIBE_MEDIA_SOURCE: {
		RtspHeader* rtspHeader = (RtspHeader*) message->obj;
		sp<Buffer> content;
		if (message->hasMetaData()) {
			sp<Bundle> bundle = message->metaData();
			content = bundle->getObject<Buffer>("Content");
		}
		if ((*rtspHeader)[String("ResultCode")] == "200") {
			onDescribeMediaSource(content);
		} else {
			// TODO: error handling
		}
		delete rtspHeader;
		break;
	}
	case SETUP_AUDIO_TRACK: {
		setupAudioTrack(NetHandler::RTP_AUDIO_SOURCE_PORT);
		break;
	}
	case SETUP_AUDIO_TRACK_DONE: {
		RtspHeader* rtspHeader = (RtspHeader*) message->obj;
		if ((*rtspHeader)[String("ResultCode")] == "200") {
			mAudioMediaSource.sessionId = *(*rtspHeader)[String("Session").toLowerCase()].split(";")->begin();

			sp<Message> msg = mNetHandler->obtainMessage(NetHandler::START_AUDIO_TRACK);
			msg->metaData()->putUInt32("Type", mAudioMediaSource.type);
			msg->metaData()->putString("TransportProtocol", mAudioMediaSource.transportProtocol);
			msg->metaData()->putString("ServerIpAddress", mAudioMediaSource.serverIpAddress);
			msg->metaData()->putUInt16("ServerPorts", mAudioMediaSource.serverPorts[0]);
			msg->metaData()->putString("ProfileId", mAudioMediaSource.profileId);
			msg->metaData()->putString("SpropParams", mAudioMediaSource.spropParams);
			msg->metaData()->putString("CodecConfig", mAudioMediaSource.codecConfig);
			msg->sendToTarget();
		} else {
			// TODO: error handling
		}
		delete rtspHeader;
		break;
	}
	case PLAY_AUDIO_TRACK: {
		playAudioTrack();
		break;
	}
	case PLAY_AUDIO_TRACK_DONE: {
		RtspHeader* rtspHeader = (RtspHeader*) message->obj;
		if ((*rtspHeader)[String("ResultCode")] == "200") {
			// OK
		} else {
			// TODO: error handling
		}
		delete rtspHeader;
		startPendingTracks();
		break;
	}
	case SETUP_VIDEO_TRACK: {
		setupVideoTrack(NetHandler::RTP_VIDEO_SOURCE_PORT);
		break;
	}
	case SETUP_VIDEO_TRACK_DONE: {
		RtspHeader* rtspHeader = (RtspHeader*) message->obj;
		if ((*rtspHeader)[String("ResultCode")] == "200") {
			mVideoMediaSource.sessionId = *(*rtspHeader)[String("Session").toLowerCase()].split(";")->begin();
			sp< List<String> > parts = (*rtspHeader)[String("Transport").toLowerCase()].split(";");
			List<String>::iterator itr = parts->begin();
			while (itr != parts->end()) {
				if (itr->startsWith("server_port=")) {
					String portRange = itr->substr(String::size("server_port="));
					sp< List<String> > ports = portRange.split("-");
					if (ports->size() == 2) {
						mVideoMediaSource.serverPorts[0] = atoi(ports->begin()->c_str());
						mVideoMediaSource.serverPorts[1] = mVideoMediaSource.serverPorts[0] + 1;
					}
				}
				++itr;
			}

			sp<Message> msg = mNetHandler->obtainMessage(NetHandler::START_VIDEO_TRACK);
			msg->metaData()->putUInt32("Type", mVideoMediaSource.type);
			msg->metaData()->putString("TransportProtocol", mVideoMediaSource.transportProtocol);
			msg->metaData()->putString("ServerIpAddress", mVideoMediaSource.serverIpAddress);
			msg->metaData()->putUInt16("ServerPorts", mVideoMediaSource.serverPorts[0]);
			msg->metaData()->putString("ProfileId", mVideoMediaSource.profileId);
			msg->metaData()->putString("SpropParams", mVideoMediaSource.spropParams);
			msg->metaData()->putString("CodecConfig", mVideoMediaSource.codecConfig);
			msg->sendToTarget();
		} else {
			// TODO: error handling
		}
		delete rtspHeader;
		break;
	}
	case PLAY_VIDEO_TRACK: {
		playVideoTrack();
		break;
	}
	case PLAY_VIDEO_TRACK_DONE: {
		RtspHeader* rtspHeader = (RtspHeader*) message->obj;
		if ((*rtspHeader)[String("ResultCode")] == "200") {
			// OK
		} else {
			// TODO: error handling
		}
		delete rtspHeader;
		startPendingTracks();
		break;
	}
	case TEARDOWN_AUDIO_TRACK: {
		mAudioMediaSource.sessionId = NULL;
		if (mVideoMediaSource.sessionId == NULL && !mTeardownDone) {
			mTeardownDone = true;
			sp<Bundle> bundle = message->metaData();
			sp<Message> reply = bundle->getObject<Message>("Reply");
			mNetReceiver->stop();
			mNetReceiver = NULL;
			reply->sendToTarget();
		}
		break;
	}
	case TEARDOWN_VIDEO_TRACK: {
		mVideoMediaSource.sessionId = NULL;
		if (mAudioMediaSource.sessionId == NULL && !mTeardownDone) {
			mTeardownDone = true;
			sp<Bundle> bundle = message->metaData();
			sp<Message> reply = bundle->getObject<Message>("Reply");
			if (mNetReceiver != NULL) {
				mNetReceiver->stop();
				mNetReceiver = NULL;
			}
			reply->sendToTarget();
		}
		break;
	}
	case MEDIA_SOURCE_HAS_QUIT: {
		mNetReceiver = NULL;
		mNetHandler->obtainMessage(NetHandler::MEDIA_SOURCE_HAS_QUIT)->sendToTarget();
		break;
	}
	}
}

void RtspMediaSource::describeMediaSource() {
	setPendingRequest(mCSeq, obtainMessage(DESCRIBE_MEDIA_SOURCE));
	String describeMessage = String::format("DESCRIBE rtsp://%s:%s/%s RTSP/1.0\r\nCSeq: %d\r\n\r\n",
			mHost.c_str(), mPort.c_str(), mSdpFile.c_str(), mCSeq++);
	mSocket->write(describeMessage.c_str(), describeMessage.size());
}

void RtspMediaSource::onDescribeMediaSource(const sp<Buffer>& desc) {
	// TODO: Build a SdpParser class that parses the service desc correctly :)

	mPendingTracks->clear();

	String mediaSourceDesc((char*)desc->data(), desc->size());
	sp< List<String> > lines = mediaSourceDesc.split("\n");
	List<String>::iterator itr = lines->begin();

	String audioMediaDesc;
	String videoMediaDesc;
	String mediaType;
	String profileId;
	String spropParams;
	String codecConfig;
	String transportProtocol;

	while (itr != lines->end()) {
		String line = itr->trim();
		if (line.startsWith("m=")) {
			if (line.startsWith("m=audio")) {
				sp< List<String> > strings = line.split(" ");
				List<String>::iterator itr = strings->begin();
				String port = *(++itr);
				String protocol = *(++itr);
				mediaType = *(++itr);
				if (protocol.trim() == "RTP/AVP") {
					audioMediaDesc = line;
					transportProtocol = "UDP";
				} else {
					audioMediaDesc = NULL;
				}
				videoMediaDesc = NULL;
			} else if (line.startsWith("m=video")) {
				sp< List<String> > strings = line.split(" ");
				List<String>::iterator itr = strings->begin();
				String port = *(++itr);
				String protocol = *(++itr);
				mediaType = *(++itr);
				if (protocol.trim() == "RTP/AVP") {
					videoMediaDesc = line;
					transportProtocol = "UDP";
				} else if (protocol.trim() == "TCP/RTP/AVP") {
					videoMediaDesc = line;
					transportProtocol = "TCP";
				} else {
					videoMediaDesc = NULL;
				}
				audioMediaDesc = NULL;
			} else {
				audioMediaDesc = NULL;
				videoMediaDesc = NULL;
			}
		} else if (line.startsWith("a=")) {
			if (line.startsWith("a=fmtp:")) {
				sp< List<String> > strings = line.split(";");
				List<String>::iterator itr = strings->begin();
				while (itr != strings->end()) {
					if (itr->trim().startsWith("profile-level-id=")) {
						ssize_t pos = itr->indexOf("=");
						profileId = itr->substr(pos + 1);
					} else if (itr->trim().startsWith("sprop-parameter-sets=")) {
						ssize_t pos = itr->indexOf("=");
						spropParams = itr->substr(pos + 1);
					} else if (itr->trim().startsWith("config=")) {
						ssize_t pos = itr->indexOf("=");
						codecConfig = itr->substr(pos + 1);
					}
					++itr;
				}
			} else if (line.startsWith("a=rtpmap")) {
				sp< List<String> > strings = line.split(" ");
				List<String>::iterator itr = strings->begin();
				if (strings->size() < 2) {
					audioMediaDesc = NULL;
					videoMediaDesc = NULL;
				}
				if (!audioMediaDesc.isEmpty()) {
					String rtpmapType = *(++itr);
					if (rtpmapType != "mpeg4-generic/44100/2" && rtpmapType != "L16/44100/2") {
						audioMediaDesc = NULL;
						// TODO: add support for other audio streams
					}
				} else if (!videoMediaDesc.isEmpty()) {
					if (*(++itr) != "H264/90000") {
						videoMediaDesc = NULL;
					}
				}
			} else if (line.startsWith("a=control:")) {
				if (!audioMediaDesc.isEmpty()) {
					mAudioMediaSource.url = line.substr(String::size("a=control:")).trim();
					mAudioMediaSource.type = atoi(mediaType.c_str());
					mAudioMediaSource.transportProtocol = transportProtocol;
					mAudioMediaSource.profileId = profileId;
					mAudioMediaSource.spropParams = spropParams;
					mAudioMediaSource.codecConfig = codecConfig;

					mPendingTracks->push_back(obtainMessage(SETUP_AUDIO_TRACK));
				} else if (!videoMediaDesc.isEmpty()) {
					mVideoMediaSource.url = line.substr(String::size("a=control:")).trim();
					mVideoMediaSource.type = atoi(mediaType.c_str());
					mVideoMediaSource.transportProtocol = transportProtocol;
					mVideoMediaSource.profileId = profileId;
					mVideoMediaSource.spropParams = spropParams;
					mVideoMediaSource.codecConfig = codecConfig;

					mPendingTracks->push_back(obtainMessage(SETUP_VIDEO_TRACK));
				}
			}
		} else if (line.startsWith("c=")) {
			sp< List<String> > strings = line.substr(String::size("c=")).trim().split(" ");
			if (strings->size() >= 3) {
				List<String>::iterator itr = strings->begin();
				if (*itr++ == "IN" && *itr++ == "IP4") {
					mAudioMediaSource.serverIpAddress = *itr;
					mVideoMediaSource.serverIpAddress = *itr;
				}
			}
		}
		++itr;
	}

	if (mPendingTracks->size() > 0) {
		startPendingTracks();
	} else {
		printf("The media source does not offer any audio or video streams.\n");
		mNetHandler->obtainMessage(NetHandler::MEDIA_SOURCE_HAS_NO_STREAMS)->sendToTarget();
	}
}

void RtspMediaSource::startPendingTracks() {
	List< sp<Message> >::iterator itr = mPendingTracks->begin();
	if (itr != mPendingTracks->end()) {
		sp<Message> msg = (*itr);
		itr = mPendingTracks->erase(itr);
		msg->sendToTarget();
	}
}

void RtspMediaSource::setupAudioTrack(uint16_t port) {
	setPendingRequest(mCSeq, obtainMessage(SETUP_AUDIO_TRACK_DONE));
	String setupMessage = String::format("SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n",
			mAudioMediaSource.url.c_str(), mCSeq++, port, port + 1);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playAudioTrack() {
	setPendingRequest(mCSeq, obtainMessage(PLAY_AUDIO_TRACK_DONE));
	String playMessage = String::format("PLAY %s RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mAudioMediaSource.url.c_str(), mCSeq++, mAudioMediaSource.sessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

void RtspMediaSource::setupVideoTrack(uint16_t port) {
	setPendingRequest(mCSeq, obtainMessage(SETUP_VIDEO_TRACK_DONE));
	String setupMessage = String::format("SETUP %s RTSP/1.0\r\nCSeq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n",
			mVideoMediaSource.url.c_str(), mCSeq++, port, port + 1);
	mSocket->write(setupMessage.c_str(), setupMessage.size());
}

void RtspMediaSource::playVideoTrack() {
	setPendingRequest(mCSeq, obtainMessage(PLAY_VIDEO_TRACK_DONE));
	String playMessage = String::format("PLAY %s RTSP/1.0\r\nCSeq: %d\r\nRange: npt=0.000-\r\nSession: %s\r\n\r\n",
			mVideoMediaSource.url.c_str(), mCSeq++, mVideoMediaSource.sessionId.c_str());
	mSocket->write(playMessage.c_str(), playMessage.size());
}

void RtspMediaSource::teardownMediaSource(const sp<mindroid::Message>& reply) {
	if (mAudioMediaSource.sessionId != NULL) {
		sp<Message> msg = obtainMessage(TEARDOWN_AUDIO_TRACK);
		msg->metaData()->putObject("Reply", reply);
		setPendingRequest(mCSeq, msg);
		String teardownMessage = String::format("TEARDOWN rtsp://%s:%s/%s RTSP/1.0\r\nCSeq: %d\r\nSession: %s\r\n\r\n",
				mHost.c_str(), mPort.c_str(), mSdpFile.c_str(), mCSeq++, mAudioMediaSource.sessionId.c_str());
		mSocket->write(teardownMessage.c_str(), teardownMessage.size());

		// Tear down the audio track if we do not receive a response within 2 seconds.
		sendMessageDelayed(msg, TIMEOUT_2_SECONDS);
	}
	if (mVideoMediaSource.sessionId != NULL) {
		sp<Message> msg = obtainMessage(TEARDOWN_VIDEO_TRACK);
		msg->metaData()->putObject("Reply", reply);
		setPendingRequest(mCSeq, msg);
		String teardownMessage = String::format("TEARDOWN rtsp://%s:%s/%s RTSP/1.0\r\nCSeq: %d\r\nSession: %s\r\n\r\n",
				mHost.c_str(), mPort.c_str(), mSdpFile.c_str(), mCSeq++, mVideoMediaSource.sessionId.c_str());
		mSocket->write(teardownMessage.c_str(), teardownMessage.size());

		// Tear down the video track if we do not receive a response within 2 seconds.
		sendMessageDelayed(msg, TIMEOUT_2_SECONDS);
	}
}

void RtspMediaSource::setPendingRequest(uint32_t id, const sp<Message>& message) {
	AutoLock autoLock(mLock);
	mPendingRequests[id] = message;
}

sp<Message> RtspMediaSource::getPendingRequest(uint32_t id) {
	AutoLock autoLock(mLock);
	return mPendingRequests[id];
}

sp<Message> RtspMediaSource::removePendingRequest(uint32_t id) {
	AutoLock autoLock(mLock);
	sp<Message> message =  mPendingRequests[id];
	mPendingRequests.erase(id);
	return message;
}

RtspMediaSource::NetReceiver::NetReceiver(const sp<RtspMediaSource>& mediaSource) :
		mMediaSource(mediaSource) {
	mSocket = mMediaSource->getSocket();
}

void RtspMediaSource::NetReceiver::run() {
	RtspHeader* rtspHeader;
	while (!isInterrupted()) {
		bool result = mSocket->readPacketHeader(rtspHeader);
		if (result) {
			if (rtspHeader != NULL && !rtspHeader->empty()) {
				if ((*rtspHeader)[String("ResultCode")] == "200") {
					uint32_t seqNum = atoi((*rtspHeader)[String("CSeq").toLowerCase()]);
					sp<Message> reply = mMediaSource->removePendingRequest(seqNum);
					reply->obj = rtspHeader;

					String strContentLength = (*rtspHeader)[String("Content-Length").toLowerCase()];
					if (strContentLength != NULL) {
						size_t contentLength = atoi(strContentLength.c_str());
						if (contentLength > 0) {
							sp<Buffer> buffer = new Buffer(contentLength);
							mSocket->readFully(buffer->data(), contentLength);
							buffer->setRange(0, contentLength);
							sp<Bundle> bundle = reply->metaData();
							bundle->putObject("Content", buffer);
						}
					}

					reply->sendToTarget();
				} else {
					if ((*rtspHeader)[String("ResultCode")] == "404") {
						printf("RTSP media source is not available.\n");
					}
					mMediaSource->obtainMessage(MEDIA_SOURCE_HAS_QUIT)->sendToTarget();
				}
			}
		} else {
			mMediaSource->obtainMessage(MEDIA_SOURCE_HAS_QUIT)->sendToTarget();
			break;
		}
	}
}

void RtspMediaSource::NetReceiver::stop() {
	interrupt();
	mSocket->close();
	join();
	mMediaSource = NULL;
}
