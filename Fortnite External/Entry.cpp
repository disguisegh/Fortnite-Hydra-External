#include <iostream>
#include <windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>

#include "Driver/driver.hpp"
#include "Menu/overlay.h"

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

extern FILE* g_log;
extern void LogPrintf(const char* fmt, ...);

static void SetConsoleColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

static void ExitWithError(const char* message, int code = 0) {
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::cout << "\n[-] Error: " << message;
    if (code != 0) {
        std::cout << " [E" << std::hex << std::uppercase << code << std::dec << "]";
    }
    std::cout << "\n";
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::cout << "\nPress any key to exit..." << std::flush;
    system("pause >nul");
    exit(1);
}

static bool IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

static void WaitForDiscordOverlayWindow() {
    LogPrintf("[*] Waiting for Discord Overlay window...\n");
    HWND hOverlay = nullptr;
    int attempts = 0;
    constexpr char spinner[] = "|/-\\";
    while (!hOverlay && attempts < 300) {
        hOverlay = FindWindowA("Chrome_WidgetWin_1", "Discord Overlay");
        if (!hOverlay) {
            if (attempts % 10 == 0) {
                std::cout << '\r' << "[*] Waiting for Discord Overlay... "
                          << spinner[(attempts / 10) % 4] << std::flush;
            }
            Sleep(100);
            attempts++;
        }
    }
    std::cout << '\r' << "                                          " << '\r' << std::flush;
    if (!hOverlay) {
        ExitWithError(
            "Discord Overlay window not found.\n"
            "    Enable Discord in-game overlay, disable legacy overlay,\n"
            "    and open Fortnite so the overlay window exists.", 0x20);
    }
    LogPrintf("[+] Discord Overlay window detected.\n");
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleA("hydra");

    {
        char path[MAX_PATH]{};
        ExpandEnvironmentStringsA("%TEMP%\\hydra.log", path, MAX_PATH);
        fopen_s(&g_log, path, "w");
        if (g_log) setvbuf(g_log, nullptr, _IONBF, 0);
    }

    std::cout << "\n";
    LogPrintf("[*] hydra\n");
    std::cout << "\n";

    if (!IsAdmin()) {
        ExitWithError("Administrator privileges required.", 0x10);
    }

    LogPrintf("[*] Connecting to driver...\n");
    if (!driver::vulnerable()->s_dgx().get_export()) {
        ExitWithError("Driver not ready.", 0x3A);
    }
    LogPrintf("[+] Driver connected\n");

    while (!IsProcessRunning(L"Discord.exe")) {
        LogPrintf("[*] Waiting for Discord.exe...\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LogPrintf("[+] Discord detected\n");

    while (!driver::vulnerable.proc_id) {
        LogPrintf("[*] Waiting for FortniteClient-Win64-Shipping.exe...\n");
        driver::vulnerable.proc_id =
            driver::vulnerable()->exported_functions().get_process_id(
                "FortniteClient-Win64-Shipping.exe");
        if (!driver::vulnerable.proc_id)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LogPrintf("[+] Attached PID %llu\n",
              static_cast<unsigned long long>(driver::vulnerable.proc_id));

    driver::vulnerable.base_address =
        driver::vulnerable()->exported_functions().retrieve_image_base(0);
    base_address = static_cast<uintptr_t>(driver::vulnerable.base_address);

    LogPrintf("[+] Process base 0x%llX  CR3 0x%llX\n",
              static_cast<unsigned long long>(base_address),
              static_cast<unsigned long long>(g_driver ? g_driver->GetCr3() : 0));

    if (!base_address || !g_driver || !g_driver->is_attached()) {
        ExitWithError("Attach failed — no base / not attached.", 0x41);
    }

    WaitForDiscordOverlayWindow();

    LogPrintf("[*] Launching overlay (console stays open for errors)...\n");

    Overlay::BeginOverlay();

    LogPrintf("[*] Overlay exited\n");
    std::cout << "\n[*] Overlay ended. Press any key...\n" << std::flush;
    system("pause >nul");
    if (g_log) {
        fclose(g_log);
        g_log = nullptr;
    }
    return 0;
}
