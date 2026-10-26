#include "subtick.h"
#include "game.h"
#include "memory.h"
#include "pattern.h"

#include <algorithm>
#include <cmath>

namespace subtick {

constexpr int maxsteps = 32;
constexpr u64 heldbuttons = 0x58;
constexpr u64 cmdforward = 0x1a0;
constexpr u64 cmdleft = 0x1a4;

using allocfn = subtickstep*(*)(void* arena);
using addfn = subtickstep*(*)(repeatedfield<subtickstep>* field, subtickstep* step);

static allocfn allocstep = nullptr;
static addfn addstep = nullptr;
static subtickstep* prototype = nullptr;
static subtickstep spare[64]{};
static int spareindex = 0;

bool init() {
    HMODULE client = GetModuleHandleA("client.dll");
    if (!client)
        return false;

    u64 call = findpattern(client, "e8 ? ? ? ? 48 8b d0 48 8b ce e8 ? ? ? ? 48 8b c8");
    if (!call)
        return false;

    u64 alloc = resolverip(call, 1, 5);
    u64 add = resolverip(call, 12, 16);
    if (inrange(alloc, 1))
        allocstep = reinterpret_cast<allocfn>(alloc);
    if (inrange(add, 1))
        addstep = reinterpret_cast<addfn>(add);
    return ready();
}

bool ready() {
    return allocstep && addstep;
}

static bool validstep(subtickstep* step) {
    if (!inrange(reinterpret_cast<u64>(step), sizeof(subtickstep)))
        return false;

    __try {
        return reinterpret_cast<u64>(step->vtable) > 0x10000;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static subtickstep* allocate(void* arena) {
    if (!allocstep)
        return nullptr;

    __try {
        return allocstep(arena);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

static subtickstep* append(repeatedfield<subtickstep>* field, subtickstep* step) {
    if (!addstep || !field || !step)
        return nullptr;

    __try {
        return addstep(field, step);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

static subtickstep* sparestep() {
    if (!prototype)
        prototype = allocate(nullptr);
    if (!prototype)
        return nullptr;

    subtickstep* step = &spare[spareindex++ & 63];
    __try {
        *step = *prototype;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
    return step;
}

static void invalidate(basecmd* base) {
    if (!inrange(reinterpret_cast<u64>(&base->bits), sizeof(u32)))
        return;

    __try {
        base->bits &= ~0x40u;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

static subtickstep* takeslot(repeatedfield<subtickstep>& field) {
    __try {
        int index = field.size;
        if (index < 0 || index >= maxsteps)
            return nullptr;

        if (field.capacity > index && field.data && index < field.data->allocated) {
            subtickstep* step = field.data->elements[index];
            if (!validstep(step)) {
                step = allocate(field.arena);
                if (!validstep(step))
                    step = sparestep();
                if (!validstep(step))
                    return nullptr;
                field.data->elements[index] = step;
            }
            field.size = index + 1;
            return step;
        }

        subtickstep* created = allocate(field.arena);
        if (!validstep(created))
            created = sparestep();
        if (!validstep(created))
            return nullptr;

        subtickstep* step = append(&field, created);
        if (!validstep(step))
            step = created;

        if (field.size > index)
            return step;

        if (!field.data || index >= field.capacity)
            return nullptr;

        if (field.data->allocated <= index)
            field.data->allocated = index + 1;
        field.data->elements[index] = step;
        field.size = index + 1;
        return step;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

static void removemovement(basecmd* base) {
    if (!inrange(reinterpret_cast<u64>(base), sizeof(basecmd)))
        return;

    __try {
        auto& moves = base->moves;
        if (moves.size <= 0 || !moves.data)
            return;

        int count = std::min(moves.size, maxsteps);
        subtickstep* keep[maxsteps]{};
        subtickstep* removed[maxsteps]{};
        int keepcount = 0;
        int removedcount = 0;

        for (int i = 0; i < count; i++) {
            subtickstep* step = moves.data->elements[i];
            if (!validstep(step))
                continue;

            bool buttonmove = (step->button & inmove) != 0;
            bool analogmove = step->button == 0 && (step->bits & 0x18) != 0;
            if (!buttonmove && !analogmove) {
                keep[keepcount++] = step;
                continue;
            }

            u64 remaining = step->button & ~inmove;
            if (remaining) {
                step->button = remaining;
                keep[keepcount++] = step;
            } else {
                removed[removedcount++] = step;
            }
        }

        int write = 0;
        for (int i = 0; i < keepcount; i++)
            moves.data->elements[write++] = keep[i];
        for (int i = 0; i < removedcount; i++)
            moves.data->elements[write++] = removed[i];
        moves.size = keepcount;
        invalidate(base);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

static void restorecount(basecmd* base, int count) {
    if (!inrange(reinterpret_cast<u64>(base), sizeof(basecmd)))
        return;

    __try {
        base->moves.size = std::clamp(count, 0, maxsteps);
        invalidate(base);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

static bool addanalog(basecmd* base, float when, float forward, float left) {
    if (!inrange(reinterpret_cast<u64>(base), sizeof(basecmd)))
        return false;

    subtickstep* step = takeslot(base->moves);
    if (!validstep(step))
        return false;

    __try {
        step->bits = 0x4;
        step->button = 0;
        step->pressed = false;
        step->when = std::clamp(when, 0.001f, 0.999f);
        step->forward = forward;
        step->left = left;
        step->pitch = 0.0f;
        step->yaw = 0.0f;
        if (std::fabs(forward) > 0.000001f)
            step->bits |= 0x8;
        if (std::fabs(left) > 0.000001f)
            step->bits |= 0x10;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    invalidate(base);
    return true;
}

static void sortsteps(basecmd* base) {
    if (!inrange(reinterpret_cast<u64>(base), sizeof(basecmd)))
        return;

    __try {
        auto& moves = base->moves;
        if (!moves.data || moves.size <= 1)
            return;

        int count = std::min(moves.size, maxsteps);
        std::stable_sort(moves.data->elements, moves.data->elements + count, [](subtickstep* a, subtickstep* b) {
            bool avalid = validstep(a);
            bool bvalid = validstep(b);
            if (avalid != bvalid)
                return avalid;
            if (!avalid)
                return false;
            if (a->when != b->when)
                return a->when < b->when;
            if (a->pressed != b->pressed)
                return !a->pressed;
            return a->button < b->button;
        });
        invalidate(base);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

static u64 cmdbuttons(usercmd* cmd) {
    __try {
        return cmd->buttons.value;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

bool tap(usercmd* cmd, float forward, float side) {
    if (!cmd)
        return false;

    basecmd* base = cmd->pb.base;
    if (!inrange(reinterpret_cast<u64>(base), sizeof(basecmd)))
        return false;

    u64 services = game::services(game::localpawn());
    if (!services)
        return false;

    u64 held = 0;
    float startforward = 0.0f;
    float startside = 0.0f;
    __try {
        held = *reinterpret_cast<u64*>(services + heldbuttons);
        if (held & (inforward | inback))
            startforward = *reinterpret_cast<float*>(services + cmdforward);
        if (held & (inmoveleft | inmoveright))
            startside = *reinterpret_cast<float*>(services + cmdleft);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if (!std::isfinite(startforward))
        startforward = 0.0f;
    if (!std::isfinite(startside))
        startside = 0.0f;

    u64 now = cmdbuttons(cmd);
    float endforward = ((now & inforward) ? 1.0f : 0.0f) - ((now & inback) ? 1.0f : 0.0f);
    float endside = ((now & inmoveleft) ? 1.0f : 0.0f) - ((now & inmoveright) ? 1.0f : 0.0f);

    removemovement(base);
    int kept = 0;
    __try {
        kept = base->moves.size;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    bool ok = true;
    float toforward = forward - startforward;
    float toside = side - startside;
    if (toforward != 0.0f || toside != 0.0f)
        ok = addanalog(base, 0.0f, toforward, toside);

    float backforward = endforward - forward;
    float backside = endside - side;
    if (ok && (backforward != 0.0f || backside != 0.0f))
        ok = addanalog(base, 0.999f, backforward, backside);

    if (!ok) {
        restorecount(base, kept);
        return false;
    }
    sortsteps(base);

    __try {
        base->forwardmove = endforward;
        base->leftmove = endside;
        base->bits |= 0x60;
        invalidate(base);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return true;
}

} // namespace subtick
