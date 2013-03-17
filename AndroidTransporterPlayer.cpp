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
		printf("Usage: AndroidTransporterPlayer rtsp://<Host>:<Port>/<SDP-File>\n");
		return -1;
	}

	signal(SIGINT, shutdownHook);

	if (setpriority(PRIO_PROCESS, 0, -17) != 0) {
		printf("Cannot set thread priorities. Please restart with more privileges.\n");
	}
	Looper::prepare();
	rpiPlayer = new RPiPlayer();
	rpiPlayer->start(new String(argv[1]));
	Looper::loop();

	return 0;
}
