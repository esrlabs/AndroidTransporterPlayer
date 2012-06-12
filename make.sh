. ./envsetup.sh
cd ..
cd rootfs/lib
ln -s libgcc_s.so.1 libgcc_s.so
cd ..
cd usr/lib
ln -s libstdc++.so.6.0.13 libstdc++.so
cd ..
cd ..
cd ..
cd AndroidTransporterPlayer
make
