#include "game/game.h"
#include "core/hooks.h"
#include "game/subtick.h"

#include <Windows.h>
#include <MinHook.h>

static DWORD WINAPI mainthread(LPVOID param) {
    HMODULE self = static_cast<HMODULE>(param);

    MH_Initialize();
    for (int i = 0; i < 600 && !GetModuleHandleA("client.dll"); i++)
        Sleep(100);

    game::init();
    subtick::init();
    hooks::init();

    while (!(GetAsyncKeyState(VK_END) & 0x8000))
        Sleep(50);

    hooks::shutdown();
    Sleep(100);
    MH_Uninitialize();
    FreeLibraryAndExitThread(self, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);

        HANDLE thread = CreateThread(nullptr, 0, mainthread, instance, 0, nullptr);
        if (thread)
            CloseHandle(thread);
    }
    return true;
}
