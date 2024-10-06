#pragma once

#include <chrono>

namespace potential::stats {
};

#define ADD_TO_STATS(Field) namespace potential::stats { static size_t Field = 0; }
#define ADD_TIME_TO_STATS(Field)                                     \
  namespace potential::stats {                                       \
    static std::chrono::high_resolution_clock::time_point Field;               \
  }

#define C(Field) do { potential::stats::Field++; } while (0)
#define START_TIME(Field) do {                                          \
    potential::stats::Field -= std::chrono::high_resolution_clock::now ().time_since_epoch (); \
  } while (0)

#define STOP_TIME(Field) do {                                           \
    potential::stats::Field += std::chrono::high_resolution_clock::now ().time_since_epoch (); \
  } while (0)

#define GET_TIME(Field) duration_cast<std::chrono::milliseconds> (potential::stats::Field.time_since_epoch ())
