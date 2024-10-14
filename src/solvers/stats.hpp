#pragma once

#include <cstring>
#include <chrono>

#ifndef NDEBUG

namespace stats {
  struct stat_clearer {
      static inline std::vector<std::pair<void*, size_t>> fields;
      stat_clearer (void* p, size_t s) { fields.emplace_back (p, s); }
      static void clear_stats () {
        for (auto [p, s] : fields) { std::memset (p, 0, s); }
      }
  };
};

# define CLEAR_STATS stats::stat_clearer::clear_stats ()

# define ADD_TO_STATS(Field)                                             \
  namespace stats {                                                     \
    static size_t Field = 0;                                            \
    static stat_clearer clear_ ##Field {&Field, sizeof (Field)};        \
  }

# define ADD_TIME_TO_STATS(Field)                                        \
  namespace stats {                                                     \
    static std::chrono::high_resolution_clock::time_point Field;        \
    static stat_clearer clear_ ##Field {&Field, sizeof (Field)};        \
  }

# define TICK(Field) do { stats::Field++; } while (0)
# define GET_STAT(Field) stats::Field

# define START_TIME(Field) do {                                          \
    stats::Field -= std::chrono::high_resolution_clock::now ().time_since_epoch (); \
  } while (0)

# define STOP_TIME(Field) do {                                           \
    stats::Field += std::chrono::high_resolution_clock::now ().time_since_epoch (); \
  } while (0)

# define GET_TIME(Field) duration_cast<std::chrono::milliseconds> (stats::Field.time_since_epoch ())

#else

# define CLEAR_STATS
# define ADD_TO_STATS(F)
# define ADD_TIME_TO_STATS(F)
# define TICK(F)
# define GET_STAT(F) 0
# define START_TIME(F)
# define STOP_TIME(F)
# define GET_TIME(F) 0

#endif
