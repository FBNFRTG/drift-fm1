# Building DRIFT

Clone https://github.com/FBNFRTG/drift-fm1 or extract the complete source download. Engine files are included directly; no submodule initialization is needed.

## Prerequisites (macOS)

- Python 3 with Pillow and fontTools: `pip3 install Pillow fonttools` (the UI font and icons are
  rasterised at build time)
- Docker Desktop. The JieLi toolchain is Linux x86-64 only; the build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder Docker can share, e.g. under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).
- Emscripten (optional, `emcc`: the browser emulator, `web/emu/`).

On Linux x86-64 the toolchain runs natively and Docker is not needed.

## Firmware

From the repository root:

```sh
sh build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations. Output: `build/felucca.bin`, `build/loader/ota.bin`, `build/felucca.fwsc`. These internal filenames are inherited; the package is DRIFT, identity FM-1_983 and loader ota-FM-1_983. Do not change the identity for each release.

## Browser player and website

```sh
EMU_PREVIEW=0 sh web/emu/build.sh
python3 tools/package_drift.py
```

Requires Emscripten. The website is written to `build/drift-0.8-website/upload/`. Serve that directory over HTTPS or localhost. AudioWorklet and Web MIDI do not work over ordinary HTTP on a remote hostname or file://.

```sh
python3 -m http.server 8000 --directory build/drift-0.8-website/upload
```

Open http://localhost:8000/webplayer/ . Browser patches are local to that browser. USB hardware is not emulated.

## DRIFT checks

After building the firmware (generated headers in build/gen):

```sh
sh tests/run_tests.sh
```

The script checks sample conversion, installer protocol, native DRIFT synthesis/storage, OTA and loader. If build/emu/felucca.wasm exists, it also runs the DRIFT WebAssembly checks. Tests are software simulations; they cannot certify physical update/recovery behaviour.

## Install

Use https://www.fbnfrtg.de/tools/fm1/drift/webplayer/install/ or `python3 tools/fm1_install.py build/felucca.fwsc` (requires mido and python-rtmidi). Back up device data first.

## Included assets

Keep `assets/`, `firmware/engines/`, the fonts and licence files: the build uses them even where internal names refer to Felucca. External SDK/toolchain files are obtained separately as described above.
