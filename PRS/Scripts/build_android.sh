#!/usr/bin/env bash
set -e
# Android APK build helper (requires Android NDK + cmake)
cmake -S . -B build-android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24
cmake --build build-android --parallel
echo "Build done"