#!/bin/bash
set -euo pipefail
archive=llvm-mingw-20260421-ucrt-macos-universal.tar.xz
mkdir -p toolchains build/controller/out
if [ ! -d "toolchains/${archive%.tar.xz}" ]; then
    curl --fail --location --retry 3 "https://github.com/mstorsjo/llvm-mingw/releases/download/20260421/$archive" -o "toolchains/$archive"
    echo "bd85a3975723815cef28dbbd2ca2cb0c926f6b348a12a0453f39f7af273cb3f7  toolchains/$archive" | shasum -a 256 -c -
    tar -xf "toolchains/$archive" -C toolchains
fi
toolchain="$PWD/toolchains/${archive%.tar.xz}/bin"
"$toolchain/x86_64-w64-mingw32-clang" -std=c11 -O2 -static -Iapp/Madeira \
    build/controller/xinput-test.c -o build/controller/out/controller-test.exe
for arch in x86_64 aarch64; do
    mkdir -p "app/Madeira/ControllerSupport/$arch"
    "$toolchain/$arch-w64-mingw32-clang" -std=c11 -O2 -shared -static-libgcc \
        -Iapp/Madeira build/controller/xinput.c build/controller/xinput.def \
        -o "build/controller/out/xinput-$arch.dll" -Wl,--no-insert-timestamp
    "$toolchain/llvm-readobj" --coff-exports "build/controller/out/xinput-$arch.dll"
    for name in xinput1_1 xinput1_2 xinput1_3 xinput1_4 xinput9_1_0; do
        cp "build/controller/out/xinput-$arch.dll" "app/Madeira/ControllerSupport/$arch/$name.dll"
    done
done
