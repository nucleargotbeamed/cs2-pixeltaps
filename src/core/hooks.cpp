#include "core/hooks.h"
#include "utils/memory.h"
#include "game/offsets.h"
#include "utils/pattern.h"
#include "features/pixeltap.h"
#include "game/usercmd.h"

#include <MinHook.h>

#include <atomic>
#include <cmath>

namespace hooks {

using createmovefn = void(*)(csgoinput* input, int slot, usercmd* cmd);

static createmovefn original = nullptr;
static void* target = nullptr;
static std::atomic<bool> stopping = false;

static bool validinput(csgoinput* input) {
    if (!input || !inrange(reinterpret_cast<u64>(&input->subtickcount), sizeof(int)))
        return false;

    __try {
        if (input->subtickcount < 0 || input->subtickcount > 64)
            return false;
        vec3 angles = input->viewangles;
        return std::isfinite(angles.x) && std::isfinite(angles.y) && std::isfinite(angles.z);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void hookedcreatemove(csgoinput* input, int slot, usercmd* cmd) {
    original(input, slot, cmd);

    if (stopping || !cmd || !validinput(input))
        return;

    static int lastsequence = -1;
    int sequence = -1;
    __try {
        sequence = cmd->sequence;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    bool fresh = sequence != lastsequence;
    lastsequence = sequence;

    __try {
        pixeltap::run(cmd, fresh);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

bool init() {
    HMODULE client = GetModuleHandleA("client.dll");
    if (!client)
        return false;

    u64 address = findpattern(client, patterns::createmove);
    if (!address)
        return false;

    target = reinterpret_cast<void*>(address);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&hookedcreatemove), reinterpret_cast<void**>(&original)) != MH_OK
        || MH_EnableHook(target) != MH_OK) {
        target = nullptr;
        return false;
    }
    return true;
}

void shutdown() {
    stopping = true;
    if (!target)
        return;

    MH_DisableHook(target);
    Sleep(100);
    MH_RemoveHook(target);
    target = nullptr;
}

} // namespace hooks
