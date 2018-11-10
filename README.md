# parrot
Parrot Robots Tools

## Install
on Ubuntu 18.04 according to 
https://developer.parrot.com/docs/SDK3/#how-to-build-the-sdk

download Parrot SDK sources
```
mkdir ~/Desktop/robot/parrot/sdk
cd ~/Desktop/robot/parrot/sdk
sudo apt-get install repo
repo init -m release.xml \
          -u https://github.com/Parrot-Developers/arsdk_manifests.git
repo sync
```
build Parrot SDK and JumpingSumoSample
```
sudo apt-get install -y \
    git build-essential autoconf libtool \
    python python3 libavahi-client-dev libavcodec-dev \
    libavformat-dev libswscale-dev libncurses5-dev mplayer
./build.sh -p arsdk-native -t build-sdk -j
./build.sh -p arsdk-native -t build-sample-JumpingSumoSample -j    
```
run JumpingSumoSample
```
. ~/Desktop/robot/parrot/sdk/out/arsdk-native/staging/native-wrapper.sh
JumpingSumoSample
```

## Configure CLion
Assuming your home directory is /home/sasha

1. Open Run -> Edit Configurations -> sumosh -> Environment variables

1. Set LD_LIBRARY_PATH to /home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/lib:/home/sasha/Desktop/robot/parrot/sdk/out/arsdk-native/staging/usr/lib:
