#!/bin/bash
set -euo pipefail
brew list --versions bison >/dev/null 2>&1 || brew install bison
brew list --versions llvm >/dev/null 2>&1 || brew install llvm
export PATH="$(brew --prefix bison)/bin:$(brew --prefix llvm)/bin:$PATH"
mkdir -p wine/build-macos
(
    cd wine/build-macos
    ../configure --enable-archs=none --disable-tests --disable-win16 --without-x --without-gnutls
    make -j3 tools/widl/widl
    python3 - <<'PY'
from pathlib import Path
import re
import subprocess
headers = sorted(set(re.findall(r"^(include/[^\s:]+\.h)(?:\s[^:\n]*)?:", Path("Makefile").read_text(), re.M)))
assert headers, "No generated Wine header targets"
subprocess.run(["make", "-j3", *headers], check=True)
PY
)
cat >> wine/build-macos/include/config.h <<'EOF'
#undef SONAME_LIBGNUTLS
#define SONAME_LIBGNUTLS "libgnutls.a"
#undef HAVE_GNUTLS_CIPHER_INIT
#define HAVE_GNUTLS_CIPHER_INIT 1
#undef HAVE_SYS_PTRACE_H
#undef HAVE_SYS_USER_H
#undef HAVE_NETINET_TCP_FSM_H
EOF
if [ ! -d research/freetype/.git ]; then
    git clone --depth 1 --branch VER-2-13-3 https://github.com/freetype/freetype.git research/freetype
fi
bash build/freetype-ios/build.sh
bash build/wineserver/build.sh
bash build/ntdll-unix/build.sh
bash build/win32u-unix/build.sh
for library in gmp nettle hogweed gnutls; do
    cp "toolchains/gnutls-ios/lib/lib$library.a" app/Madeira/
done
for library in wineserver ntdll_unix win32u_unix gmp nettle hogweed gnutls; do
    test -s "app/Madeira/lib$library.a"
    xcrun lipo -verify_arch arm64 "app/Madeira/lib$library.a"
done
