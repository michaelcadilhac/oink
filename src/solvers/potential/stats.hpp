#pragma once

#include <chrono>

namespace potential::stats {
  struct stat_clearer {
      static inline std::vector<std::pair<void*, size_t>> fields;
      stat_clearer (void* p, size_t s) { fields.emplace_back (p, s); }
      static void clear_stats () {
        for (auto [p, s] : fields) { std::memset (p, 0, s); }
      }
  };
};

#define ADD_TO_STATS(Field)                                             \
  namespace potential::stats {                                          \
    static size_t Field = 0;                                            \
    static stat_clearer clear_ ##Field {&Field, sizeof (Field)};        \
  }

#define ADD_TIME_TO_STATS(Field)                                        \
  namespace potential::stats {                                          \
    static std::chrono::high_resolution_clock::time_point Field;        \
    static stat_clearer clear_ ##Field {&Field, sizeof (Field)};        \
  }

#define C(Field) do { potential::stats::Field++; } while (0)
#define START_TIME(Field) do {                                          \
    potential::stats::Field -= std::chrono::high_resolution_clock::now ().time_since_epoch (); \
  } while (0)

#define STOP_TIME(Field) do {                                           \
    potential::stats::Field += std::chrono::high_resolution_clock::now ().time_since_epoch (); \
  } while (0)

#define GET_TIME(Field) duration_cast<std::chrono::milliseconds> (potential::stats::Field.time_since_epoch ())
