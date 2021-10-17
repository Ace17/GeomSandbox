#!/usr/bin/env bash
set -euo pipefail
g++ -g3 -Wall -Wextra -Werror *.cpp `sdl2-config --cflags --libs` -o GeomSandbox.exe
./GeomSandbox.exe "$@"
