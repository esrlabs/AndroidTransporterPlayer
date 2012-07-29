#include <stdio.h>
#include <signal.h>
#include <mindroid/os/Looper.h>
#include <mindroid/lang/String.h>
#include "RPiPlayer.h"

using namespace mindroid;

sp<RPiPlayer> rpiPlayer;

void shutdownHook(int signal) {
	if (signal == SIGINT) {
		rpiPlayer->stop();
	}
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("Usage: rtsp://<Host>:<Port>/<SDP-File>\n");
		return -1;
	}

	signal(SIGINT, shutdownHook);

	Thread::currentThread()->setSchedulingParams(SCHED_OTHER, -17);
	Looper::prepare();
	rpiPlayer = new RPiPlayer();
	rpiPlayer->start(String(argv[1]));
	Looper::loop();

	return 0;
}
