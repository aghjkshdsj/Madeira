#!/bin/bash
set -euo pipefail
xcodebuild -project app/Madeira.xcodeproj -scheme Madeira \
    -configuration Release -sdk iphoneos -destination 'generic/platform=iOS' \
    -derivedDataPath build/DerivedData \
    CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" \
    build 2>&1 | tee build/xcodebuild.log
app=build/DerivedData/Build/Products/Release-iphoneos/Madeira.app
test -d "$app"
plutil -lint "$app/Info.plist"
executable=$(/usr/libexec/PlistBuddy -c 'Print CFBundleExecutable' "$app/Info.plist")
test -s "$app/$executable"
xcrun lipo -verify_arch arm64 "$app/$executable"
xcrun vtool -show-build "$app/$executable" | grep -q 'platform IOS'
if codesign --verify "$app" 2>/dev/null; then
    echo "Expected an unsigned app, but a signature was found" >&2
    exit 1
fi
mkdir -p build/IPA/Payload
ditto "$app" build/IPA/Payload/Madeira.app
(
    cd build/IPA
    zip -qry Madeira.ipa Payload
    unzip -t Madeira.ipa
    shasum -a 256 Madeira.ipa > Madeira.ipa.sha256
)
python3 - <<'PY'
from pathlib import Path
import plistlib
import zipfile
path = Path("build/IPA/Madeira.ipa")
with zipfile.ZipFile(path) as archive:
    assert archive.testzip() is None
    info = plistlib.loads(archive.read("Payload/Madeira.app/Info.plist"))
    executable = "Payload/Madeira.app/" + info["CFBundleExecutable"]
    assert archive.getinfo(executable).file_size > 1_000_000
    names = archive.namelist()
    for directory in ("aarch64-windows", "arm64ec-windows", "x86_64-vcruntime"):
        assert any("/" + directory + "/" in name for name in names), directory
print(f"Verified unsigned IPA: {path.stat().st_size:,} bytes")
PY
