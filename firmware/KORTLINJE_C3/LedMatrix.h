#pragma once

#include <stdint.h>
#include <string.h>

// The board's LED-to-VK1640 routing. See docs/HARDWARE.md.
namespace IkeaMatrix {
constexpr int WIDTH = 42;
constexpr int HEIGHT = 9;
inline bool rotate180 = true;
inline bool invertPixels = false;

struct Address { uint8_t chip; uint8_t index; };
enum Indicator : uint8_t { LEFT_TOP, LEFT_MIDDLE, LEFT_BOTTOM, RIGHT_TOP, RIGHT_BOTTOM };
constexpr Address INDICATORS[5] = {
    {1, 127}, {2, 126}, {1, 126}, {0, 127}, {0, 126}};

inline bool pixelAddress(int x, int y, Address &address) {
  if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return false;
  if (rotate180) {
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;
  }

  // The first six and remaining eight columns have different row strides.
  constexpr uint8_t firstRow[14] = {
      125, 124, 123, 122, 121, 120, 71, 70, 69, 68, 67, 66, 65, 64};
  const int column = x % 14;
  address.chip = uint8_t(2 - x / 14);
  address.index = uint8_t(firstRow[column] - y * (column < 6 ? 6 : 8));
  return true;
}

class Frame {
 public:
  uint8_t ram[3][16] = {};

  void clear() { memset(ram, 0, sizeof(ram)); }

  void set(Address address, bool on = true) {
    if (address.chip >= 3 || address.index >= 128) return;
    const uint8_t mask = uint8_t(1U << (address.index & 7));
    uint8_t &byte = ram[address.chip][address.index >> 3];
    byte = on ? uint8_t(byte | mask) : uint8_t(byte & ~mask);
  }

  void drawPixel(int x, int y, bool on = true) {
    Address address;
    if (pixelAddress(x, y, address)) set(address, on);
  }

  void setIndicator(Indicator indicator, bool on = true) {
    if (indicator < 5) set(INDICATORS[indicator], on);
  }
};
}  // namespace IkeaMatrix
