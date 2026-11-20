#pragma once
#include <Arduino.h>
#include <string.h>
#include <math.h>
#include "LedMatrix.h"

namespace Patterns {
constexpr int W = IkeaMatrix::WIDTH, H = IkeaMatrix::HEIGHT;
inline uint8_t life[W * H] = {};

inline void seedLife() {
  memset(life, 0, sizeof(life));
  for (int i = 0; i < W * H; ++i) life[i] = random(100) < 23;
}

inline void sparkLife() {
  // Add a small moving shape without clearing the current generation.
  static constexpr uint8_t glider[5][2] = {{1,0},{2,1},{0,2},{1,2},{2,2}};
  int left = random(W), top = random(H), rotation = random(4);
  for (auto &point : glider) {
    int x = point[0], y = point[1];
    for (int i = 0; i < rotation; ++i) { int temp = x; x = 2 - y; y = temp; }
    life[((top + y) % H) * W + ((left + x) % W)] = 1;
  }
}

inline bool stepLife() {
  uint8_t next[W * H] = {};
  for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
    uint8_t neighbors = 0;
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
      if (dx || dy)
        neighbors += life[((y + dy + H) % H) * W + (x + dx + W) % W];
    bool alive = life[y * W + x];
    next[y * W + x] = neighbors == 3 || (alive && neighbors == 2);
  }
  bool changed = memcmp(life, next, sizeof(life)) != 0;
  memcpy(life, next, sizeof(life));
  return changed;
}

inline void drawLife(IkeaMatrix::Frame &frame) {
  frame.clear();
  for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
    if (life[y * W + x]) frame.drawPixel(x, y);
}

// Full-width snow: free flakes above a slowly changing, unframed snowbank.
namespace Snow {
constexpr uint8_t FLAKES = 22;
struct Flake { uint8_t x, y, period, phase; };
inline Flake flakes[FLAKES] = {};
inline uint8_t bank[W] = {};
inline uint32_t tick = 0;

inline void respawn(Flake &flake, bool anywhere = false) {
  flake.x = uint8_t(random(W));
  flake.y = anywhere ? uint8_t(random(H - 2)) : 0;
  flake.period = uint8_t(2 + random(3));
  flake.phase = uint8_t(random(flake.period));
}

inline void reset() {
  tick = 0;
  for (int x = 0; x < W; ++x) bank[x] = uint8_t(1 + ((x / 8) % 3 == 1));
  for (auto &flake : flakes) respawn(flake, true);
}

inline void step() {
  ++tick;
  // The wind changes slowly; flakes move sideways at different moments.
  int wind = (tick / 65) % 3 == 0 ? -1 : (tick / 65) % 3 == 1 ? 0 : 1;
  for (uint8_t i = 0; i < FLAKES; ++i) {
    Flake &flake = flakes[i];
    if (tick % flake.period != flake.phase) continue;
    if ((tick + i * 7) % 4 == 0 && wind) {
      int nextX = int(flake.x) + wind;
      if (nextX < 0) nextX = W - 1;
      if (nextX >= W) nextX = 0;
      flake.x = uint8_t(nextX);
    }
    ++flake.y;
    if (flake.y >= H - bank[flake.x]) {
      if (random(80) == 0 && bank[flake.x] < 3) ++bank[flake.x];
      respawn(flake);
    }
  }
  if (tick % 32 == 0) {
    uint8_t x = uint8_t(random(W));
    if (bank[x] > 1) --bank[x];
  }
}

inline void draw(IkeaMatrix::Frame &frame) {
  frame.clear();
  for (int x = 0; x < W; ++x)
    for (int y = H - bank[x]; y < H; ++y) frame.drawPixel(x, y);
  for (const auto &flake : flakes)
    if (flake.y < H - bank[flake.x]) frame.drawPixel(flake.x, flake.y);
}
} // namespace Snow

// Each column has its own pace and tail. All coordinates stay in 42x9 space.
namespace Rain {
struct Column { int8_t head, wind; uint8_t period, phase, trail; };
inline Column columns[W] = {};
inline uint32_t tick = 0;
inline void newDrop(Column &drop, bool anywhere = false) {
  drop.head = anywhere ? int8_t(random(-8, H)) : int8_t(-random(1, 10));
  drop.wind = int8_t(random(-1, 2));
  drop.period = uint8_t(random(1, 5));
  drop.phase = uint8_t(random(drop.period));
  drop.trail = uint8_t(random(1, 5));
}
inline void reset() { tick = 0; for (auto &drop : columns) newDrop(drop, true); }
inline void step() {
  ++tick;
  for (auto &drop : columns) {
    if (tick % drop.period == drop.phase) ++drop.head;
    if (drop.head >= H + drop.trail) newDrop(drop);
  }
}
inline void draw(IkeaMatrix::Frame &frame) {
  frame.clear();
  for (int x = 0; x < W; ++x)
    for (int tail = 0; tail < columns[x].trail; ++tail) {
      int y = columns[x].head - tail;
      int shiftedX = x + (columns[x].wind * tail) / 2;
      if (shiftedX >= 0 && shiftedX < W && y >= 0 && y < H)
        frame.drawPixel(shiftedX, y);
    }
}
}

namespace Dither {
inline uint32_t phase = 0;
static constexpr uint8_t BAYER[4][4] = {
  {0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}
};
inline void reset() { phase = 0; }
inline void step() { ++phase; }
inline void draw(IkeaMatrix::Frame &frame) {
  frame.clear();
  float horizontal[W], vertical[H];
  for (int x = 0; x < W; ++x) horizontal[x] = sinf(x * .38f + phase * .11f);
  for (int y = 0; y < H; ++y) vertical[y] = sinf(y * 1.03f - phase * .075f);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      int value = int(128 + 62 * (horizontal[x] + vertical[y]));
      if (value > int(BAYER[y & 3][x & 3]) * 16 + 8) frame.drawPixel(x, y);
    }
}
}

} // namespace Patterns
