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

#ifndef MEDIAASSEMBLER_H_
#define MEDIAASSEMBLER_H_

#include "mindroid/os/Ref.h"

class MediaAssembler :
	public mindroid::Ref
{
public:
	enum Status {
		OK,
		PACKET_FAILURE,
		SEQ_NUMBER_FAILURE,
	};

	static const uint64_t TIME_PERIOD_20MS = 20000000LL;

	MediaAssembler();
	virtual ~MediaAssembler() { }
	virtual Status assembleMediaData() = 0;
	virtual uint32_t fixPacketLoss() const = 0; // returns the next sequence number
	void processMediaQueue();

protected:
	uint32_t mSeqNumber;
	bool mInitSeqNumber;

private:
	uint64_t mFirstSeqNumberFailureTime;
};

#endif /* MEDIAASSEMBLER_H_ */
