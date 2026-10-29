#pragma once

#include "utils/types.h"

namespace patterns {
constexpr const char* createmove = "48 8b c4 4c 89 40 ? 48 89 48 ? 55 53 57";
constexpr const char* subtick = "e8 ? ? ? ? 48 8b d0 48 8b ce e8 ? ? ? ? 48 8b c8";
constexpr const char* pawnbase = "48 8d 05 ? ? ? ? c3 cc cc cc cc cc cc cc cc 40 53 56 41 54";
constexpr const char* pawndelta = "4c 39 b6 ? ? ? ? 74 ? 44 88 be";
} // namespace patterns

namespace offsets {

namespace pawn {
constexpr u64 flags = 0x3f4;
constexpr u64 movementservices = 0x1330;
} // namespace pawn

namespace movement {
constexpr u64 buttons = 0x58;
constexpr u64 cmdforward = 0x1a0;
constexpr u64 cmdleft = 0x1a4;
} // namespace movement

namespace input {
constexpr u64 subtickcount = 0x284;
constexpr u64 viewangles = 0x688;
} // namespace input

namespace inputsystem {
constexpr int windowactive = 68;
constexpr int mousefocus = 77;
} // namespace inputsystem

} // namespace offsets
