# Build

## Build on macOS

#### Setup

1. Install Xcode Command Line Tools (recommended: install Homebrew)
2. Install `cmake` (recommended: `brew install cmake`)
3. Download the most recent [FLTK](https://www.fltk.org) release (1.4.x branch)


### Build FLTK

 Change to the top directory of the FLTK source (the one containing `configure`) and build FLTK using the command line:

    MIN_MAC_VERSION=13.0
    NUM_CORES=$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || sysctl -n hw.physicalcpu)
    ./configure CFLAGS="-mmacosx-version-min=$MIN_MAC_VERSION" CXXFLAGS="-mmacosx-version-min=$MIN_MAC_VERSION" LDFLAGS="-mmacosx-version-min=$MIN_MAC_VERSION" --enable-localzlib
    make -j$NUM_CORES

Be sure to use the same `MIN_MAC_VERSION` when building FLTK as you use while building Tablecruncher itself.


### Build Tablecruncher

Adapt the paths to your FLTK builds in `CMakeLists.txt` and build the application on the command line:

    mkdir -p build && cd build
    NUM_CORES=$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || sysctl -n hw.physicalcpu)
    rm -rf *
    cmake -DFLTKDIR="/Users/sf/Documents/Builds/FLTK/fltk-1.4.3" -DMACOS_VERSION=13.0 ..
    cmake --build . -- -j$NUM_CORES


### Only call create bundle script to build an Apple app bundle

    cmake --build . --target create_bundle -- -j$(sysctl -n hw.perflevel0.physicalcpu)


### Cleaning

    cmake --build . --target clean
    cmake --build . --target custom_clean



## Build on Windows

### Setup

1. Install `VS Studio C++` using the `Visual Studio Installer` app.
2. Make sure `cmake` is installed. (Should be installed with VS Studio)
3. Download the most recent [FLTK](https://www.fltk.org) release (1.4.x branch)


### Build FLTK (using nmake)

Run the following commands in a "Developer Command Prompt":

    cd C:\path\to\your\fltk\dir\fltk-1.4.x
    mkdir build-nmake
    cd build-nmake
    cmake -G "NMake Makefiles" -D CMAKE_BUILD_TYPE=Release -D FLTK_MSVC_RUNTIME_DLL=off ..
    nmake


### Build Tablecruncher

Before building, edit `CMakePresets.json` and include the FLTK paths for your build. (Or pass `-DFLTKINCDIR` and `-DFLTKLIBDIR` to CMake.)

Use `VS Studio C++` to build the application. Or use `cmake` in a "Developer Command Prompt":

    mkdir build
    cd build
    cmake ..
    cmake --build .


## Build on Linux

### Fedora
    sudo dnf install gcc-c++ cmake make git
    sudo dnf install libX11-devel libXft-devel

### Ubuntu/Debian
    
    sudo apt install g++ cmake make
    sudo apt install libx11-dev libxft-dev

### Build FLTK

    ./configure --enable-localzlib --disable-wayland
    make -j$(nproc)

### Build Tablecruncher

    mkdir -p build && cd build
    rm -rf *
    cmake -DFLTKDIR="/home/sf/Documents/Builds/fltk-1.4.3" ..
    cmake --build . -- -j$(nproc)


## Build AppImage package

    cd build/
    rm -rf *
    cmake -DFLTKDIR="/home/sf/Documents/Builds/fltk-1.4.3" -DAPPIMAGE=1 ..
    cmake --build . -- -j$(nproc)
    cd ../scripts/
    ./build_appimage.sh


