#include "RPiPlayer.h"
#include <mindroid/os/Looper.h>
#include <mindroid/lang/String.h>
#include <stdio.h>
#include <signal.h>
#include <sys/resource.h>

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

	if (setpriority(PRIO_PROCESS, 0, -17) != 0) {
		printf("Cannot set thread priorities. Please restart with more privileges.\n");
	}
	Looper::prepare();
	rpiPlayer = new RPiPlayer();
	rpiPlayer->start(String(argv[1]));
	Looper::loop();

	return 0;
}
