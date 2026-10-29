#pragma once

#include "utils/types.h"

#include <Windows.h>

inline bool inrange(u64 address, u64 size) {
    return address >= 0x10000 && size != 0 && address + size >= address && address + size <= 0x7fffffffffff;
}

template <class t>
t readsafe(u64 address) {
    if (!inrange(address, sizeof(t)))
        return t{};

    __try {
        return *reinterpret_cast<t*>(address);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return t{};
    }
}
