#!/bin/bash
set -e

echo "── Kgp Nav Backend ──────────────────────"

# ── CHECK TOOLCHAIN ──────────────────────────────────────────────
CMAKE="/c/msys64/ucrt64/bin/cmake"
CXX="/c/msys64/ucrt64/bin/g++.exe"
MAKE="/c/msys64/ucrt64/bin/mingw32-make.exe"

if ! command -v "$CMAKE" &>/dev/null; then echo "[build] cmake not found at $CMAKE"; exit 1; fi
if ! command -v "$CXX" &>/dev/null; then   echo "[build] g++ not found at $CXX";   exit 1; fi
if ! command -v "$MAKE" &>/dev/null; then   echo "[build] mingw32-make not found";  exit 1; fi

# ── DOWNLOAD SINGLE-HEADER DEPS ──────────────────────────────────
echo "[deps] Checking dependencies..."

mkdir -p include/nlohmann

if [ ! -f include/httplib.h ]; then
  echo "[deps] Downloading cpp-httplib..."
  if curl -sL --fail https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h \
    -o include/httplib.h; then
    echo "[deps] httplib.h done"
  else
    echo "[deps] FAILED to download httplib.h — check network"
    exit 1
  fi
fi

if [ ! -f include/nlohmann/json.hpp ]; then
  echo "[deps] Downloading nlohmann/json..."
  if curl -sL --fail https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
    -o include/nlohmann/json.hpp; then
    echo "[deps] json.hpp done"
  else
    echo "[deps] FAILED to download json.hpp — check network"
    exit 1
  fi
fi

# ── BUILD ─────────────────────────────────────────────────────────
echo "[build] Configuring..."
if ! "$CMAKE" -S . -B build \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_MAKE_PROGRAM="$MAKE" \
  -Wno-dev; then
  echo "[build] CMake configuration failed"
  exit 1
fi

echo "[build] Compiling..."
if ! "$CMAKE" --build build --parallel; then
  echo "[build] Compilation failed"
  exit 1
fi

BINARY="build/kgp_nav.exe"
if [ ! -f "$BINARY" ]; then echo "[build] Binary not found: $BINARY"; exit 1; fi
echo "[build] Done → $BINARY ($(stat -c%s "$BINARY" 2>/dev/null || echo "?") bytes)"

# ── RUN ───────────────────────────────────────────────────────────
PORT=${1:-8080}
echo "[run] Starting server on port $PORT..."
echo "[run] Data dir: server/data (default)"
echo ""
"$BINARY" "$PORT"