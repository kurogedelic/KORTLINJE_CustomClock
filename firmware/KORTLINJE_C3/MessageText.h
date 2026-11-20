#pragma once
#include <Arduino.h>
#include <string.h>
#include "LedMatrix.h"

// 42x9 text: KORTLINJEカンゼンニリカイシタ。
// Latin is 5x7. Kana follows the supplied 5x7 chart, with the adjusted ニ and ゼ.
namespace MessageText {
constexpr int MAX_COLUMNS = 160;
inline uint16_t columns[MAX_COLUMNS] = {};
inline int length = 0;

inline void add(uint16_t bits) {
  if (length < MAX_COLUMNS) columns[length++] = bits;
}
inline void space(int count = 2) { while (count-- > 0) add(0); }
inline void latin(const uint8_t (&bits)[5], int gap = 2) {
  for (uint8_t x = 0; x < 5; ++x) add(uint16_t(bits[x]) << 1);
  space(gap);
}
inline void kana(const char *const rows[], uint8_t rowCount, uint8_t width,
                 uint8_t top = 1) {
  for (uint8_t x = 0; x < width; ++x) {
    uint16_t bits = 0;
    for (uint8_t y = 0; y < rowCount; ++y)
      if (rows[y][x] == '#') bits |= uint16_t(1) << (y + top);
    add(bits);
  }
  space();
}

inline void build() {
  length = 0;
  static constexpr uint8_t K[5] = {0x7f,0x08,0x14,0x22,0x41};
  static constexpr uint8_t O[5] = {0x3e,0x41,0x41,0x41,0x3e};
  static constexpr uint8_t R[5] = {0x7f,0x09,0x19,0x29,0x46};
  static constexpr uint8_t T[5] = {0x01,0x01,0x7f,0x01,0x01};
  static constexpr uint8_t L[5] = {0x7f,0x40,0x40,0x40,0x40};
  static constexpr uint8_t I[5] = {0x41,0x41,0x7f,0x41,0x41};
  static constexpr uint8_t N[5] = {0x7f,0x02,0x0c,0x10,0x7f};
  static constexpr uint8_t J[5] = {0x20,0x40,0x40,0x3f,0x01};
  static constexpr uint8_t E[5] = {0x7f,0x49,0x49,0x49,0x41};
  latin(K); latin(O); latin(R); latin(T); latin(L);
  latin(I); latin(N); latin(J); latin(E, 5);

  static constexpr const char *KA[] = {".#...","#####",".#..#",".#..#",".#..#",".#..#","#..#."};
  static constexpr const char *NN[] = {".....","##...",".....","....#","....#","...#.","###.."};
  static constexpr const char *ZE[] = {".......",".#.#.#.","#####..",".#..#..",".#.#...",".#.....",".#.....","..###..","......."};
  static constexpr const char *NI[] = {".....",".....",".###.",".....",".....",".....",".....","#####","....."};
  static constexpr const char *RI[] = {"#..#.","#..#.","#..#.","#..#.","...#.","..#..",".#..."};
  static constexpr const char *II[] = {"....#","...#.","..#..",".##..","#.#..","..#..","..#.."};
  static constexpr const char *SHI[] = {"##...",".....","##..#","....#","....#","...#.","###.."};
  static constexpr const char *TA[] = {".####",".#..#","#.#.#","...##","...#.","..#..","##..."};
  static constexpr const char *PERIOD[] = {"..","..","..","..","..","##","##"};
  kana(KA,7,5); kana(NN,7,5); kana(ZE,9,7,0); kana(NN,7,5);
  kana(NI,9,5,0); kana(RI,7,5); kana(KA,7,5); kana(II,7,5);
  kana(SHI,7,5); kana(TA,7,5); kana(PERIOD,7,2);
}

inline void draw(IkeaMatrix::Frame &frame, int left) {
  frame.clear();
  for (int x = 0; x < IkeaMatrix::WIDTH; ++x) {
    int source = x - left;
    if (source < 0 || source >= length) continue;
    uint16_t bits = columns[source];
    for (int y = 0; y < IkeaMatrix::HEIGHT; ++y)
      if (bits & (uint16_t(1) << y)) frame.drawPixel(x, y);
  }
}
} // namespace MessageText
