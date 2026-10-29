#include "game/game.h"
#include "utils/memory.h"
#include "game/offsets.h"
#include "utils/pattern.h"

namespace game {

using createinterfacefn = void*(*)(const char* name, int* code);
using boolfn = bool(*)(void* self);

static u64 client = 0;
static u64 pawnoffset = 0;

bool init() {
    HMODULE dll = GetModuleHandleA("client.dll");
    if (!dll)
        return false;
    client = reinterpret_cast<u64>(dll);

    u64 prediction = findpattern(dll, patterns::pawnbase);
    u64 pawnhit = findpattern(dll, patterns::pawndelta);
    if (!prediction || !pawnhit)
        return false;

    int delta = *reinterpret_cast<int*>(pawnhit + 3);
    pawnoffset = resolverip(prediction, 3, 7) - client + delta;
    return true;
}

u64 localpawn() {
    if (!pawnoffset)
        return 0;
    return readsafe<u64>(client + pawnoffset);
}

bool onground(u64 pawn) {
    if (!pawn)
        return false;
    return readsafe<u32>(pawn + offsets::pawn::flags) & 1;
}

u64 services(u64 pawn) {
    if (!pawn)
        return 0;
    return readsafe<u64>(pawn + offsets::pawn::movementservices);
}

static void* inputsystem() {
    static void* cached = nullptr;
    if (cached)
        return cached;

    HMODULE dll = GetModuleHandleA("inputsystem.dll");
    if (!dll)
        return nullptr;

    auto createinterface = reinterpret_cast<createinterfacefn>(GetProcAddress(dll, "CreateInterface"));
    if (!createinterface)
        return nullptr;

    cached = createinterface("InputSystemVersion001", nullptr);
    return cached;
}

static bool callbool(int index) {
    void* system = inputsystem();
    if (!system)
        return false;

    __try {
        void** vtable = *reinterpret_cast<void***>(system);
        if (!vtable)
            return false;
        return reinterpret_cast<boolfn>(vtable[index])(system);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool inputactive() {
    return callbool(offsets::inputsystem::windowactive) && callbool(offsets::inputsystem::mousefocus);
}

} // namespace game
