## The AndroidTransporterPlayer is a media player for the Raspberry Pi

### Setup
- cd /opt/vc/src/hello_pi/libs/ilclient
- make
- cd /home/pi
- sudo apt-get install git-core
- git clone https://github.com/esrlabs/AndroidTransporterPlayer.git
- cd AndroidTransporterPlayer
- git clone https://github.com/esrlabs/Mindroid.git
- make

###Usage
&lt;IP-Address&gt; is always the IP address of the VLC streaming server.

#### Raspberry Pi media player
/home/pi/AndroidTransporterPlayer/AndroidTransporterPlayer rtsp://&lt;IP-Address&gt;:9000/test.sdp

#### VLC streaming server
vlc &lt;video&gt;.mp4 --sout '#rtp{sdp=rtsp://&lt;IP-Address&gt;:9000/test.sdp}' --rtsp-timeout=-1



