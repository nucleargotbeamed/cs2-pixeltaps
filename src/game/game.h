#pragma once

#include "utils/types.h"

namespace game {

bool init();
u64 localpawn();
bool onground(u64 pawn);
u64 services(u64 pawn);
bool inputactive();

} // namespace game
