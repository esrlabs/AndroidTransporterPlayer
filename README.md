## The AndroidTransporterPlayer is a media player for the Raspberry Pi

### Setup
    sudo apt-get install git-core
    cd /opt/vc/src/hello_pi/libs/ilclient
    make
    cd /home/pi
    mkdir Projects
    cd Projects
    git clone https://github.com/esrlabs/AndroidTransporterPlayer.git
    git clone https://github.com/esrlabs/Mindroid.cpp.git Mindroid
    git clone https://github.com/esrlabs/fdk-aac.git
    cd Mindroid
    make -f Makefile.RPi
    sudo cp libmindroid.so /usr/lib/
    cd ..
    cd fdk-aac
    make -f Makefile.RPi
    sudo cp libaac.so /usr/lib/
    cd ..
    cd AndroidTransporterPlayer
    make -f Makefile.RPi
    ./AndroidTransporterPlayer rtsp://&lt;IP-Address&gt;:9000/test.sdp

###Usage
&lt;IP-Address&gt; is always the IP address of the VLC streaming server.

#### Raspberry Pi media player
/home/pi/AndroidTransporterPlayer/AndroidTransporterPlayer rtsp://&lt;IP-Address&gt;:9000/test.sdp

#### VLC streaming server
vlc &lt;video&gt;.mp4 --sout '#rtp{sdp=rtsp://&lt;IP-Address&gt;:9000/test.sdp}' --rtsp-timeout=-1

