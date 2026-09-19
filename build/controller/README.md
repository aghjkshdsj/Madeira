# Something PC controller bridge

iOS GameController profiles are sampled on the main run loop at 60 Hz.
Four stable player slots are exposed to Windows XInput DLLs through a
136-byte, versioned memory-mapped file in the Wine prefix. The native
writer uses a sequence counter and memory barriers; readers retry torn
snapshots. Disconnect, backgrounding, and opening Settings release input.

The DLLs are compiled for AMD64 and ARM64 with a checksum-pinned
llvm-mingw release. They replace the prefix's XInput symlinks on launch,
not game files. Games with their own adjacent XInput DLL may continue
using that DLL. DirectInput/HID enumeration, rumble, controller audio,
and analog-trigger/stick keystroke events are not implemented.
XInputGetState provides complete analog values.

Keyboard fallback maps player 1 to WASD, mouse movement, and the bindings
listed in Settings. Controller Test shows raw iOS input with forwarding
suspended; it does not claim to test a game's own bindings.

The Windows CI probe checks the shared-file reader, player indices,
buttons/axes/triggers, enable/disable, capabilities, battery, disconnect,
and button keystroke behavior. Backbone hardware and individual games
still require on-device testing.

Desktop resolution applies at the next Wine session launch. Games may
override it; this is not a forced internal rendering scale.

Microsoft's unmodified x64 and ARM64 installers are supplied for manual
use inside Wine. Installer compatibility is not guaranteed; merely
including an installer does not prove a game's prerequisite check is
fixed. Existing working C++ runtime overrides are deliberately retained.
