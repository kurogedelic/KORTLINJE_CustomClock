#!/usr/bin/env bash
# Usage: tools/build.sh supermini|xiao  -> dist/KORTLINJE_C3-<board>-{app,merged}.bin
# Builds from a temp copy using MqttPrivate.h.example (MQTT disabled).
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
board="${1:?supermini|xiao}"
case "$board" in
  supermini) fqbn="esp32:esp32:esp32c3:CDCOnBoot=cdc"; flag="KORTLINJE_BOARD_SUPERMINI" ;;
  xiao)      fqbn="esp32:esp32:XIAO_ESP32C3:CDCOnBoot=cdc"; flag="KORTLINJE_BOARD_XIAO_C3" ;;
  *) echo "unknown board" >&2; exit 1 ;;
esac
work="$(mktemp -d)"; [ -n "${KEEP:-}" ] || trap 'rm -rf "$work"' EXIT
cp -r "$root/firmware/KORTLINJE_C3" "$work/"
sk="$work/KORTLINJE_C3"
[ -f "$sk/MqttPrivate.h" ] || cp "$sk/MqttPrivate.h.example" "$sk/MqttPrivate.h"
arduino-cli compile --fqbn "$fqbn" --build-property "compiler.cpp.extra_flags=-D$flag" \
  --output-dir "$work/out" "$sk"
mkdir -p "$root/dist"
cp "$work/out/KORTLINJE_C3.ino.bin" "$root/dist/KORTLINJE_C3-$board-app.bin"
if [ -n "${ARDUINO_DIRECTORIES_DATA:-}" ]; then
  arduino_data="$ARDUINO_DIRECTORIES_DATA"
elif [ -d "$HOME/Library/Arduino15" ]; then
  arduino_data="$HOME/Library/Arduino15"
else
  arduino_data="$HOME/.arduino15"
fi
bootapp0="$(find "$arduino_data/packages/esp32/hardware/esp32" -name boot_app0.bin | head -1)"
esptool="$(find "$arduino_data/packages/esp32/tools/esptool_py" -type f \( -name esptool -o -name esptool.py \) | head -1)"
[ -f "$bootapp0" ] && [ -f "$esptool" ] || { echo "ESP32 merge tools not found" >&2; exit 1; }
if [[ "$esptool" == *.py ]]; then esp_cmd=(python3 "$esptool"); else esp_cmd=("$esptool"); fi
"${esp_cmd[@]}" --chip esp32c3 merge_bin -o "$root/dist/KORTLINJE_C3-$board-merged.bin" \
  --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x0 "$work/out/KORTLINJE_C3.ino.bootloader.bin" 0x8000 "$work/out/KORTLINJE_C3.ino.partitions.bin" \
  0xe000 "$bootapp0" 0x10000 "$work/out/KORTLINJE_C3.ino.bin"
