Dependencies
```
sudo apt-get install build-essential cmake git \
                     libgtk2.0-dev pkg-config \
                     libavcodec-dev libavformat-dev libswscale-dev
```

Build and Install OpenCV
```
mkdir /home/sasha/Desktop/cv
cd /home/sasha/Desktop/cv
git clone https://github.com/opencv/opencv.git
cd opencv
git checkout 4.2.0
cd ..
git clone https://github.com/opencv/opencv_contrib.git
cd opencv_contrib
git checkout 4.2.0
cd ../opencv
mkdir build-debug
cd build-debug
cmake -D CMAKE_BUILD_TYPE=Debug \
      -D CMAKE_INSTALL_PREFIX=/home/sasha/Desktop/cv/opencv4.2.0-debug \
      -D OPENCV_EXTRA_MODULES_PATH=/home/sasha/Desktop/cv/opencv_contrib/modules ..
make -j 5
make install
```

Build sumocv
```
git clone https://github.com/esmanchik/parrot.git /home/sasha/Desktop/robot/parrot/esmanchik
cd /home/sasha/Desktop/robot/parrot/esmanchik/sumocv
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
make
```