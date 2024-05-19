#!/bin/bash

# Get latest git submodules
git submodule init
git submodule update

# Change to tsc directory
cd tsc

# Delete old build files
rm -rf build
mkdir build
cd build

# Delete old game directory at that is at repos/Secretchronicles/TSC/tsc/tsc
rm -rf ../tsc

# Build TSC
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../tsc ..
make -j$(nproc)

# Install TSC to $HOME/tsc
make install

# Changing back to TSC repo directory
cd ../..

echo "You can now run TSC with ./run-tsc.sh"
