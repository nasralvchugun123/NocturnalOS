#!/usr/bin/env bash
set -euo pipefail

echo "=========================================="
echo "       NocturnalOS Builder script         "
echo "=========================================="

# Check and install dependencies
install_dependencies() {
    if [ -f /etc/arch-release ]; then
        echo "📦 Arch Linux detected. Installing dependencies..."
        sudo pacman -Syu --needed base-devel cmake ninja clang lld xorriso mtools qemu-full

    elif [ -f /etc/void-release ]; then
        echo "📦 Void Linux detected. Installing dependencies..."
        sudo xbps-install -Syu cmake ninja clang lld xorriso mtools qemu

    elif [ -f /etc/debian_version ]; then
        echo "📦 Debian/Ubuntu detected. Installing dependencies..."
        sudo apt update && sudo apt install -y \
            cmake ninja-build clang lld xorriso mtools qemu-system-x86 qemu-utils

    elif [ -f /etc/fedora-release ]; then
        echo "📦 Fedora detected. Installing dependencies..."
        sudo dnf install -y cmake ninja-build clang lld xorriso mtools qemu-system-x86
    else
        echo "⚠️ Unknown distribution. Make sure you have installed: cmake, ninja, clang, lld, xorriso, mtools, qemu."
    fi
}

if [[ "${1:-}" == "--deps" ]] || ! command -v cmake &>/dev/null || ! command -v ninja &>/dev/null; then
    install_dependencies
fi

export CC=clang
export CXX=clang++

BUILD_TYPE="Debug"
if [[ "${1:-}" == "--release" ]]; then
    BUILD_TYPE="Release"
    shift
fi

echo "⚙️ Configuring CMake (${BUILD_TYPE})..."
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_LINKER=lld

echo "🔨 Building OS (ISO and disk images)..."
cmake --build build --target iso

echo "✅ Build completed successfully!"