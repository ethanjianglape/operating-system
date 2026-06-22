#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_section() {
    echo -e "${YELLOW}$1${NC}"
}

log_failure() {
    echo -e "${RED} FAILURE!$1${NC}"
}

log_success() {
    echo -e "${GREEN}$1${NC}"
}

log() {
    echo -e "$1${NC}"
}

log_section "Cleaning MyOS build and output files"

log "Deleting cmake_build/"
rm -rf cmake_build/
rm -rf cmake-build-debug/

log "Deleting initramfs/bin"
rm -rf initramfs/bin

log "Deleting sysroot/"
rm -rf sysroot/

log "Deleting myos.iso"
rm -f myos.iso

log_success "Done!"
