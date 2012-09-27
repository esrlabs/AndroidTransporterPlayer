/*
 * Copyright (C) 2010 The Android Open Source Project
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
 *
 * Additions and refactorings by E.S.R.Labs GmbH
 */

#include "RtpMediaSource.h"
#include "mindroid/os/Looper.h"
#include "mindroid/os/Handler.h"
#include "mindroid/os/Closure.h"
#include "mindroid/net/DatagramSocket.h"
#include "mindroid/net/Socket.h"
#include "mindroid/util/Buffer.h"
#include "MediaAssembler.h"
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <errno.h>

using namespace mindroid;

RtpMediaSource::NetReceiver::NetReceiver() {
	// This pipe is used to unblock the 'select' call when stopping the NetReceiver.
	pipe(mPipe);
	int flags = fcntl(mPipe[0], F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(mPipe[0], F_SETFL, flags);
	flags = fcntl(mPipe[1], F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(mPipe[1], F_SETFL, flags);
}

void RtpMediaSource::NetReceiver::stop() {
	// mNotifyRtpPacket holds a (circular) dependency to the RtpMediaSource handler.
	mNotifyRtpPacket = NULL;
	mNotifyRtcpPacket = NULL;
}

void RtpMediaSource::NetReceiver::createNotifMessages(const sp<Handler>& hander) {
	mNotifyRtpPacket = hander->obtainMessage(NOTIFY_RTP_PACKET);
	mNotifyRtcpPacket = hander->obtainMessage(NOTIFY_RTCP_PACKET);
}

RtpMediaSource::UdpNetReceiver::UdpNetReceiver(uint16_t port) {
	mRtpSocket = new DatagramSocket(port);
	mRtcpSocket = new DatagramSocket(port + 1);
	// We saw some drops when working with standard buffer sizes, so give the sockets 256KB buffer.
	int size = 256 * 1024;
	setsockopt(mRtpSocket->getId(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

void RtpMediaSource::UdpNetReceiver::stop() {
	interrupt();
	mRtpSocket->close();
	mRtcpSocket->close();
	write(mPipe[1], "X", 1);
	join();
	NetReceiver::stop();
}

void RtpMediaSource::UdpNetReceiver::run() {
	setpriority(PRIO_PROCESS, 0, -16);

	while (!isInterrupted()) {
		fd_set sockets;
		FD_ZERO(&sockets);

		FD_SET(mRtpSocket->getId(), &sockets);
		FD_SET(mRtcpSocket->getId(), &sockets);
		FD_SET(mPipe[0], &sockets);

		int maxId = mRtpSocket->getId();
		if (mRtcpSocket->getId() > maxId) {
			maxId = mRtcpSocket->getId();
		}
		if (mPipe[0] > maxId) {
			maxId = mPipe[0];
		}

		int result = select(maxId + 1, &sockets, NULL, NULL, NULL);

		if (result > 0) {
			if (FD_ISSET(mRtpSocket->getId(), &sockets)) {
				sp<Buffer> buffer(new Buffer(MAX_UDP_PACKET_SIZE));
				ssize_t size = mRtpSocket->recv(buffer->data(), buffer->capacity());
				if (size > 0) {
					buffer->setRange(0, size);
					sp<Message> msg = mNotifyRtpPacket->dup();
					sp<Bundle> bundle = msg->metaData();
					bundle->putObject("RTP-Packet", buffer);
					msg->sendToTarget();
				}
			} else if (FD_ISSET(mRtcpSocket->getId(), &sockets)) {
				sp<Buffer> buffer(new Buffer(MAX_UDP_PACKET_SIZE));
				ssize_t size = mRtcpSocket->recv(buffer->data(), buffer->capacity());
				if (size > 0) {
					buffer->setRange(0, size);
					sp<Message> msg = mNotifyRtcpPacket->dup();
					sp<Bundle> bundle = msg->metaData();
					bundle->putObject("RTCP-Packet", buffer);
					msg->sendToTarget();
				}
			}
		}
	}
}

RtpMediaSource::TcpNetReceiver::TcpNetReceiver(String hostName, uint16_t port) :
		mLooper(NULL),
		mHostName(hostName),
		mPort(port) {
}

void RtpMediaSource::TcpNetReceiver::stop() {
	if (mLooper != NULL) {
		mLooper->quit();
	}
	write(mPipe[1], "X", 1);
	if (mRtpSocket != NULL) {
		mRtpSocket->close();
	}
	if (mRtcpSocket != NULL) {
		mRtcpSocket->close();
	}
	join();
	mHandler->removeCallbacksAndMessages();
	mHandler = NULL;
	NetReceiver::stop();
}

void RtpMediaSource::TcpNetReceiver::run() {
	setpriority(PRIO_PROCESS, 0, -16);

	Looper::prepare();
	mLooper = Looper::myLooper();
	sp<TcpNetReceiverHandler> handler = new TcpNetReceiverHandler(this);
	setHandler(handler);
	sp<Runnable> runnable = newRunnable<TcpNetReceiver, sp<Socket>, String, uint16_t, uint16_t>(*this, &TcpNetReceiver::asyncConnectToServer, new Socket(), mHostName, mPort, 0);
	handler->post(runnable);
	Looper::loop();
}

void RtpMediaSource::TcpNetReceiver::asyncConnectToServer(sp<Socket> socket, String hostName, uint16_t port, uint16_t retryCounter) {
	// We saw some drops when working with standard buffer sizes, so give the sockets 256KB buffer.
	int size = 256 * 1024;
	setsockopt(socket->getId(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	socket->setBlockingMode(false);

	sp<Message> reply = mHandler->obtainMessage();
	sp<Bundle> metaData = reply->metaData();
	metaData->putObject("Socket", socket);
	metaData->putString("HostName", hostName);
	metaData->putUInt16("Port", port);
	metaData->putUInt16("Retry-Counter", retryCounter);

	int rc = socket->connect(hostName.c_str(), port);
	if (rc < 0) {
		if (errno == EINPROGRESS) {
			reply->what = ON_CONNECT_TO_SERVER_PENDING;
			mHandler->sendMessageDelayed(reply, 1);
			return;
		}

		reply->what = ON_CONNECT_TO_SERVER_RETRY;
		mHandler->sendMessageDelayed(reply, 10);
	} else {
		reply->what = ON_CONNECT_TO_SERVER_DONE;
		reply->sendToTarget();
	}
}

void RtpMediaSource::TcpNetReceiver::onConnectToServerDone(const sp<Message>& message) {
	sp<Socket> socket = message->metaData()->getObject<Socket>("Socket");
	String hostName = message->metaData()->getString("HostName");
	uint16_t port;
	message->metaData()->fillUInt16("Port", port);

	if (mRtpSocket == NULL) {
		mRtpSocket = socket;
		asyncConnectToServer(new Socket(), hostName, port + 1, 0);
	} else {
		mRtcpSocket = socket;
		mHandler->obtainMessage(ON_RECV_DATA)->sendToTarget();
	}
}

void RtpMediaSource::TcpNetReceiver::onConnectToServerPending(const sp<Message>& message) {
	sp<Socket> socket = message->metaData()->getObject<Socket>("Socket");

	fd_set writeFds;
	FD_ZERO(&writeFds);
	FD_SET(socket->getId(), &writeFds);

	int maxId = socket->getId();

	int rc = select(maxId + 1, NULL, &writeFds, NULL, NULL);

	int errorCode;
	socklen_t ecSize = sizeof(errorCode);
	getsockopt(socket->getId(), SOL_SOCKET, SO_ERROR, &errorCode, &ecSize);

	sp<Message> reply = message;
	if (errorCode != 0) {
		reply->what = ON_CONNECT_TO_SERVER_RETRY;
		uint16_t retryCounter;
		reply->metaData()->fillUInt16("Retry-Counter", retryCounter);
		reply->metaData()->remove("Retry-Counter");
		reply->metaData()->putUInt16("Retry-Counter", retryCounter + 1);
		mHandler->sendMessageDelayed(reply, 10);
	} else {
		reply->what = ON_CONNECT_TO_SERVER_DONE;
		reply->sendToTarget();
	}
}

void RtpMediaSource::TcpNetReceiver::onConnectToServerRetry(const sp<Message>& message) {
	sp<Socket> socket = message->metaData()->getObject<Socket>("Socket");
	uint16_t retryCounter;
	message->metaData()->fillUInt16("Retry-Counter", retryCounter);

	if (retryCounter > 20) {
		sp<Message> reply = message;
		reply->what = ON_CONNECT_TO_SERVER_ERROR;
		reply->sendToTarget();
		return;
	}

	socket->close();
	socket.clear();

	String hostName = message->metaData()->getString("HostName");
	uint16_t port;
	message->metaData()->fillUInt16("Port", port);
	asyncConnectToServer(new Socket(), hostName, port, retryCounter);
}

void RtpMediaSource::TcpNetReceiver::onConnectToServerError(const sp<Message>& message) {
	String hostName = message->metaData()->getString("HostName");
	uint16_t port;
	message->metaData()->fillUInt16("Port", port);
	printf("Cannot connect to %s:%d\n", hostName.c_str(), port);
}

void RtpMediaSource::TcpNetReceiver::onReceiveData(const sp<Message>& message) {
	bool shutdown = false;

	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(mRtpSocket->getId(), &readFds);
	FD_SET(mRtcpSocket->getId(), &readFds);
	FD_SET(mPipe[0], &readFds);

	int maxId = mRtpSocket->getId();
	if (mRtcpSocket->getId() > maxId) {
		maxId = mRtcpSocket->getId();
	}
	if (mPipe[0] > maxId) {
		maxId = mPipe[0];
	}

	int rc = select(maxId + 1, &readFds, NULL, NULL, NULL);
	if (rc > 0) {
		uint16_t bePacketSize;
		if (FD_ISSET(mRtpSocket->getId(), &readFds)) {
			sp<Buffer> buffer(new Buffer(MAX_TCP_PACKET_SIZE));
			mRtpSocket->setBlockingMode(true);
			mRtpSocket->readFully((uint8_t*) &bePacketSize, 2);
			ssize_t size = mRtpSocket->readFully(buffer->data(), ntohs(bePacketSize));
			mRtpSocket->setBlockingMode(false);
			if (size > 0) {
				buffer->setRange(0, size);
				sp<Message> msg = mNotifyRtpPacket->dup();
				sp<Bundle> bundle = msg->metaData();
				bundle->putObject("RTP-Packet", buffer);
				msg->sendToTarget();
			}
		}
		if (FD_ISSET(mRtcpSocket->getId(), &readFds)) {
			sp<Buffer> buffer(new Buffer(MAX_TCP_PACKET_SIZE));
			mRtcpSocket->setBlockingMode(true);
			mRtcpSocket->readFully((uint8_t*) &bePacketSize, 2);
			ssize_t size = mRtcpSocket->readFully(buffer->data(), ntohs(bePacketSize));
			mRtcpSocket->setBlockingMode(false);
			if (size > 0) {
				buffer->setRange(0, size);
				sp<Message> msg = mNotifyRtpPacket->dup();
				sp<Bundle> bundle = msg->metaData();
				bundle->putObject("RTP-Packet", buffer);
				msg->sendToTarget();
			}
		}
		if (FD_ISSET(mPipe[0], &readFds)) {
			shutdown = true;
		}
	}

	if (!shutdown) {
		message->sendToTarget();
	}
}

void RtpMediaSource::TcpNetReceiver::TcpNetReceiverHandler::handleMessage(const sp<Message>& message) {
	switch (message->what) {
	case ON_CONNECT_TO_SERVER_DONE:
		mTcpNetReceiver->onConnectToServerDone(message);
		break;
	case ON_CONNECT_TO_SERVER_PENDING:
		mTcpNetReceiver->onConnectToServerPending(message);
		break;
	case ON_CONNECT_TO_SERVER_RETRY:
		mTcpNetReceiver->onConnectToServerRetry(message);
		break;
	case ON_CONNECT_TO_SERVER_ERROR:
		mTcpNetReceiver->onConnectToServerError(message);
		break;
	case ON_RECV_DATA:
		mTcpNetReceiver->onReceiveData(message);
		break;
	}
}

RtpMediaSource::RtpMediaSource(sp<NetReceiver> netReceiver) :
		mRtpPacketCounter(0),
		mHighestSeqNumber(0),
		mNetReceiver(netReceiver) {
	mQueue = new List< sp<Buffer> >();
	mNetReceiver->createNotifMessages(this);
}

RtpMediaSource::~RtpMediaSource() {
}

bool RtpMediaSource::start(sp<MediaAssembler> mediaAssembler) {
	mMediaAssembler = mediaAssembler;
	return mNetReceiver->start();
}

void RtpMediaSource::stop() {
	mNetReceiver->stop();
	removeCallbacksAndMessages();
}

void RtpMediaSource::handleMessage(const sp<Message>& message) {
    switch (message->what) {
        case NOTIFY_RTP_PACKET: {
            sp<Bundle> bundle = message->metaData();
            sp<Buffer> buffer = bundle->getObject<Buffer>("RTP-Packet");
            if (parseRtpHeader(buffer) == 0) {
                processRtpPayload(buffer);
            }
            break;
        }
        case NOTIFY_RTCP_PACKET: {
			sp<Bundle> bundle = message->metaData();
			sp<Buffer> buffer = bundle->getObject<Buffer>("RTCP-Packet");
            break;
        }
        default:
            break;
    }
}

int RtpMediaSource::parseRtpHeader(const sp<Buffer>& buffer) {
	size_t size = buffer->size();
	const uint8_t* data = buffer->data();

	if (size < RTP_HEADER_SIZE) {
		return -1;
	}

	if ((data[0] >> 6) != 2) { // v2.0
		return -1;
	}

	if (data[0] & PADDING_BIT) {
		size_t paddingSize = data[size - 1];
		if (paddingSize + RTP_HEADER_SIZE > size) {
			return -1;
		}
		size -= paddingSize;
	}

	int numCSRCs = data[0] & 0x0F;
	size_t payloadOffset = 12 + 4 * numCSRCs;
	if (size < payloadOffset) {
		return -1;
	}

	if (data[0] & EXT_HEADER_BIT) {
		if (size < payloadOffset + 4) {
			return -1;
		}

		const uint8_t* extensionData = &data[payloadOffset];

		size_t extensionSize =
			4 * (extensionData[2] << 8 | extensionData[3]);

		if (size < payloadOffset + 4 + extensionSize) {
			return -1;
		}

		payloadOffset += 4 + extensionSize;
	}

	buffer->setRange(payloadOffset, size - payloadOffset);

	uint16_t seqNum = data[2] << 8 | data[3];
	uint32_t srcId = data[8] << 24 | data[9] << 16 | data[10] << 8 | data[11];
	uint32_t rtpTime = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];

	buffer->setId(seqNum);
	buffer->metaData()->putUInt32("SSRC", srcId);
	buffer->metaData()->putUInt32("RTP-Time", rtpTime);
	buffer->metaData()->putUInt32("PT", data[1] & 0x7f);
	buffer->metaData()->putBool("M", data[1] & MARK_BIT);

	return 0;
}

void RtpMediaSource::processRtpPayload(const sp<Buffer>& buffer) {
	uint32_t seqNum = (uint32_t)buffer->getId();

	if (mRtpPacketCounter++ == 0) {
		mHighestSeqNumber = seqNum;
		mQueue->push_back(buffer);
		mMediaAssembler->processMediaQueue();
		return;
	}

	// Expand the sequence number to a 32 bit value.
	uint32_t seqNum1 = seqNum | (mHighestSeqNumber & 0xFFFF0000);
	uint32_t seqNum2 = seqNum | ((mHighestSeqNumber & 0xFFFF0000) + 0x10000);
	uint32_t seqNum3 = seqNum | ((mHighestSeqNumber & 0xFFFF0000) - 0x10000);
	uint32_t diffNum1 = seqNum1 > mHighestSeqNumber ? seqNum1 - mHighestSeqNumber : mHighestSeqNumber - seqNum1;
	uint32_t diffNum2 = seqNum2 > mHighestSeqNumber ? seqNum2 - mHighestSeqNumber : mHighestSeqNumber - seqNum2;
	uint32_t diffNum3 = seqNum3 > mHighestSeqNumber ? seqNum3 - mHighestSeqNumber : mHighestSeqNumber - seqNum3;

	if (diffNum1 < diffNum2) {
		if (diffNum1 < diffNum3) {
			seqNum = seqNum1;
		} else {
			seqNum = seqNum3;
		}
	} else if (diffNum2 < diffNum3) {
		seqNum = seqNum2;
	} else {
		seqNum = seqNum3;
	}

	if (seqNum > mHighestSeqNumber) {
		mHighestSeqNumber = seqNum;
	}

	buffer->setId(seqNum);

	List< sp<Buffer> >::iterator itr = mQueue->begin();
	while (itr != mQueue->end() && (uint32_t)(*itr)->getId() < seqNum) {
		++itr;
	}

	if (itr != mQueue->end() && (uint32_t)(*itr)->getId() == seqNum) {
		return;
	}

	mQueue->insert(itr, buffer);
	mMediaAssembler->processMediaQueue();
}
