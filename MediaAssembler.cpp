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
