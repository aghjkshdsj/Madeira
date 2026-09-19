# Something PC 0.3

The library scans `Documents/wine/drive_c/Games` for regular `.exe` files, case-insensitively. Candidates must have MZ/PE headers, an executable-image flag, and a PE32/PE32+ optional-header signature, and must not have the DLL flag. Installer/crash helper names and symbolic links are excluded. The same application validation runs on EXE import and game launch. Cards show the actual executable filename as well as the display title. `.bin`, `.dll`, `.dat`, `.pak`, and other supporting files never become game cards, even if their contents resemble a PE image. Import Game Folder still copies the complete folder because games need those supporting files. Custom EXE copies only the executable; adjacent game data must be supplied separately. Games are never downloaded or bundled.

Artwork is resolved from `cover.png`, `cover.jpg`, `folder.jpg`, `header.jpg`, or an EXE-named image next to the executable. A numeric `steam_appid.txt` identifies official Steam artwork without guessing game names. Optional `somethingpc-game.json` accepts string fields `title`, `publisher`, `cover` (local relative image path), and `steamAppID`. Long-press a card to choose artwork and save per-game settings. Generic art is used if no verified identity/cover exists.

The PC tile opens the Wine desktop. One session per app process is supported. Restart to switch games or runtime profiles; the library offers Resume session. Existing signing/JIT requirements remain. JIT requests time out after 90 seconds.

## Runtime

An ARM64 Windows CI runner installs Microsoft's signed redistributable, verifies the installed DLL signatures/PE architectures, and records hashes plus actual installed version fields. The iOS app verifies these again before activating DLL links and the ARM64 registry key. The previous key is journaled and restored at next startup without replacing unrelated registry data. The Wine process recreates its default DLL links on every fresh session before the optional ARM64 overlay. ARM64 files go to the ARM64 farm (and system32 only in native ARM64 sessions); x64/ARM64EC exception handlers remain intact. No installer executes on iOS. This is experimental compatibility support, not a claim that Mecha Chameleon or Scarlet Skips is tested or fixed.

## Unexpected termination

A durable foreground-session marker is checked before LogStore rotates. The last 4 MiB of the previous log is archived with session/device metadata; five reports are retained. Close acknowledges the prompt without deleting the report; Share opens the system share sheet and does not upload automatically. A foreground force-quit or OS termination may also trigger the prompt. Background kills may not. This is not a crash stack trace and does not intercept Wine/FEX signal handlers. Reports may include private paths and game output.

## Validation

`swiftc app/Madeira/GameLibraryCore.swift build/app-tests/main.swift -o /tmp/app-tests && /tmp/app-tests`

CI tests PE validation, path containment, symlink rejection, discovery filtering, metadata, folder/EXE imports, profile round trips, and registry activation/restoration. Release CI builds the complete iOS app and downloads its published IPA to compare bytes and checksum. Real-device testing is still required for layout, controller behavior, JIT launch, and game/runtime compatibility. Back up app documents before installing.
