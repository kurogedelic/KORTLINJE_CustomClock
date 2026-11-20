# KORTLINJE CustomClock

Custom firmware and web UI for the IKEA KORTLINJE clock (42×9 LED matrix, three VK1640 drivers) with an ESP32-C3 replacing the original controller.

Display driver and wiring are based on [kobebeefg/ikea-clock-lab](https://github.com/kobebeefg/ikea-clock-lab).

## Features

Time, date, seconds, calendar, patterns, games, Pomodoro, stopwatch, countdown, tempo, manual sky display, local web control (light/dark), optional MQTT over WSS, rollback-aware OTA.

## Supported boards

Select the board in `firmware/KORTLINJE_C3/Board.h` (or build with `-DKORTLINJE_BOARD_XIAO_C3`).

| Signal | SuperMini (GPIO) | XIAO ESP32-C3 (GPIO / label) |
|---|---:|---:|
| U1 CLK / DATA | 5 / 6 | 5 / 6 (D3 / D4) |
| U2 CLK / DATA | 7 / 10 | 7 / 10 (D5 / D10) |
| U4 CLK / DATA | 20 / 21 | 20 / 21 (D7 / D6) |
| Rear POWER ladder (ADC) | 0 | 3 (D1) |
| TOP-KEY ladder (ADC) | 1 | 4 (D2) |

The XIAO assignment is not yet verified on hardware.

## Install a release binary

Download from Releases and verify with `SHA256SUMS.txt`.

- `*-merged.bin`: full image, first flash: `esptool.py --chip esp32c3 write_flash 0x0 KORTLINJE_C3-<board>-merged.bin`
- `*-app.bin`: application only, for OTA or flashing at `0x10000`

Release binaries have MQTT disabled. Provision Wi-Fi and OTA credentials over USB serial (stored in NVS).

## Build

Requires Arduino-ESP32 2.0.17, ESP32-C3 4 MB board, USB CDC on boot, and the Adafruit GFX Library (with Adafruit BusIO).

1. Copy `MqttPrivate.h.example` to `MqttPrivate.h` and edit it. It is git-ignored. Leave `MQTT_URI` empty to disable MQTT.
2. Open `firmware/KORTLINJE_C3/KORTLINJE_C3.ino`, or run `tools/build.sh supermini|xiao` (outputs to `dist/`).

## Web UI

`web/index.html` is the source. Open it directly to preview. It follows the system light/dark setting. After editing, run `python3 tools/embed_web_ui.py` to regenerate `firmware/KORTLINJE_C3/WebUi.h`.

## Credits

- [kobebeefg/ikea-clock-lab](https://github.com/kobebeefg/ikea-clock-lab): `LedMatrix.h` and `BasicDisplay.h` (modified: C3 pin assignment, pixel inversion), VK1640 wiring and D44 modification ([tutorial](https://github.com/kobebeefg/ikea-clock-lab/blob/main/tutorial/README.md))
- Fonts: [Press Start 2P](https://github.com/google/fonts/tree/main/ofl/pressstart2p), [Tiny5 Mono](https://github.com/Gissio/font_Tiny5Mono), [Matrix Sans](https://github.com/FriedOrange/MatrixSans) (SIL OFL 1.1; license files in the sketch folder)

## License

MIT, see `LICENSE`, except:

- `LedMatrix.h` and `BasicDisplay.h` are derived from kobebeefg/ikea-clock-lab, which has no license. They are not covered by the MIT license; permission from the author is pending.
- Bundled fonts remain under their own OFL licenses.
