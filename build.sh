#!/usr/bin/env bash
# Atalho: tudo do build mora em build/. Ex.: ./build.sh --all | ./build.sh --windows64 --rpi | ./build.sh --help
exec "$(dirname "$0")/build/build.sh" "$@"
