#!/bin/bash
set -e

echo "── Campus Nav Backend ──────────────────────"

# ── DOWNLOAD SINGLE-HEADER DEPS ──────────────────────────────────
echo "[deps] Checking dependencies..."

mkdir -p include/nlohmann

if [ ! -f include/httplib.h ]; then
  echo "[deps] Downloading cpp-httplib..."
  curl -sL https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h \
    -o include/httplib.h
  echo "[deps] httplib.h done"
fi

if [ ! -f include/nlohmann/json.hpp ]; then
  echo "[deps] Downloading nlohmann/json..."
  curl -sL https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
    -o include/nlohmann/json.hpp
  echo "[deps] json.hpp done"
fi

# ── BUILD ─────────────────────────────────────────────────────────
echo "[build] Configuring..."
/c/msys64/ucrt64/bin/cmake -S . -B build \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/c/msys64/ucrt64/bin/g++.exe \
  -DCMAKE_MAKE_PROGRAM=/c/msys64/ucrt64/bin/mingw32-make.exe \
  -Wno-dev

echo "[build] Compiling..."
/c/msys64/ucrt64/bin/cmake --build build --parallel

echo "[build] Done → build/campus_nav.exe"

# ── RUN ───────────────────────────────────────────────────────────
PORT=${1:-8080}
echo "[run] Starting server on port $PORT..."
echo "[run] Data dir: ./data"
echo ""
./build/campus_nav.exe "$PORT" "data"