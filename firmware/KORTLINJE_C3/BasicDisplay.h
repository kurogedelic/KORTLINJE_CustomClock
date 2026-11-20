#pragma once
// Derived from https://github.com/kobebeefg/ikea-clock-lab (no license specified). Not covered by this repository's MIT license.
#include <Arduino.h>
#include "LedMatrix.h"
#include "Board.h"
namespace BasicDisplay {
constexpr const uint8_t (&CLOCK)[3] = DISPLAY_CLK;
constexpr const uint8_t (&DATA)[3] = DISPLAY_DATA;
constexpr uint8_t HALF_US = 10;
inline void clockLevel(bool high) { for (uint8_t pin : CLOCK) digitalWrite(pin, high); }
inline void dataLevel(bool high) { for (uint8_t pin : DATA) digitalWrite(pin, high); }
inline void start() { clockLevel(LOW); dataLevel(HIGH); delayMicroseconds(HALF_US); clockLevel(HIGH); delayMicroseconds(HALF_US); dataLevel(LOW); delayMicroseconds(HALF_US); clockLevel(LOW); }
inline void stop() { clockLevel(LOW); dataLevel(LOW); delayMicroseconds(HALF_US); clockLevel(HIGH); delayMicroseconds(HALF_US); dataLevel(HIGH); delayMicroseconds(HALF_US); }
inline void bytes(uint8_t a, uint8_t b, uint8_t c) {
  const uint8_t v[3] = {a,b,c};
  for (uint8_t bit = 0; bit < 8; ++bit) {
    clockLevel(LOW);
    for (uint8_t chip = 0; chip < 3; ++chip) digitalWrite(DATA[chip], (v[chip] >> bit) & 1);
    delayMicroseconds(HALF_US); clockLevel(HIGH); delayMicroseconds(HALF_US); clockLevel(LOW);
  }
}
inline void command(uint8_t value) { start(); bytes(value,value,value); stop(); }
inline void show(const IkeaMatrix::Frame &frame, uint8_t brightness) {
  IkeaMatrix::Frame output = frame;
  if (IkeaMatrix::invertPixels)
    for (int y = 0; y < IkeaMatrix::HEIGHT; ++y)
      for (int x = 0; x < IkeaMatrix::WIDTH; ++x) {
        IkeaMatrix::Address a;
        if (IkeaMatrix::pixelAddress(x, y, a))
          output.ram[a.chip][a.index / 8] ^= uint8_t(1U << (a.index % 8));
      }
  command(0x40); start(); bytes(0xC0,0xC0,0xC0);
  for (uint8_t a=0;a<16;++a) bytes(output.ram[0][a],output.ram[1][a],output.ram[2][a]);
  stop(); command(0x88 | (brightness & 7));
}
inline void begin() {
  clockLevel(LOW);
  for (uint8_t pin : CLOCK) pinMode(pin,OUTPUT);
  for (uint8_t pin : DATA) { digitalWrite(pin,LOW); pinMode(pin,OUTPUT); }
  stop(); command(0x80);
}
}
