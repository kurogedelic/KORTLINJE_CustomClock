#pragma once
// Derived from https://github.com/kobebeefg/ikea-clock-lab (no license specified). Not covered by this repository's MIT license.
#include <stdint.h>
#include <string.h>
namespace IkeaMatrix {
inline bool rotate180 = true;
inline bool invertPixels = false;
constexpr int WIDTH = 42, HEIGHT = 9;
struct Address { uint8_t chip, index; };
enum Indicator : uint8_t { LEFT_TOP, LEFT_MIDDLE, LEFT_BOTTOM, RIGHT_TOP, RIGHT_BOTTOM };
constexpr Address INDICATORS[5] = {{1,127},{2,126},{1,126},{0,127},{0,126}};
inline bool pixelAddress(int x, int y, Address &out) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
  if (rotate180) { x = WIDTH - 1 - x; y = HEIGHT - 1 - y; }
  int localX = x % 14;
  out.chip = 2 - x / 14;
  out.index = localX < 6 ? 125 - 6*y - localX : 71 - 8*y - (localX-6);
  return true;
}
class Frame {
public:
  uint8_t ram[3][16] = {};
  void clear() { memset(ram, 0, sizeof(ram)); }
  void set(Address a, bool on = true) {
    if (a.chip >= 3 || a.index >= 128) return;
    uint8_t &v = ram[a.chip][a.index/8];
    uint8_t bit = uint8_t(1U << (a.index%8));
    if (on) v |= bit; else v &= uint8_t(~bit);
  }
  void drawPixel(int x, int y, bool on = true) { Address a; if (pixelAddress(x,y,a)) set(a,on); }
  void setIndicator(Indicator i, bool on = true) { if (uint8_t(i)<5) set(INDICATORS[uint8_t(i)],on); }
};
}
