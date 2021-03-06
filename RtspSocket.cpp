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

#include "RtspSocket.h"
#include <stdio.h>
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

bool RtspSocket::readLine(sp<String>& result) {
	int32_t resultCode = 0;
	StringWrapper line;
	char character;
	bool CR = false;
	bool LF = false;
	while (!CR || !LF) {
		resultCode = readFully((uint8_t*)&character, 1);
		if (resultCode > 0) {
			if (character == '\r') {
				CR = true;
			} else if (CR && character == '\n') {
				LF = true;
				result = line.toString()->trim();
			} else {
				CR = false;
				LF = false;
			}
			line += character;
		} else {
			break;
		}
	}
	return resultCode > 0;
}

bool RtspSocket::readPacketHeader(RtspHeader*& rtspHeader) {
	rtspHeader = new RtspHeader();
	bool resultCode;
	sp<String> line;
	do {
		resultCode = readLine(line);
		if (resultCode && line->size() > 0) {
			if (rtspHeader->empty()) {
				sp< List< sp<String> > > resultCode = line->split(" ");
				if (resultCode->size() < 2) {
					delete rtspHeader;
					rtspHeader = NULL;
					return false;
				} else {
					List< sp<String> >::iterator itr = resultCode->begin();
					if (!(*itr)->trim()->equals("RTSP/1.0")) {
						delete rtspHeader;
						rtspHeader = NULL;
						return false;
					} else {
						++itr;
						(*rtspHeader)[StringWrapper("ResultCode")] = StringWrapper((*itr)->trim());
					}
				}
			} else {
				ssize_t pos = line->indexOf(":");
				sp<String> key;
				sp<String> value;
				if (pos >= 0) {
					key = line->substr(0, pos)->trim()->toLowerCase();
					value = line->substr(pos + 1)->trim();
				}
				if (!value->isEmpty()) {
					(*rtspHeader)[key] = value;
				}
			}
		}
	} while (resultCode && line->size() > 0);

	if (!resultCode) {
		delete rtspHeader;
		rtspHeader = NULL;
		return false;
	}

	return true;
}
