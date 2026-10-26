#pragma once

#include "usercmd.h"

namespace subtick {

bool init();
bool ready();
bool tap(usercmd* cmd, float forward, float side);

} // namespace subtick
