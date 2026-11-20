#pragma once
#include <Arduino.h>
#include "LedMatrix.h"
#include "PixelText.h"

namespace Games {
namespace Snake {
struct Point { int8_t x, y; };
inline Point body[IkeaMatrix::WIDTH * IkeaMatrix::HEIGHT] = {};
inline uint16_t length = 4;
inline int8_t dx = 1, dy = 0, nextDx = 1, nextDy = 0;
inline Point food = {30, 4};
inline bool over = false;
inline uint32_t ticks = 0;
inline bool occupied(int x, int y, int count) {
  for (int i = 0; i < count; ++i) if (body[i].x == x && body[i].y == y) return true;
  return false;
}
inline void placeFood() {
  for (int tries = 0; tries < 100; ++tries) {
    int x = random(IkeaMatrix::WIDTH), y = random(IkeaMatrix::HEIGHT);
    if (!occupied(x, y, length)) { food = {int8_t(x), int8_t(y)}; return; }
  }
  food = {-1, -1};
}
inline void reset() {
  length = 4; dx = nextDx = 1; dy = nextDy = 0; over = false; ticks = 0;
  for (int i = 0; i < length; ++i) body[i] = {int8_t(20 - i), 4};
  placeFood();
}
inline void control(const String &action) {
  if (action == "restart") { reset(); return; }
  int8_t x = 0, y = 0;
  if (action == "up") y = -1;
  else if (action == "down") y = 1;
  else if (action == "left") x = -1;
  else if (action == "right") x = 1;
  else return;
  if (x != -dx || y != -dy) { nextDx = x; nextDy = y; }
}
inline void step() {
  if (over) return;
  ++ticks; dx = nextDx; dy = nextDy;
  int x = (body[0].x + dx + IkeaMatrix::WIDTH) % IkeaMatrix::WIDTH;
  int y = (body[0].y + dy + IkeaMatrix::HEIGHT) % IkeaMatrix::HEIGHT;
  bool grow = x == food.x && y == food.y;
  if (occupied(x, y, grow ? length : length - 1)) { over = true; return; }
  if (grow && length < IkeaMatrix::WIDTH * IkeaMatrix::HEIGHT) ++length;
  for (int i = length - 1; i > 0; --i) body[i] = body[i - 1];
  body[0] = {int8_t(x), int8_t(y)};
  if (grow) placeFood();
}
inline void draw(IkeaMatrix::Frame &frame) {
  frame.clear();
  for (int i = 0; i < length; ++i) frame.drawPixel(body[i].x, body[i].y);
  if ((ticks / 2) % 2 == 0 && food.x >= 0) frame.drawPixel(food.x, food.y);
  if (over) for (int y = 0; y < IkeaMatrix::HEIGHT; ++y) {
    frame.drawPixel(0, y); frame.drawPixel(IkeaMatrix::WIDTH - 1, y);
  }
}
}

namespace Pong {
inline int8_t player = 3, cpu = 3, ballX = 21, ballY = 4, dx = -1, dy = 1;
inline uint8_t playerScore = 0, cpuScore = 0;
inline uint32_t ticks = 0;
inline bool paused = false;
inline uint32_t scoreUntil = 0;
inline void serve(int direction) {
  ballX = 21; ballY = int8_t(random(2, 7));
  dx = int8_t(direction); dy = int8_t(random(2) ? 1 : -1);
}
inline void reset() {
  player = cpu = 3; playerScore = cpuScore = 0; ticks = 0; paused = false; scoreUntil = 0; serve(-1);
}
inline void control(const String &action) {
  if (action == "restart") { reset(); return; }
  if (action == "up" && player > 0) --player;
  if (action == "down" && player < 6) ++player;
}
inline void setPaddle(uint8_t position) {
  if (position <= 6) player = int8_t(position);
}
inline void toggle() { paused = !paused; }
inline void step() {
  if (paused) return;
  if (scoreUntil && int32_t(scoreUntil - millis()) > 0) return;
  ++ticks;
  if (ticks % 2 == 0) {
    if (ballY < cpu + 1 && cpu > 0) --cpu;
    else if (ballY > cpu + 1 && cpu < 6) ++cpu;
  }
  int nx = ballX + dx, ny = ballY + dy;
  if (ny < 0 || ny >= IkeaMatrix::HEIGHT) { dy = -dy; ny = ballY + dy; }
  if (nx == 2 && dx < 0) {
    if (ny >= player && ny < player + 3) { dx = 1; nx = 2; }
    else { ++cpuScore; scoreUntil = millis() + 1200; serve(-1); return; }
  }
  if (nx == 39 && dx > 0) {
    if (ny >= cpu && ny < cpu + 3) { dx = -1; nx = 39; }
    else { ++playerScore; scoreUntil = millis() + 1200; serve(1); return; }
  }
  ballX = int8_t(nx); ballY = int8_t(ny);
}
inline void draw(IkeaMatrix::Frame &frame) {
  if (scoreUntil && int32_t(scoreUntil - millis()) > 0) {
    char text[6];
    snprintf(text, sizeof text, "%02u:%02u", playerScore % 100, cpuScore % 100);
    PixelText::draw(frame, text);
    return;
  }
  frame.clear();
  for (int i = 0; i < 3; ++i) {
    frame.drawPixel(1, player + i); frame.drawPixel(40, cpu + i);
  }
  for (int y = 0; y < 9; y += 2) frame.drawPixel(20, y);
  frame.drawPixel(ballX, ballY);
}
}
} // namespace Games
