#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
node web/samples/test.mjs
node web/install/test.mjs
cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src -o build/drift-test tests/drift_test.c -lm
build/drift-test
cc -O2 -o build/ota-test tests/ota_test.c
build/ota-test build/felucca.fwsc
cc -O2 -o build/ldr-test tests/ldr_test.c
build/ldr-test build/felucca.fwsc build/felucca.fwsc
if [ -f build/emu/felucca.wasm ]; then node web/emu/drift_test.mjs; fi
