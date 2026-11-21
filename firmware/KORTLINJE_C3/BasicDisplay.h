#pragma once

#include <Arduino.h>
#include "Board.h"
#include "LedMatrix.h"

// VK1640 serial protocol; the three chips receive matching commands together.
namespace BasicDisplay {
constexpr unsigned pulseUs = 10;

inline void clocks(uint8_t level) {
  for (uint8_t pin : DISPLAY_CLK) digitalWrite(pin, level);
}

inline void dataLines(uint8_t level) {
  for (uint8_t pin : DISPLAY_DATA) digitalWrite(pin, level);
}

inline void beginTransfer() {
  clocks(LOW);
  dataLines(HIGH);
  delayMicroseconds(pulseUs);
  clocks(HIGH);
  delayMicroseconds(pulseUs);
  dataLines(LOW);
  delayMicroseconds(pulseUs);
  clocks(LOW);
}

inline void endTransfer() {
  clocks(LOW);
  dataLines(LOW);
  delayMicroseconds(pulseUs);
  clocks(HIGH);
  delayMicroseconds(pulseUs);
  dataLines(HIGH);
  delayMicroseconds(pulseUs);
}

inline void transmit(uint8_t u1, uint8_t u2, uint8_t u4) {
  const uint8_t payload[3] = {u1, u2, u4};
  for (unsigned bit = 0; bit < 8; ++bit) {
    clocks(LOW);
    for (unsigned device = 0; device < 3; ++device)
      digitalWrite(DISPLAY_DATA[device], (payload[device] >> bit) & 1U);
    delayMicroseconds(pulseUs);
    clocks(HIGH);
    delayMicroseconds(pulseUs);
  }
  clocks(LOW);
}

inline void broadcast(uint8_t command) {
  beginTransfer();
  transmit(command, command, command);
  endTransfer();
}

inline void show(const IkeaMatrix::Frame &frame, uint8_t brightness) {
  IkeaMatrix::Frame output = frame;
  if (IkeaMatrix::invertPixels) {
    for (int y = 0; y < IkeaMatrix::HEIGHT; ++y) {
      for (int x = 0; x < IkeaMatrix::WIDTH; ++x) {
        IkeaMatrix::Address address;
        IkeaMatrix::pixelAddress(x, y, address);
        output.ram[address.chip][address.index >> 3] ^=
            uint8_t(1U << (address.index & 7));
      }
    }
  }

  broadcast(0x40);
  beginTransfer();
  transmit(0xC0, 0xC0, 0xC0);
  for (unsigned byte = 0; byte < 16; ++byte)
    transmit(output.ram[0][byte], output.ram[1][byte], output.ram[2][byte]);
  endTransfer();
  broadcast(uint8_t(0x88 | (brightness & 7)));
}

inline void begin() {
  for (uint8_t pin : DISPLAY_CLK) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  for (uint8_t pin : DISPLAY_DATA) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  endTransfer();
  broadcast(0x80);
}
}  // namespace BasicDisplay
