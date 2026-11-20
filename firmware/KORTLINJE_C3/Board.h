#pragma once
// Select the target board. Exactly one; or pass -DKORTLINJE_BOARD_XIAO_C3 at build time.
#if !defined(KORTLINJE_BOARD_SUPERMINI) && !defined(KORTLINJE_BOARD_XIAO_C3)
#define KORTLINJE_BOARD_SUPERMINI
// #define KORTLINJE_BOARD_XIAO_C3
#endif

#if defined(KORTLINJE_BOARD_SUPERMINI)
#include "BoardSuperMini.h"
#elif defined(KORTLINJE_BOARD_XIAO_C3)
#include "BoardXiaoC3.h"
#endif
