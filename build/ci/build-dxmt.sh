#!/bin/bash
set -euo pipefail
xcodebuild -downloadComponent MetalToolchain
mkdir -p build/dxmt-ios/shader-headers
for shader in air_msad air_samplepos air_tessellation; do
    xcrun -sdk macosx metal -std=metal3.1 --target=air64-apple-macos14.0 \
        -c "research/dxmt/src/airconv/shaders/$shader.metal" \
        -o "build/dxmt-ios/shader-headers/$shader.air"
    xxd -n "$shader" -i "build/dxmt-ios/shader-headers/$shader.air" \
        "build/dxmt-ios/shader-headers/$shader.h"
done
bash build/dxmt-ios/build.sh
xcrun --sdk iphoneos libtool -static -o app/Madeira/libdxmt_combined.a \
    build/dxmt-ios/obj/*.o toolchains/llvm-ios-build/lib/*.a
xcrun lipo -verify_arch arm64 app/Madeira/libdxmt_combined.a
