#pragma once

namespace potential::stats {

};

#define ADD_TO_STATS(Field) namespace potential::stats { static size_t Field = 0; }

#define C(Field) do { potential::stats::Field++; } while (0)
