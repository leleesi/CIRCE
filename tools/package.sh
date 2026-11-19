#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tools/prepare_presentmon.py
sh tools/build-linux.sh
makensis -V2 instalador/CIRCE.nsi
