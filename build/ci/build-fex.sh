#!/bin/bash
set -euo pipefail
set -euo pipefail
python3 - <<'PY'
from pathlib import Path

core = Path("FEX/FEXCore/Source/Interface/Core/Core.cpp")
s = core.read_text()
reporter_start = "  /* iOS-Madeira ml316: report ExitToX64 FFS bypasses"
reporter_end = "  /* iOS-Madeira: refuse to compile obviously-invalid guest RIPs."
start = s.find(reporter_start)
if start == -1:
    raise SystemExit("FEX iOS diagnostic reporter start marker not found")
end = s.find(reporter_end, start)
if end == -1:
    raise SystemExit("FEX iOS diagnostic reporter end marker not found")
s = s[:start] + s[end:]
if "const uint64_t FfsCount = IosFfsBypassLog[0]" in s or "const uint64_t CBCount = IosCbEntryLog[6]" in s:
    raise SystemExit("FEX iOS diagnostic reporters remain after cleanup")
core.write_text(s)

arm = Path("FEX/FEXCore/Source/Utils/ArchHelpers/Arm64.cpp")
s = arm.read_text()
start = s.find("  MEMORY_BASIC_INFORMATION mbi {};")
if start != -1:
    end_marker = "mbi.Protect, type, mbi.State);"
    end = s.find(end_marker, start)
    if end == -1:
        raise SystemExit("Arm64 CASPAL diagnostic end marker not found")
    new = """  LogMan::Msg::EFmt("[caspal128] MISALIGNED-UNSUPPORTED Size={} addrReg=x{} addr={:#x} misalign={} ",
                  Size, AddressReg, GPRs[AddressReg], GPRs[AddressReg] & 15);"""
    arm.write_text(s[:start] + new + s[end + len(end_marker):])
PY
set -euo pipefail
cmake -S FEX -B FEX/build-ios -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_SYSTEM_PROCESSOR=arm64 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
  -DCMAKE_OSX_SYSROOT="$(xcrun --sdk iphoneos --show-sdk-path)" \
  -DCMAKE_BUILD_TYPE=Release \
  -DTUNE_CPU=generic \
  -DBUILD_TESTING=OFF \
  -DBUILD_FEX_LINUX_TESTS=OFF \
  -DBUILD_THUNKS=OFF \
  -DBUILD_FEXCONFIG=OFF \
  -DBUILD_STEAM_SUPPORT=OFF \
  -DENABLE_ZYDIS=OFF \
  -DENABLE_LTO=OFF \
  -DENABLE_CCACHE=OFF \
  -DENABLE_FEX_ALLOCATOR=OFF \
  -DENABLE_JEMALLOC_GLIBC_ALLOC=OFF
cmake --build FEX/build-ios --target FEXCore FEXCore_Base JemallocLibs --parallel 3
test -s FEX/build-ios/FEXCore/Source/libFEXCore.a
test -s FEX/build-ios/FEXCore/Source/libFEXCore_Base.a
test -s FEX/build-ios/FEXCore/Source/libJemallocLibs.a
test -s FEX/build-ios/include/FEXCore/Config/ConfigValues.inl
