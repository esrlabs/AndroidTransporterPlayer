## The AndroidTransporterPlayer is a media player for the Raspberry Pi

### Demo video
[![Android Transporter Player demo](http://img.youtube.com/vi/PsLb-nDXUyQ/0.jpg)](http://www.youtube.com/watch?v=PsLb-nDXUyQ)

Take a look at the [Android Transporter blog post](http://esrlabs.com/android-transporter-for-the-nexus-7-and-the-raspberry-pi/) for more information.

### Setup
    sudo apt-get install git-core
    cd /opt/vc/src/hello_pi/libs/ilclient
    make
    cd /home/pi
    mkdir Projects
    cd Projects
    git clone https://github.com/esrlabs/AndroidTransporterPlayer.git
    git clone -b android-transporter https://github.com/esrlabs/Mindroid.cpp.git Mindroid
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
    ./AndroidTransporterPlayer rtsp://<IP-Address>:9000/test.sdp

### Usage
&lt;IP-Address&gt; is always the IP address of the VLC streaming server.

#### Raspberry Pi media player
/home/pi/AndroidTransporterPlayer/AndroidTransporterPlayer rtsp://&lt;IP-Address&gt;:9000/test.sdp

#### VLC streaming server
vlc &lt;video&gt;.mp4 --sout '#rtp{sdp=rtsp://&lt;IP-Address&gt;:9000/test.sdp}' --rtsp-timeout=-1

