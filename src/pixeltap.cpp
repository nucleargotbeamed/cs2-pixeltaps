#include "pixeltap.h"
#include "game.h"
#include "subtick.h"

#include <Windows.h>
#include <algorithm>

namespace pixeltap {

struct bind {
    int key;
    float forward;
    float side;
    bool wasdown;
    bool active;
};

static bind binds[] = {
    {VK_UP, 1.0f, 0.0f, false, false},
    {VK_LEFT, 0.0f, 1.0f, false, false},
    {VK_DOWN, -1.0f, 0.0f, false, false},
    {VK_RIGHT, 0.0f, -1.0f, false, false},
};

void run(usercmd* cmd, bool fresh) {
    if (!cmd)
        return;

    u64 pawn = game::localpawn();
    bool canemit = pawn && game::onground(pawn) && subtick::ready();
    bool focused = game::inputactive();

    bool any = false;
    float forward = 0.0f;
    float side = 0.0f;

    for (bind& b : binds) {
        if (fresh)
            b.active = false;

        bool down = focused && (GetAsyncKeyState(b.key) & 0x8000);
        bool pressed = down && !b.wasdown;
        b.wasdown = down;

        if (pressed && !b.active && canemit)
            b.active = true;

        if (b.active) {
            forward += b.forward;
            side += b.side;
            any = true;
        }
    }

    if (!any)
        return;

    forward = std::clamp(forward, -1.0f, 1.0f);
    side = std::clamp(side, -1.0f, 1.0f);
    if (forward == 0.0f && side == 0.0f)
        return;

    subtick::tap(cmd, forward, side);
}

} // namespace pixeltap
