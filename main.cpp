#include "shared_state.h"

typedef shared_state f_UpdateDraw(shared_state sharedState);

#pragma warning(push, 0)
#include <windows.h>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#pragma warning(pop)
{
    HMODULE codeLib = NULL;
    f_UpdateDraw* UpdateDraw = NULL;
    b32 prevLockFileExists = 0;
    b32 lockFileExists = 0;
    b32 loadCode = true;
    shared_state sharedState = {};
    //TODO: hot Reload on shader delta
    //possible solution:
    // file watchlist
    // header file containts an array of strings which
    // will be looped through each game loop and checked if changed
    // if any have changed, reload the game
    while (!sharedState.quit) {
        prevLockFileExists = lockFileExists;
        lockFileExists = (GetFileAttributesA("lock.tmp") != INVALID_FILE_ATTRIBUTES);
        if ((!lockFileExists && prevLockFileExists) || loadCode) {
            if (loadCode) {
                if (codeLib) {
                    FreeLibrary(codeLib);
                }
                if (CopyFile("code.dll", "code_temp.dll", FALSE)) {
                    codeLib = LoadLibraryA("code_temp.dll");
                    if (codeLib) {
                        if (loadCode) {
                            UpdateDraw = (f_UpdateDraw*)GetProcAddress(codeLib, "UpdateDraw");
                            if (UpdateDraw) {
                                loadCode = false;
                            }
                        }
                    }
                }
            } else {
                sharedState.releaseEverything = true;
                loadCode = true;
            }
        }
        sharedState = UpdateDraw(sharedState);
    }
    FreeLibrary(codeLib);
    DeleteFileA("code_temp.dll");
    return(0);
}