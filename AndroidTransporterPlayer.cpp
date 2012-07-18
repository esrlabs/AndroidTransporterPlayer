#include <stdio.h>
#include "android/os/Looper.h"
#include "android/lang/String.h"
#include "RPiPlayer.h"

using namespace android::os;
using namespace android::lang;

int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("Usage: rtsp://<Host>:<Port>/<SDP-File>\n");
		return -1;
	}

	Looper::prepare();
	Thread::currentThread()->setSchedulingParams(SCHED_OTHER, -17);
	sp<RPiPlayer> rPiPlayer = new RPiPlayer();
	rPiPlayer->start(String(argv[1]));
	Looper::loop();

	return 0;
}
