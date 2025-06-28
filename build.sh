#!/bin/bash

# Build script for ua project
# Supports both autotools and CMake build systems

set -e

show_help() {
    cat << EOF
Build script for ua project

Usage: $0 [OPTIONS] [BUILD_SYSTEM]

OPTIONS:
    -h, --help      Show this help message
    -c, --clean     Clean build artifacts before building
    -j, --jobs N    Use N parallel jobs (default: auto-detect)

BUILD_SYSTEM:
    autotools       Use autotools (./configure && make) [default]
    cmake           Use CMake (mkdir build && cmake && make)

Examples:
    $0                    # Build with autotools
    $0 cmake              # Build with CMake
    $0 -c autotools       # Clean and build with autotools
    $0 -j4 cmake          # Build with CMake using 4 jobs

EOF
}

# Default values
BUILD_SYSTEM="autotools"
CLEAN=false
JOBS=$(nproc 2>/dev/null || echo 1)

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        autotools|cmake)
            BUILD_SYSTEM="$1"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

echo "Building ua project with $BUILD_SYSTEM build system"
echo "Jobs: $JOBS"
echo "Clean: $CLEAN"
echo

case $BUILD_SYSTEM in
    autotools)
        echo "=== Using Autotools Build System ==="
        
        if [ "$CLEAN" = true ]; then
            echo "Cleaning autotools build artifacts..."
            make dist-clean 2>/dev/null || true
        fi
        
        if [ ! -f "configure" ]; then
            echo "Running autogen.sh..."
            bash ./autogen.sh
        fi
        
        if [ ! -f "Makefile" ]; then
            echo "Running configure..."
            bash ./configure
        fi
        
        echo "Building with make..."
        make -j"$JOBS"
        
        echo "Build complete! Binaries are in build/ directory"
        ;;
        
    cmake)
        echo "=== Using CMake Build System ==="
        
        if [ "$CLEAN" = true ]; then
            echo "Cleaning CMake build artifacts..."
            rm -rf build
        fi
        
        echo "Creating build directory..."
        mkdir -p build
        cd build
        
        echo "Running cmake..."
        cmake ..
        
        echo "Building with make..."
        make -j"$JOBS"
        
        cd ..
        echo "Build complete! Binaries are in build/ directory"
        ;;
        
    *)
        echo "Unknown build system: $BUILD_SYSTEM"
        exit 1
        ;;
esac

echo
echo "Available binaries:"
if [ -f "build/ua" ]; then
    echo "  build/ua"
fi
if [ -f "build/kua" ]; then
    echo "  build/kua"
fi
if [ -f "ua" ]; then
    echo "  ua (autotools build)"
fi
if [ -f "kua" ]; then
    echo "  kua (autotools build)"
fi 