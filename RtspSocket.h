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

#ifndef RTSPSOCKET_H_
#define RTSPSOCKET_H_

#include "mindroid/net/Socket.h"
#include "mindroid/lang/String.h"
#include "mindroid/lang/StringWrapper.h"
#include <map>

typedef std::map<mindroid::StringWrapper, mindroid::StringWrapper> RtspHeader;

using mindroid::sp;

class RtspSocket :
	public mindroid::Socket
{
public:

	RtspSocket();
	RtspSocket(const char* host, uint16_t port);
	virtual ~RtspSocket() {}
	bool readLine(sp<mindroid::String>& result);
	bool readPacketHeader(RtspHeader*& rtspHeader);

private:
	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtspSocket)
};


#endif /* RTSPSOCKET_H_ */
