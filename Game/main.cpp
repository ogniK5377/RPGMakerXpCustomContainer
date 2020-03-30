#include <array>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <Windows.h>
#include <psapi.h>
#include "Common.h"
#include "EnginePatches.h"
#include "MemoryUtil.h"
#include "SigScanner.h"

constexpr bool PATCH_KEY_AND_HEADER = false;
constexpr bool PATCH_CUSTOM_MODULES = false;

using RGSSInitializeProc = void (*)(HINSTANCE);
using RGSSFinalizeProc = void (*)(void);
using RGSSGameMainProc = int (*)(HWND, const char*, const char*);
using RGSSEvalProc = void (*)(const char*);
using RGSSSetupRTPProc = bool (*)(const char*, char*, int);

RGSSInitializeProc RGSSInitialize = nullptr;
RGSSFinalizeProc RGSSFinalize = nullptr;
RGSSGameMainProc RGSSGameMain = nullptr;
RGSSEvalProc RGSSEval = nullptr;
RGSSSetupRTPProc RGSSSetupRTP = nullptr;

bool GetRGSSExports(HMODULE rgssad) {
    RGSSInitialize =
        MemoryUtil::MakeCallable<RGSSInitializeProc>(GetProcAddress(rgssad, "RGSSInitialize"));
    RGSSFinalize =
        MemoryUtil::MakeCallable<RGSSFinalizeProc>(GetProcAddress(rgssad, "RGSSFinalize"));
    RGSSGameMain =
        MemoryUtil::MakeCallable<RGSSGameMainProc>(GetProcAddress(rgssad, "RGSSGameMain"));
    RGSSEval = MemoryUtil::MakeCallable<RGSSEvalProc>(GetProcAddress(rgssad, "RGSSEval"));
    RGSSSetupRTP =
        MemoryUtil::MakeCallable<RGSSSetupRTPProc>(GetProcAddress(rgssad, "RGSSSetupRTP"));

    return RGSSInitialize != nullptr && RGSSFinalize != nullptr && RGSSGameMain != nullptr &&
           RGSSEval != nullptr && RGSSSetupRTP != nullptr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Get the games launch directory
    char launch_directory[MAX_PATH];
    GetModuleFileName(NULL, launch_directory, MAX_PATH);

    char* last_dir = strrchr(launch_directory, '\\');
    if (!last_dir) {
        LOG("Bad launch directory");
        return 1;
    }
    last_dir += 1;
    launch_directory[last_dir - launch_directory] = 0;

    // Setup consts for paths
    const std::string game_directory(launch_directory);
    const std::string game_config(game_directory + "\\Game.ini");

    // Set the current working directory
    SetCurrentDirectory(game_directory.c_str());

    // Game.ini does not exist
    if (GetFileAttributes(game_config.c_str()) == INVALID_FILE_ATTRIBUTES) {
        LOG("Failed to find Game.ini");
        return 1;
    }

    // Read Game.ini
    char library_path[MAX_PATH];
    char game_title[0x200];
    char script_path[0x200];

    char rgssad_path[0x200];
    char dll_path[0x200];
    char cache_file[0x200];

    GetPrivateProfileString("Game", "Library", NULL, library_path, MAX_PATH, game_config.c_str());
    GetPrivateProfileString("Game", "Title", "RGSSAD Container", game_title, 0x200,
                            game_config.c_str());
    GetPrivateProfileString("Game", "Scripts", NULL, script_path, 0x200, game_config.c_str());

    // Custom properties

    GetPrivateProfileString("Container", "Dlls", "", dll_path, 0x200, game_config.c_str());
    GetPrivateProfileString("Container", "RGSSAD", "Game.rgssad", rgssad_path, 0x200,
                            game_config.c_str());
    GetPrivateProfileString("Container", "SignatureCache", "sig_cache.che", cache_file, 0x200,
                            game_config.c_str());

    const auto fast_boot =
        GetPrivateProfileInt("Container", "FastBoot", 0, game_config.c_str()) > 0;
    const auto allow_debugger =
        GetPrivateProfileInt("Container", "AllowDebugger", 0, game_config.c_str()) > 0;

    if (allow_debugger) {
        Patches::PatchDebugPresent();
    }

    // With the custom container, we can reroute where DLLs are stored, this lets us cleanup the
    // base game directory.
    const std::string base_dll_path(game_directory + "\\" + dll_path); // New DLL load directory
    SetDllDirectory(base_dll_path.c_str());

    std::string RGSSAD(game_directory + "\\" + rgssad_path);
    if (GetFileAttributes(RGSSAD.c_str()) == INVALID_FILE_ATTRIBUTES) {
        RGSSAD = "";
    }

    // Create RGSS Player window class
    WNDCLASS window_class{};
    window_class.style = CS_DBLCLKS;
    window_class.lpfnWndProc = DefWindowProc;
    window_class.hInstance = hInstance;
    window_class.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(32512)); // TODO(ogniK): Swap icon
    window_class.hCursor = LoadCursor(0, MAKEINTRESOURCE(32512));
    window_class.lpszClassName = "RGSS Player";
    RegisterClass(&window_class);

    // Create the window
    const HMODULE module_handle = GetModuleHandle(NULL);
    const HWND window =
        CreateWindowEx(WS_EX_WINDOWEDGE, window_class.lpszClassName, game_title,
                       WS_GROUP | WS_SYSMENU | WS_DLGFRAME | WS_BORDER, CW_USEDEFAULT,
                       CW_USEDEFAULT, 0, 0, NULL, NULL, module_handle, NULL);
    if (!window) {
        LOG("Failed to create window");
        return 1;
    }
    // Center RGSS Player window to the center of the screen
    RECT window_rect{0, 0, 640, 480};
    const auto window_styles = GetWindowLongA(window, GWL_STYLE);
    const auto ex_window_styles = GetWindowLongA(window, GWL_EXSTYLE);
    AdjustWindowRectEx(&window_rect, window_styles, NULL, ex_window_styles);

    const auto window_padding_width = 2 * GetSystemMetrics(SM_CXFRAME);
    const auto window_padding_height = 2 * GetSystemMetrics(SM_CYFRAME);

    const auto window_maximized_width = GetSystemMetrics(SM_CXMAXIMIZED);
    const auto window_maximized_height = GetSystemMetrics(SM_CYMAXIMIZED);

    const auto center_width = window_maximized_width - window_padding_width;
    const auto center_height = window_maximized_height - window_padding_height;

    const auto x_pos = (center_width - (window_rect.right - window_rect.left)) / 2;
    const auto y_pos = (center_height - (window_rect.bottom - window_rect.top)) / 2;

    SetWindowPos(window, NULL, x_pos, y_pos, window_rect.right - window_rect.left,
                 window_rect.bottom - window_rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(window, SW_SHOW);

    // Fill the default screen color to be black
    auto dc = GetDC(window);
    FillRect(dc, &window_rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    ReleaseDC(window, dc);
    MSG message{};

    if (!fast_boot) {
        auto tick_count = GetTickCount64();
        while (GetTickCount64() - tick_count < 500) {
            if (PeekMessage(&message, NULL, NULL, NULL, NULL)) {
                if (!GetMessage(&message, NULL, NULL, NULL)) {
                    return 1;
                }
                DispatchMessage(&message);
            }
        }
    }

    // Load the RPG make rgss dll
    const auto rgssad_library = LoadLibrary(library_path);
    if (!rgssad_library) {
        const auto error_code = GetLastError();
        // Failed to load RGSS DLL
        if (error_code == ERROR_MOD_NOT_FOUND) {
            LOG("Missing RGSS DLL! Please re-download the game");
        } else {
            const std::string error("An unknown error has occurred with the error code " +
                                    GetLastError());
            LOG(error.c_str());
        }
        return 1;
    }

    // If we want to patch the rgssad, start patching
    if (PATCH_KEY_AND_HEADER) {
        Patches::SwapRgssadEncryption(library_path);
    }

    // RGSSAD Level patches
    if (PATCH_CUSTOM_MODULES) {
        Patches::SetupDetours(library_path);
    }

    Patches::PatchiBindings(library_path);

    // Get the exports from the RGSS dll
    if (!GetRGSSExports(rgssad_library)) {
        LOG("Failed to load RGSSAD library!");
        return 1;
    }

    char rtp_error[0x400];
    // Setup the RTP
    if (!RGSSSetupRTP(game_config.c_str(), rtp_error, 0x400)) {
        LOG("Got RTP load error!");
        LOG(rtp_error);
        return 1;
    }

    // Initialize the rgss dll
    RGSSInitialize(rgssad_library);

    // Enable debug mode if we're running from RPG Makers editor or pass the correct command line
    if (!memcpy(lpCmdLine, "btest", 6)) {
        RGSSEval("$DEBUG = true");
        RGSSEval("$BTEST = true");
    } else {
        if (!memcmp(lpCmdLine, "debug", 6)) {
            RGSSEval("$DEBUG = true");
        }
        RGSSEval("$BTEST = false");
    }

    // Execute main game logic
    return RGSSGameMain(window, script_path, RGSSAD.c_str());
}
