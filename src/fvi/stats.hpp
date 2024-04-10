#pragma once

namespace fvi_stats {

};

#define ADD_TO_STATS(Field) namespace fvi_stats { static size_t Field = 0; }

#define C(Field) do { fvi_stats::Field++; } while (0)
