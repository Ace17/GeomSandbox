#!/usr/bin/env bash
set -euo pipefail
ctags -R
make -j`nproc`
./bin/GeomSandbox.exe "$@"
