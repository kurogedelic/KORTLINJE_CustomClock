#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "KatakanaFont.h"
#include "LedMatrix.h"

// ASCII uses the existing 5x7 face; full-width Katakana uses Matrix Sans dots.
namespace MqttText {
constexpr size_t MAX_BYTES = 180;
constexpr size_t MAX_GLYPHS = 60;
constexpr int MAX_COLUMNS = MAX_GLYPHS * 8;
inline GFXcanvas1 canvas(MAX_COLUMNS, IkeaMatrix::HEIGHT);
inline int length = 0;

inline bool decode(const char *&cursor, const char *end, uint32_t &codepoint) {
  if (cursor >= end) return false;
  uint8_t a = uint8_t(*cursor++);
  if (a < 0x80) { codepoint = a; return true; }
  if (a >= 0xc2 && a <= 0xdf) {
    if (end - cursor < 1) return false;
    uint8_t b = uint8_t(*cursor++);
    if ((b & 0xc0) != 0x80) return false;
    codepoint = ((a & 0x1f) << 6) | (b & 0x3f);
    return true;
  }
  if (a >= 0xe0 && a <= 0xef) {
    if (end - cursor < 2) return false;
    uint8_t b = uint8_t(*cursor++), c = uint8_t(*cursor++);
    if ((b & 0xc0) != 0x80 || (c & 0xc0) != 0x80 ||
        (a == 0xe0 && b < 0xa0) || (a == 0xed && b >= 0xa0)) return false;
    codepoint = ((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f);
    return true;
  }
  return false;
}

inline bool prepare(const char *text, size_t count) {
  if (!text || count == 0 || count > MAX_BYTES || !canvas.getBuffer()) return false;
  const char *cursor = text, *end = text + count;
  uint32_t codepoints[MAX_GLYPHS] = {};
  size_t glyphCount = 0;
  int columns = 0;
  while (cursor < end) {
    uint32_t codepoint = 0;
    if (!decode(cursor, end, codepoint) || glyphCount >= MAX_GLYPHS) return false;
    int width = 0;
    if (codepoint >= 32 && codepoint <= 126) width = 6;
    else if (const auto *glyph = KatakanaFont::find(codepoint)) width = glyph->width + 1;
    else return false;
    if (columns + width > MAX_COLUMNS) return false;
    codepoints[glyphCount++] = codepoint;
    columns += width;
  }

  canvas.fillScreen(0);
  canvas.setTextSize(1);
  canvas.setTextColor(1);
  canvas.setTextWrap(false);
  int x = 0;
  for (size_t i = 0; i < glyphCount; ++i) {
    uint32_t codepoint = codepoints[i];
    if (codepoint <= 126) {
      char ch = char(codepoint);
      if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
      canvas.setCursor(x, 1);
      canvas.write(uint8_t(ch));
      x += 6;
    } else {
      const auto *glyph = KatakanaFont::find(codepoint);
      for (int dx = 0; dx < glyph->width; ++dx)
        for (int y = 0; y < IkeaMatrix::HEIGHT; ++y)
          if (glyph->columns[dx] & (1 << y)) canvas.drawPixel(x + dx, y, 1);
      x += glyph->width + 1;
    }
  }
  length = x;
  return true;
}

inline void draw(IkeaMatrix::Frame &frame, int left) {
  frame.clear();
  for (int x = 0; x < IkeaMatrix::WIDTH; ++x) {
    int source = x - left;
    if (source < 0 || source >= length) continue;
    for (int y = 0; y < IkeaMatrix::HEIGHT; ++y)
      if (canvas.getPixel(source, y)) frame.drawPixel(x, y);
  }
}
} // namespace MqttText
