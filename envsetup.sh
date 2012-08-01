#!/bin/bash
RPiDir="`cd ..; pwd`"
export PATH=$RPiDir/tools/arm-bcm2708/x86-linux64-cross-arm-linux-hardfp/bin:$PATH
export CC=$RPiDir/tools/arm-bcm2708/x86-linux64-cross-arm-linux-hardfp/bin/arm-bcm2708hardfp-linux-gnueabi-gcc
