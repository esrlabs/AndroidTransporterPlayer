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
 * Refactorings by E.S.R.Labs GmbH
 */

#include "MediaAssembler.h"
#include "mindroid/os/Clock.h"
#include "mindroid/util/Buffer.h"

using namespace mindroid;

MediaAssembler::MediaAssembler() :
		mSeqNumber(0),
		mInitSeqNumber(true),
		mFirstSeqNumberFailureTime(0) {
}

void MediaAssembler::processMediaQueue() {
    Status status;

    while (true) {
        status = assembleMediaData();

        if (status == OK) {
        	mFirstSeqNumberFailureTime = 0;
			break;
		} else if (status == SEQ_NUMBER_FAILURE) {
            if (mFirstSeqNumberFailureTime != 0) {
                if (Clock::monotonicTime() - mFirstSeqNumberFailureTime > TIME_PERIOD_20MS) {
                	mFirstSeqNumberFailureTime = 0;
                	// We lost that packet. Empty the media queue.
                	mSeqNumber = getNextSeqNum();
                    continue;
                }
            } else {
            	mFirstSeqNumberFailureTime = Clock::monotonicTime();
            }
            break;
        } else {
        	mFirstSeqNumberFailureTime = 0;
        }
    }
}
