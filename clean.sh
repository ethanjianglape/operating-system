#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_section() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "${YELLOW}[$CURRENT_TIME] $1${NC}"
}

log_failure() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "${RED}[$CURRENT_TIME] FAILURE! $1${NC}"
}

log_success() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "${GREEN}[$CURRENT_TIME] $1${NC}"
}

log() {
    CURRENT_TIME=$(date +"%T.%3N")
    echo -e "[$CURRENT_TIME] $1${NC}"
}

log_section "Cleaning MyOS build and output files"

log "Deleting cmake_build/"
rm -rf cmake_build/
rm -rf cmake-build-debug/

log "Deleting initramfs/"
rm -rf initramfs/

log "Deleting sysroot/"
rm -rf sysroot/

log "Deleting myos.iso"
rm -f myos.iso

log_success "Done!"
