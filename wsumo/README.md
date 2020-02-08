Build Instructions
```
git submodule init
cd libwebsockets
mkdir cmake-build-debug
cd cmake-build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
cd ..
mkdir cmake-build-debug
cd cmake-build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

```

To add libwebsockets to project:
```
git submodule add https://github.com/warmcat/libwebsockets.git
```
