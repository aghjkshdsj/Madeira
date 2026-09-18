# Unsigned IPA release

The Build Madeira IPA workflow builds FEX, Wine's native iOS libraries,
FreeType, the GnuTLS stack, LLVM 15.0.7, and DXMT. The checked-in Wine PE
modules are retained, and Microsoft's x64 VC runtime is independently
downloaded and verified on Windows.

Run the workflow manually or push changes to main. Native components run
independently, with successful outputs cached against their sources and
build scripts. Failure logs are uploaded as Actions artifacts.

Only after all native components succeed does Xcode build the unsigned
device app. Packaging verifies the arm64/iOS executable, property list,
required bundled modules, and ZIP integrity. The release includes
Madeira.ipa and its SHA-256 checksum. A final step downloads the release
asset again and verifies its checksum.

This is an unsigned distribution archive, not an App Store build.
Installation requires appropriate signing and the device/JIT setup
documented in the project README. CI does not test execution on a device.
