#pragma once
#include <Arduino.h>
#include "LedMatrix.h"
#include "PressStart2PClock.h"
#include "Tiny5Clock.h"

namespace ClockFaces {
enum class Font : uint8_t { Pixel, Thin, Segment, Tiny5 };
enum class Effect : uint8_t { Off, Wipe, Roll, Dissolve };

inline const char *name(Font font) {
  switch (font) {
    case Font::Thin: return "thin";
    case Font::Segment: return "segment";
    case Font::Tiny5: return "tiny5";
    default: return "pixel";
  }
}
inline const char *name(Effect effect) {
  switch (effect) {
    case Effect::Wipe: return "wipe";
    case Effect::Roll: return "roll";
    case Effect::Dissolve: return "dissolve";
    default: return "off";
  }
}
inline bool parseFont(const char *value, Font &font) {
  if (!strcmp(value, "pixel")) font = Font::Pixel;
  else if (!strcmp(value, "thin")) font = Font::Thin;
  else if (!strcmp(value, "segment")) font = Font::Segment;
  else if (!strcmp(value, "tiny5")) font = Font::Tiny5;
  else return false;
  return true;
}
inline bool parseEffect(const char *value, Effect &effect) {
  if (!strcmp(value, "off")) effect = Effect::Off;
  else if (!strcmp(value, "wipe")) effect = Effect::Wipe;
  else if (!strcmp(value, "roll")) effect = Effect::Roll;
  else if (!strcmp(value, "dissolve")) effect = Effect::Dissolve;
  else return false;
  return true;
}

// Original 5x7 single-stroke numeral design, columns are low bit first.
static constexpr uint8_t THIN[10][5] = {
  {0x3e,0x41,0x41,0x41,0x3e}, {0,0x42,0x7f,0x40,0},
  {0x62,0x51,0x49,0x49,0x46}, {0x22,0x41,0x49,0x49,0x36},
  {0x18,0x14,0x12,0x7f,0x10}, {0x2f,0x49,0x49,0x49,0x31},
  {0x3e,0x49,0x49,0x49,0x32}, {0x01,0x61,0x19,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x26,0x49,0x49,0x49,0x3e}
};
static constexpr uint8_t SEGMENTS[10] = {
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
}; // a,b,c,d,e,f,g

inline void drawGlyph(IkeaMatrix::Frame &frame, char ch, int left, Font font) {
  if (ch == ':') {
    frame.drawPixel(left + 4, 3);
    frame.drawPixel(left + 4, 6);
    return;
  }
  if (ch == '/') {
    for (int i = 0; i < 7; ++i) frame.drawPixel(left + 1 + (6 - i) / 2, 1 + i);
    return;
  }
  if (ch == '-') {
    for (int x = 2; x <= 6; ++x) frame.drawPixel(left + x, 4);
    return;
  }
  if (ch < '0' || ch > '9') return;
  uint8_t digit = ch - '0';
  if (font == Font::Thin) {
    for (int x = 0; x < 5; ++x)
      for (int y = 0; y < 7; ++y)
        if (THIN[digit][x] & (1 << y)) frame.drawPixel(left + x + 2, y + 1);
  } else {
    uint8_t bits = SEGMENTS[digit];
    if (bits & 0x01) for (int x = 2; x <= 5; ++x) frame.drawPixel(left + x, 1);
    if (bits & 0x02) for (int y = 2; y <= 3; ++y) frame.drawPixel(left + 6, y);
    if (bits & 0x04) for (int y = 5; y <= 6; ++y) frame.drawPixel(left + 6, y);
    if (bits & 0x08) for (int x = 2; x <= 5; ++x) frame.drawPixel(left + x, 7);
    if (bits & 0x10) for (int y = 5; y <= 6; ++y) frame.drawPixel(left + 1, y);
    if (bits & 0x20) for (int y = 2; y <= 3; ++y) frame.drawPixel(left + 1, y);
    if (bits & 0x40) for (int x = 2; x <= 5; ++x) frame.drawPixel(left + x, 4);
  }
}

inline void draw(IkeaMatrix::Frame &frame, const char text[6], Font font) {
  if (font == Font::Pixel) {
    PressStart2PClock::draw(frame, text);
    return;
  }
  if (font == Font::Tiny5) {
    Tiny5Clock::draw(frame, text);
    return;
  }
  frame.clear();
  for (int i = 0; i < 5; ++i) drawGlyph(frame, text[i], 1 + i * 8, font);
}

inline bool get(const IkeaMatrix::Frame &frame, int x, int y) {
  IkeaMatrix::Address a;
  if (!IkeaMatrix::pixelAddress(x, y, a)) return false;
  return (frame.ram[a.chip][a.index / 8] & (1U << (a.index % 8))) != 0;
}

inline void transition(IkeaMatrix::Frame &output, const char oldText[6],
                       const char newText[6], Font font, Effect effect,
                       uint8_t phase) {
  IkeaMatrix::Frame oldFrame, newFrame;
  draw(oldFrame, oldText, font);
  draw(newFrame, newText, font);
  output.clear();
  for (int x = 0; x < IkeaMatrix::WIDTH; ++x) {
    bool compact = font == Font::Tiny5;
    int left = compact ? 6 : 1;
    int width = compact ? 6 : 8;
    int index = (x - left) / width;
    bool changed = x >= left && index < 5 && oldText[index] != newText[index];
    for (int y = 0; y < IkeaMatrix::HEIGHT; ++y) {
      bool on = get(newFrame, x, y);
      if (changed && effect == Effect::Wipe)
        on = get((x - left) % width < phase ? newFrame : oldFrame, x, y);
      else if (changed && effect == Effect::Dissolve)
        on = get(((x * 29 + y * 17 + x * y * 7) % 9) < phase ? newFrame : oldFrame, x, y);
      else if (changed && effect == Effect::Roll)
        on = get(oldFrame, x, y - phase) || get(newFrame, x, y + 9 - phase);
      if (on) output.drawPixel(x, y);
    }
  }
}
} // namespace ClockFaces
