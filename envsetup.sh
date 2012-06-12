#!/bin/bash
RPiDir="`cd ..; pwd`"
export PATH=$RPiDir/tools/arm-bcm2708/linux-x86/bin:$PATH
export CC=$RPiDir/tools/arm-bcm2708/linux-x86/bin/arm-bcm2708-linux-gnueabi-gcc
