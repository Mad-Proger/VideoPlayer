# VideoPlayer
Multi-thread fullscreen videoplayer using SDL2 and FFmpeg

Current version creates fullscreen window to play video in, but it doesn't handle any window events. As a consequence, it's a bit difficult to close the window untill the video ends.

## Build

### Dependencies
* FFmpeg ([GitHub](https://github.com/FFmpeg/FFmpeg))
* SDL ([GitHub](https://github.com/libsdl-org/sdl))

### Prequisites
* GNU make
* gcc compiler
* pkg-config

### How to build
1. Install FFmpeg and SDL (build from souces)
2. Configure `PKG_CONFIG_PATH` variable so that `pkg-config` can find your FFmpeg and SDL installations
3. Run `make`
    * If FFmpeg and SDL header files are not located in /usr/local/include set `INCLUDE_PATH` to that specific location (i.e. `make -e "INCLUDE_PATH=/path/to/your/libraries/installation"`)
    * If you need to use cross-compiler (for example, x86_64-w64-mingw32-gcc), you can define `CROSS_PREFIX` make variable to use gcc with that prefix
