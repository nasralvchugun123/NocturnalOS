#!/usr/bin/env bash
set -euo pipefail

# Default paths
BUILD_DIR="build"
ISO="build/NocturnalOS.iso"
DISK="build/NocturnalOS.disk.img"
FIRMWARE_VARS_TEMPLATE="/home/asus/OVMF.4m.fd"
FIRMWARE_VARS="/tmp/NocturnalOS_VARS.fd"

# Default parameters (Preset: default)
RAM="512M"
CPUS="8"
MACHINE="q35"
DISPLAY_MODE="gtk"
ENABLE_GDB=0
ENABLE_NET=0
ENABLE_LOG=0

# Check required files
check_files() {
    [[ -f "$ISO" ]] || { echo "❌ ISO not found: $ISO (run ./build.sh first)"; exit 1; }
    [[ -f "$DISK" ]] || { echo "❌ Disk image not found: $DISK"; exit 1; }
    [[ -f "$FIRMWARE_VARS_TEMPLATE" ]] || { echo "❌ OVMF_VARS template not found: $FIRMWARE_VARS_TEMPLATE"; exit 1; }
    [[ -f "$FIRMWARE_VARS" ]] || cp "$FIRMWARE_VARS_TEMPLATE" "$FIRMWARE_VARS"
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --ram) RAM="$2"; shift 2 ;;
            --cpus) CPUS="$2"; shift 2 ;;
            --display) DISPLAY_MODE="$2"; shift 2 ;;
            --gdb) ENABLE_GDB=1; shift ;;
            --net) ENABLE_NET=1; shift ;;
            --log) ENABLE_LOG=1; shift ;;
            --preset)
                case "$2" in
                    default) RAM="512M"; CPUS="8" ;;
                    perf)    RAM="2G"; CPUS="4" ;;
                    debug)   RAM="1G"; CPUS="2"; ENABLE_GDB=1; ENABLE_LOG=1 ;;
                    *) echo "❌ Unknown preset: $2"; exit 1 ;;
                esac
                shift 2
                ;;
            -h|--help)
                echo "Usage: $0 [options]"
                echo "  --ram <SIZE>       RAM size (e.g., 1G, 512M)"
                echo "  --cpus <COUNT>     Number of CPU cores"
                echo "  --display <TYPE>   Display type (gtk, sdl, vnc, none)"
                echo "  --gdb              Enable GDB debugger (port 1234)"
                echo "  --net              Connect network adapter (e1000)"
                echo "  --log              Enable QEMU logging to a file"
                echo "  --preset <NAME>    Preset name (default, perf, debug)"
                exit 0
                ;;
            *) echo "❌ Unknown flag: $1"; exit 1 ;;
        esac
    done
}

# Interactive menu if no arguments are provided
interactive_menu() {
    echo "=========================================="
    echo "      NocturnalOS QEMU Launcher           "
    echo "=========================================="
    echo "1) 🚀 Default Run (512M RAM, 8 CPUs)"
    echo "2) ⚡ Performance Mode (2G RAM, 4 CPUs)"
    echo "3) 🐞 Debug Mode (1G RAM, GDB port 1234, Logs)"
    echo "4) 🛠️ Custom Settings (manual input)"
    echo "=========================================="
    read -rp "Select menu option [1-4]: " choice

    case "$choice" in
        1) ;; # defaults
        2) RAM="2G"; CPUS="4" ;;
        3) RAM="1G"; CPUS="2"; ENABLE_GDB=1; ENABLE_LOG=1 ;;
        4)
            read -rp "Enter RAM size [default 512M]: " input_ram
            [[ -n "$input_ram" ]] && RAM="$input_ram"
            read -rp "Enter CPU count [default 8]: " input_cpus
            [[ -n "$input_cpus" ]] && CPUS="$input_cpus"
            read -rp "Enable GDB debugging? (y/n): " input_gdb
            [[ "$input_gdb" == "y" ]] && ENABLE_GDB=1
            read -rp "Enable network? (y/n): " input_net
            [[ "$input_net" == "y" ]] && ENABLE_NET=1
            ;;
        *) echo "Invalid choice, exiting."; exit 1 ;;
    esac
}

# If arguments are passed, parse them. Otherwise, show the menu.
if [ $# -gt 0 ]; then
    parse_args "$@"
else
    interactive_menu
fi

check_files

# Determine hardware acceleration (KVM / TCG)
ACCEL="tcg"
if [[ -w /dev/kvm ]]; then
    ACCEL="kvm"
    echo "✅ KVM is available — using hardware acceleration"
else
    echo "⚠️ KVM is unavailable — running via TCG (slow)"
fi

# Build QEMU arguments array
QEMU_ARGS=(
    -machine "${MACHINE},accel=${ACCEL}"
    -cpu qemu64
    -m "${RAM}"
    -smp "${CPUS}"
    -drive "if=pflash,format=raw,file=${FIRMWARE_VARS}"
    -drive "file=${DISK},if=none,id=nvme0,format=raw,cache=writeback"
    -device "nvme,drive=nvme0,serial=nocturnal01"
    -drive "file=${ISO},if=none,id=cd0,media=cdrom,readonly=on"
    -device "ide-cd,drive=cd0,bus=ide.1"
    -boot "order=d"
    -vga std
    -display "${DISPLAY_MODE}"
    -serial "mon:stdio"
)

# Optional parameters
if [ "$ENABLE_GDB" -eq 1 ]; then
    echo "🐞 GDB debugging enabled. Waiting for connection on port 1234..."
    QEMU_ARGS+=(-s -S)
fi

if [ "$ENABLE_NET" -eq 1 ]; then
    echo "🌐 Network connected (e1000)"
    QEMU_ARGS+=(-netdev "user,id=net0" -device "e1000,netdev=net0")
fi

if [ "$ENABLE_LOG" -eq 1 ]; then
    echo "📝 QEMU logging enabled to qemu_debug.log"
    QEMU_ARGS+=(-D qemu_debug.log -d int,cpu_reset)
fi

# Launch QEMU
exec qemu-system-x86_64 "${QEMU_ARGS[@]}"