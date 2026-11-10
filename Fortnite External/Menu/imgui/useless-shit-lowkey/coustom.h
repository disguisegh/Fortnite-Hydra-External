#pragma once
#include "../Menu/imgui/imgui.h"
#include "../Menu/imgui/imgui_internal.h"

#include <iostream>
#include <windows.h>
#include <vcruntime.h>
#include <tlhelp32.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <winternl.h>
#include <thread>
#include <vector>
#include <array>
#include <WinInet.h>
#include <urlmon.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Wininet.lib")

using namespace std;

#define FN_OFFSET ptrdiff_t
#define FN_POINTER uintptr_t
#define FN_TEXT string
#define FN_VECTOR Vector
#define FN_STORAGE_I int
#define FN_STORAGE_F float
#define FN_STORAGE_D double
#define BOPTION bool
#define FOPTION float
#define IOPTION int
#define C_FREEZE Sleep
#define TO_HEX hex
#define END_THIS endl
#define GET_KEY GetAsyncKeyState
#define __START CreateThread
#define RENDER_BACKGROUND ImGui::GetBackgroundDrawList
#define RENDER_FOREGROUND ImGui::GetForegroundDrawList
#define M_PI (double)3.1415926535

string RoleName = ("User");
ImVec4 LocalRoleColor = ImVec4(0, 255, 71, 255);

ImVec4 Color_User = ImVec4(0, 255, 71, 255);
ImVec4 Color_Tester = ImVec4(0, 255, 255, 255);
ImVec4 Color_Reseller = ImVec4(200, 0, 255, 255);
ImVec4 Color_Staff = ImVec4(0, 150, 210, 255);
ImVec4 Color_Admin = ImVec4(255, 0, 0, 255);
ImVec4 Color_Developer = ImVec4(0, 255, 255, 255);
ImVec4 Color_Owner = ImVec4(255, 135, 255, 255);

ID3D11Device* pDevice = nullptr;

extern int CenterX;
extern int CenterY;
extern float ScreenCenterX;
extern float ScreenCenterY;

BOPTION bWriteAllowed = true;
BOPTION bReadAllowed = true;
BOPTION bProtectAllowed = true;
BOPTION bExploitsUnprotected = false;
BOPTION bValidatePlayers = true;
BOPTION bDoPlayerChecks = true;
BOPTION bGetPlayerData = true;
BOPTION bDefinePointers = true;
BOPTION bGetCameraPosition = true;
BOPTION bGetCameraRotation = true;
BOPTION bGetCameraFOV = true;
BOPTION bIsMenuOpen = true;
BOPTION bExploitsEnabled = false;
BOPTION bRunCheat = false;
BOPTION bShowVisualButtons = true;
BOPTION bCheckOnScreen = true;

template<typename T2, typename T1>
inline T2 lexical_cast(const T1& in) {
	T2 out;
	std::stringstream ss;
	ss << in;
	ss >> out;
	return out;
}

template <typename ElemT>
struct HexTo {
	ElemT value;
	operator ElemT() const { return value; }
	friend std::istream& operator>>(std::istream& in, HexTo& out) {
		in >> std::hex >> out.value;
		return in;
	}
};

inline bool CleanCombo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.20f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.70f, 0.85f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.10f, 0.70f, 0.85f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.75f, 0.90f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.20f, 0.82f, 0.95f, 0.7f));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);

    bool result = ImGui::Combo(label, current_item, items, items_count, popup_max_height_in_items);

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(9);

    return result;
}

inline bool PurpleCombo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1) {
    return CleanCombo(label, current_item, items, items_count, popup_max_height_in_items);
}
