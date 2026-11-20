#pragma once
#include <Arduino.h>
#include "LedMatrix.h"
#include "ClockFaces.h"
#include "Tiny5Clock.h"
#include "PixelText.h"

namespace Apps {
namespace Tempo {
inline uint16_t intervals[8] = {};
inline uint8_t count = 0, cursor = 0, meter = 4;
inline uint32_t lastTap = 0;
inline uint16_t bpm = 0;
inline void reset() { count = cursor = 0; lastTap = 0; bpm = 0; }
inline void nextMeter() { meter = meter == 2 ? 3 : meter == 3 ? 4 : 2; }
inline void tap(uint32_t now) {
  if (lastTap) {
    uint32_t gap = now - lastTap;
    if (gap < 180 || gap > 2000) { count = cursor = 0; bpm = 0; }
    else {
      intervals[cursor] = uint16_t(gap);
      cursor = (cursor + 1) % 8;
      if (count < 8) ++count;
      if (count >= 3) {
        uint16_t sorted[8];
        memcpy(sorted, intervals, count * sizeof(uint16_t));
        for (int i = 1; i < count; ++i) {
          uint16_t value = sorted[i]; int j = i - 1;
          while (j >= 0 && sorted[j] > value) { sorted[j+1] = sorted[j]; --j; }
          sorted[j+1] = value;
        }
        uint16_t median = sorted[count / 2];
        uint32_t total = 0; uint8_t accepted = 0;
        for (int i = 0; i < count; ++i)
          if (sorted[i] * 4U >= median * 3U && sorted[i] * 4U <= median * 5U) {
            total += sorted[i]; ++accepted;
          }
        if (accepted) bpm = uint16_t(60000UL * accepted / total);
      }
    }
  }
  lastTap = now;
}
inline void draw(IkeaMatrix::Frame &frame, uint32_t now) {
  unsigned value = bpm > 999 ? 999 : bpm;
  char label[6]; snprintf(label, sizeof label, " %03u ", value);
  PixelText::draw(frame, bpm ? label : " TAP ");
  if (bpm && lastTap) {
    uint32_t period = 60000UL / bpm;
    uint32_t elapsed = now - lastTap;
    uint8_t beat = uint8_t((elapsed / period) % meter);
    for (int i = 0; i < meter; ++i)
      if (i == beat && elapsed % period < period / 3)
        for (int y = 3; y <= 5; ++y) frame.drawPixel(33 + i * 2, y);
  } else for (int i = 0; i < 4; ++i) frame.drawPixel(33 + i * 2, 4);
}
}

namespace Pomodoro {
inline uint8_t workMinutes = 25, breakMinutes = 5, cycles = 0;
inline bool work = true, running = false, demoMode = false;
inline uint32_t remainingMs = 25UL * 60000, lastTick = 0, flashUntil = 0;
inline uint32_t duration() { return (work ? workMinutes : breakMinutes) * 60000UL; }
inline void reset() { work = true; running = false; demoMode = false; remainingMs = duration(); flashUntil = millis() + 1500; cycles = 0; lastTick = millis(); }
inline void toggle() { running = !running; lastTick = millis(); }
inline void skip() { work = !work; if (work) ++cycles; demoMode = false; remainingMs = duration(); lastTick = millis(); flashUntil = millis() + 1800; }
inline void demo() { work = true; running = true; demoMode = true; remainingMs = 10000; lastTick = millis(); flashUntil = 0; }
inline void configure(uint8_t workValue, uint8_t breakValue) {
  if (workValue < 1 || workValue > 90 || breakValue < 1 || breakValue > 90) return;
  workMinutes = workValue; breakMinutes = breakValue; reset();
}
inline void tick(uint32_t now) {
  if (!running) return;
  uint32_t elapsed = now - lastTick; lastTick = now;
  if (elapsed >= remainingMs) skip();
  else remainingMs -= elapsed;
}
inline void draw(IkeaMatrix::Frame &frame, uint32_t now) {
  if (flashUntil && int32_t(flashUntil - now) > 0) {
    PixelText::draw(frame, work ? "WORK" : "REST");
    return;
  }
  frame.clear();
  uint32_t total = demoMode ? 10000 : duration();
  int filled = total ? int((remainingMs * 42ULL + total - 1) / total) : 0;
  if (filled > 42) filled = 42;
  for (int x = 0; x < filled; ++x)
    for (int y = work ? 2 : 4; y < 8; ++y)
      if (x < filled - 2 || ((x * 13 + y * 7 + now / 250) % 4 != 0)) frame.drawPixel(x, y);
  if (flashUntil && int32_t(flashUntil - now) > 0 && (now / 180) % 2)
    for (int x = 0; x < 42; x += 2) frame.drawPixel(x, 0);
  if (!running) for (int x = 18; x <= 23; x += 4) frame.drawPixel(x, 0);
}
}

namespace Stopwatch {
inline uint32_t elapsedMs = 0, lastTick = 0, lapMs = 0, lapUntil = 0;
inline bool running = false;
inline void reset() { elapsedMs = 0; lastTick = millis(); lapMs = lapUntil = 0; running = false; }
inline void toggle() { running = !running; lastTick = millis(); }
inline void lap() { lapMs = elapsedMs; lapUntil = millis() + 3000; }
inline void tick(uint32_t now) { if (running) { elapsedMs += now - lastTick; lastTick = now; } }
inline void draw(IkeaMatrix::Frame &frame, ClockFaces::Font font, uint32_t now) {
  uint32_t showMs = lapUntil && int32_t(lapUntil - now) > 0 ? lapMs : elapsedMs;
  unsigned seconds = showMs / 1000;
  char text[6]; snprintf(text, sizeof text, "%02u:%02u", (seconds / 60) % 100, seconds % 60);
  ClockFaces::draw(frame, text, font);
  if (!running) frame.drawPixel(0, 0);
}
}

namespace Countdown {
inline uint16_t presetSeconds = 300;
inline uint32_t remainingMs = 300000, lastTick = 0;
inline bool running = false, finished = false;
inline void reset() { remainingMs = uint32_t(presetSeconds) * 1000; running = finished = false; lastTick = millis(); }
inline bool configure(uint16_t seconds) {
  if (seconds < 10 || seconds > 5999) return false;
  presetSeconds = seconds; reset(); return true;
}
inline void toggle() {
  if (finished) reset();
  running = !running; lastTick = millis();
}
inline void tick(uint32_t now) {
  if (!running) return;
  uint32_t elapsed = now - lastTick; lastTick = now;
  if (elapsed >= remainingMs) { remainingMs = 0; running = false; finished = true; }
  else remainingMs -= elapsed;
}
inline void draw(IkeaMatrix::Frame &frame, ClockFaces::Font font, uint32_t now) {
  if (finished && (now / 400) % 2) { frame.clear(); return; }
  unsigned remaining = (remainingMs + 999) / 1000;
  char text[6]; snprintf(text, sizeof text, "%02u:%02u", remaining / 60, remaining % 60);
  ClockFaces::draw(frame, text, font);
  if (!running && !finished) frame.drawPixel(0, 0);
}
}

namespace Sky {
enum class Condition : uint8_t { Sun, Cloud, Rain, Snow };
inline Condition condition = Condition::Sun;
inline uint32_t phase = 0;
inline void set(Condition value) { condition = value; phase = 0; }
inline const char *name() {
  switch (condition) { case Condition::Cloud: return "cloud"; case Condition::Rain: return "rain"; case Condition::Snow: return "snow"; default: return "sun"; }
}
inline bool parse(const String &value) {
  if (value == "sun") set(Condition::Sun);
  else if (value == "cloud") set(Condition::Cloud);
  else if (value == "rain") set(Condition::Rain);
  else if (value == "snow") set(Condition::Snow);
  else return false;
  return true;
}
inline void step() { ++phase; }
inline void draw(IkeaMatrix::Frame &frame) {
  if (phase % 65 < 15) {
    PixelText::draw(frame, condition == Condition::Sun ? "SUN" :
                           condition == Condition::Cloud ? "CLOUD" :
                           condition == Condition::Rain ? "RAIN" : "SNOW");
    return;
  }
  frame.clear();
  if (condition == Condition::Sun) {
    for (int x = 7; x <= 12; ++x) { frame.drawPixel(x, 1); frame.drawPixel(x, 6); }
    for (int y = 1; y <= 6; ++y) { frame.drawPixel(7, y); frame.drawPixel(12, y); }
    frame.drawPixel(9, 0); frame.drawPixel(9, 7); frame.drawPixel(5, 3); frame.drawPixel(14, 3);
    for (int x = 20; x < 42; ++x) if ((x + phase / 5) % 5 < 3) frame.drawPixel(x, 7);
  } else {
    int shift = int((phase / 5) % 8) - 4;
    for (int base : {7, 26}) {
      base += shift;
      for (int x = 1; x <= 9; ++x) for (int y = 3; y <= 5; ++y)
        if (x > 1 && x < 9 || y == 5) frame.drawPixel(base + x, y);
      for (int x = 3; x <= 6; ++x) frame.drawPixel(base + x, 2);
    }
    if (condition == Condition::Rain || condition == Condition::Snow)
      for (int x = 4; x < 40; x += 4) {
        int y = 6 + int((phase + x) % 3);
        frame.drawPixel(x + int((phase / 3) % 2), y);
      }
  }
}
}
} // namespace Apps
