# Planned hardware expansion (SuperMini)

Current connections are the three display pairs on GPIO 5/6, 7/10 and 20/21, plus the rear and top button ladders on ADC GPIO 0 and 1. The clock and SuperMini share ground. Keep only one USB power source connected.

## Buzzer

The original clock board has a `BUZ` connector. The modification guide labels its buzzer signal `BUZ+` and return `BUZ-`; this has not yet been measured on this particular clock. Reserve **GPIO 3** for PWM audio. Before soldering, identify whether the installed buzzer is passive or active, its voltage, current and polarity. If the load exceeds the GPIO drive limit or requires 5 V, drive it through a transistor or MOSFET with a common ground; do not connect it directly to the 3.3 V GPIO. Verify the existing board traces and any diode modification before connecting.

## Temperature and humidity

The sensor part and bus have not been identified. A close-up of its package marking and traces, or continuity measurements to the original MCU, is needed before choosing pins or firmware. **GPIO 4** is reserved provisionally for a one-wire sensor. An I²C sensor needs two lines; only GPIO 2, 8 and 9 remain after reserving GPIO 3 and 4, and these are ESP32-C3 boot strapping pins. Do not attach an unknown sensor to those pins. If the sensor is I²C, revisit the buzzer assignment or add an I/O expander after confirming startup behavior.

No buzzer or sensor firmware is enabled yet. Weather location in the web UI is a saved configuration field; forecast fetching is not implemented.

References: [KORTLINJE physical modification guide](https://github.com/kobebeefg/ikea-clock-lab/blob/main/tutorial/README.md), [ESP32-C3 strapping pins](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html).
