#include <stdio.h>
#include "mindroid/os/Looper.h"
#include "mindroid/lang/String.h"
#include "RPiPlayer.h"

using namespace mindroid;

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
