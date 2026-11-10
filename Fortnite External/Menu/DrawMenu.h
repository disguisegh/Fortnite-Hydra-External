#pragma once
#include "../settings/settings.h"
#include "../Menu/imgui/useless-shit-lowkey/Helper.h"
#include "../Menu/imgui/useless-shit-lowkey/coustom.h"
#undef TEXT
#include "../Menu/imgui/useless-shit-lowkey/CheatEncryption.h"
#include "../Menu/Config.h"
#include "../../Game/Drawing/drawing.h"
#include <chrono>
#include <ctime>

namespace ConfigUI {
    inline bool syncBoneOnLoad = false;
}

static int AimbotPage = 0;

static float g_menuAnimTimer = 0.0f;
static float g_glowPulse = 0.0f;
static float g_menuOpenAnim = 0.0f;
static bool  g_menuWasOpen = false;

static int   g_lastMenuTab     = -1;
static int   g_lastVisualPage  = -1;
static int   g_lastSettingPage = -1;
static float g_tabFadeT        = 1.0f;
static const float g_tabFadeDuration = 0.18f;

struct RippleFx { float x; float y; float t; bool active; ImU32 color; };
static const int g_rippleMax = 6;
static RippleFx  g_ripples[g_rippleMax] = {};

static void PushRipple(float x, float y, ImU32 color = IM_COL32(142, 132, 227, 255)) {
    int slot = -1;
    for (int i = 0; i < g_rippleMax; i++) {
        if (!g_ripples[i].active) { slot = i; break; }
    }
    if (slot < 0) {
        slot = 0;
        for (int i = 1; i < g_rippleMax; i++)
            if (g_ripples[i].t > g_ripples[slot].t) slot = i;
    }
    g_ripples[slot].x = x;
    g_ripples[slot].y = y;
    g_ripples[slot].t = 0.0f;
    g_ripples[slot].active = true;
    g_ripples[slot].color = color;
}

static void UpdateAndDrawRipples(ImDrawList* dl, float dt) {
    if (!dl) return;
    for (int i = 0; i < g_rippleMax; i++) {
        if (!g_ripples[i].active) continue;
        g_ripples[i].t += dt / 0.45f;
        if (g_ripples[i].t >= 1.0f) {
            g_ripples[i].active = false;
            continue;
        }
        float t = g_ripples[i].t;
        float radius = 6.0f + t * 36.0f;
        int alpha = (int)((1.0f - t) * 95.0f);
        ImU32 c = (g_ripples[i].color & 0x00FFFFFF) | ((ImU32)alpha << 24);
        dl->AddCircle(mvec(g_ripples[i].x, g_ripples[i].y), radius, c, 32, 1.6f);
    }
}

static void RippleOnLastItemClick() {
    if (ImGui::IsItemClicked()) {
        mvec mn = ImGui::GetItemRectMin();
        mvec mx = ImGui::GetItemRectMax();
        PushRipple((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
    }
}

static void WithTooltip(const char* tip) {
    if (!tip || !*tip) return;
    if (!ImGui::IsItemHovered()) return;

    static float s_lastMinX = -1.0f, s_lastMinY = -1.0f;
    static ImGuiID s_lastWin = 0;
    static float s_hoverTimer = 0.0f;
    mvec mn = ImGui::GetItemRectMin();
    ImGuiID winId = ImGui::GetCurrentWindow() ? ImGui::GetCurrentWindow()->ID : 0;
    if (mn.x != s_lastMinX || mn.y != s_lastMinY || winId != s_lastWin) {
        s_lastMinX = mn.x;
        s_lastMinY = mn.y;
        s_lastWin   = winId;
        s_hoverTimer = 0.0f;
    }
    s_hoverTimer += ImGui::GetIO().DeltaTime;
    if (s_hoverTimer < 0.45f) return;

    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(27, 27, 38, 250));
    ImGui::PushStyleColor(ImGuiCol_Border,  IM_COL32(142, 132, 227, 180));
    ImGui::PushStyleColor(ImGuiCol_Text,    IM_COL32(232, 233, 242, 240));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    mvec(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   6.0f);
    ImGui::BeginTooltip();

    ImGui::PushFont(Fonts::SegoeRegular ? Fonts::SegoeRegular : (Fonts::SegoeSemi ? Fonts::SegoeSemi : Fonts::TahomaBold));
    ImGui::PushTextWrapPos(240.0f);
    ImGui::TextUnformatted(tip);
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
    ImGui::EndTooltip();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
}

static void DrawActiveTabUnderline(ImVec2 bmin, ImVec2 bmax) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float midX = (bmin.x + bmax.x) * 0.5f;
    dl->AddRectFilledMultiColor(
        mvec(bmin.x + 8, bmax.y + 1), mvec(midX, bmax.y + 3),
        IM_COL32(142, 132, 227, 0),   IM_COL32(142, 132, 227, 255),
        IM_COL32(142, 132, 227, 255), IM_COL32(142, 132, 227, 0));
    dl->AddRectFilledMultiColor(
        mvec(midX, bmax.y + 1), mvec(bmax.x - 8, bmax.y + 3),
        IM_COL32(142, 132, 227, 255), IM_COL32(142, 132, 227, 0),
        IM_COL32(142, 132, 227, 0),   IM_COL32(142, 132, 227, 255));
}

static float g_loadingTimer = 0.0f;
static const float g_loadingDuration = 2.8f;
static const float g_loadingFadeOut = 0.5f;
static bool  g_loadingDone = false;

bool LoadingScreen_IsActive()
{
    return !g_loadingDone;
}

void DrawLoadingScreen()
{
    ImGuiIO& io = ImGui::GetIO();
    g_loadingTimer += io.DeltaTime;

    float W = (float)(settings::CenterX * 2);
    float H = (float)(settings::CenterY * 2);
    float cx = W * 0.5f;
    float cy = H * 0.5f;

    float t = g_loadingTimer / g_loadingDuration;
    if (t > 1.0f) t = 1.0f;

    float alpha = 1.0f;
    if (g_loadingTimer < 0.3f) alpha = g_loadingTimer / 0.3f;
    float remaining = g_loadingDuration - g_loadingTimer;
    if (remaining < g_loadingFadeOut) alpha = remaining / g_loadingFadeOut;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    dl->AddRectFilledMultiColor(
        mvec(0, 0), mvec(W, H),
        IM_COL32(20, 20, 28, (int)(245 * alpha)),
        IM_COL32(20, 20, 28, (int)(245 * alpha)),
        IM_COL32(8, 8, 14, (int)(245 * alpha)),
        IM_COL32(8, 8, 14, (int)(245 * alpha)));

    float pulse = 0.5f + 0.5f * sinf(g_loadingTimer * 2.2f);
    for (int i = 0; i < 5; i++) {
        float r = 140.0f + i * 40.0f;
        int a = (int)((20 - i * 3) * alpha * (0.6f + 0.4f * pulse));
        dl->AddCircleFilled(mvec(cx, cy), r, IM_COL32(142, 132, 227, a), 48);
    }

    ImFont* titleFont = Fonts::DisplayBold ? Fonts::DisplayBold
                       : (Fonts::SegoeBold ? Fonts::SegoeBold
                       : (Fonts::TahomaBold ? Fonts::TahomaBold : ImGui::GetFont()));
    float titleScale = (Fonts::DisplayBold ? 1.45f : 2.4f);
    float titleSize = titleFont->FontSize * titleScale;
    const char* title = "hydra";
    mvec tsize = titleFont->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
    mvec tpos = mvec(cx - tsize.x * 0.5f, cy - tsize.y - 20.0f);

    for (int i = 4; i >= 1; i--) {
        int ga = (int)(40 * alpha / i);
        dl->AddText(titleFont, titleSize,
            mvec(tpos.x - i, tpos.y), IM_COL32(142, 132, 227, ga), title);
        dl->AddText(titleFont, titleSize,
            mvec(tpos.x + i, tpos.y), IM_COL32(142, 132, 227, ga), title);
    }
    dl->AddText(titleFont, titleSize, tpos,
        IM_COL32(235, 240, 248, (int)(255 * alpha)), title);

    const char* tagline = "slotted";
    float tagSize = titleFont->FontSize * 0.95f;
    mvec tgsize = titleFont->CalcTextSizeA(tagSize, FLT_MAX, 0.0f, tagline);
    mvec tgpos = mvec(cx - tgsize.x * 0.5f, tpos.y + tsize.y + 2.0f);
    dl->AddText(titleFont, tagSize, tgpos,
        IM_COL32(150, 165, 185, (int)(220 * alpha)), tagline);

    const char* statusText = "Waiting for Discord...";
    if (g_loadingTimer >= 0.7f)  statusText = "Attaching to Fortnite...";
    if (g_loadingTimer >= 1.4f)  statusText = "Loading configuration...";
    if (g_loadingTimer >= 2.1f) statusText = "Initializing ESP...";
    mvec stsize = ImGui::CalcTextSize(statusText);
    dl->AddText(mvec(cx - stsize.x * 0.5f, cy + 10.0f),
        IM_COL32(150, 165, 185, (int)(220 * alpha)), statusText);

    float barW = 320.0f;
    float barH = 3.0f;
    float barX = cx - barW * 0.5f;
    float barY = cy + 32.0f;

    dl->AddRectFilled(
        mvec(barX, barY), mvec(barX + barW, barY + barH),
        IM_COL32(40, 48, 60, (int)(180 * alpha)), barH * 0.5f);

    float p = t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f;
    float fillW = barW * p;

    if (fillW > 1.0f) {
        dl->AddRectFilledMultiColor(
            mvec(barX, barY), mvec(barX + fillW, barY + barH),
            IM_COL32(63, 60, 168, (int)(255 * alpha)),
            IM_COL32(142, 132, 227, (int)(255 * alpha)),
            IM_COL32(142, 132, 227, (int)(255 * alpha)),
            IM_COL32(63, 60, 168, (int)(255 * alpha)));

        float glowPulse = 0.5f + 0.5f * sinf(g_loadingTimer * 4.0f);
        dl->AddCircleFilled(mvec(barX + fillW, barY + barH * 0.5f),
            5.0f, IM_COL32(180, 170, 245, (int)(140 + 100 * glowPulse * alpha)), 16);
    }

    char msg[32];
    int percent = (int)(p * 100.0f);
    sprintf_s(msg, "loading... %d%%", percent);
    mvec msize = ImGui::CalcTextSize(msg);
    dl->AddText(mvec(cx - msize.x * 0.5f, barY + barH + 10.0f),
        IM_COL32(130, 145, 165, (int)(220 * alpha)), msg);

    if (g_loadingTimer >= g_loadingDuration) {
        g_loadingDone = true;
    }
}

void ApplyColors(ImVec4* colors)
{

	colors[ImGuiCol_Text]                   = ImVec4(0.910f, 0.914f, 0.949f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.306f, 0.314f, 0.376f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.078f, 0.078f, 0.110f, 0.75f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.106f, 0.106f, 0.149f, 0.98f);
	colors[ImGuiCol_Border]                 = ImVec4(0.165f, 0.165f, 0.227f, 0.00f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.165f, 0.165f, 0.251f, 0.94f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.212f, 0.208f, 0.310f, 0.94f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.255f, 0.251f, 0.376f, 0.94f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.058f, 0.058f, 0.090f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.058f, 0.058f, 0.090f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.000f, 0.000f, 0.000f, 0.51f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.106f, 0.106f, 0.149f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.058f, 0.058f, 0.090f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.353f, 0.322f, 0.612f, 0.55f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.443f, 0.404f, 0.760f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.557f, 0.518f, 0.890f, 1.00f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.557f, 0.518f, 0.890f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.557f, 0.518f, 0.890f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.659f, 0.616f, 0.941f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.557f, 0.518f, 0.890f, 0.25f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.557f, 0.518f, 0.890f, 0.55f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.557f, 0.518f, 0.890f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.557f, 0.518f, 0.890f, 0.20f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.557f, 0.518f, 0.890f, 0.40f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.557f, 0.518f, 0.890f, 0.85f);
	colors[ImGuiCol_Separator]              = ImVec4(0.165f, 0.165f, 0.227f, 0.55f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.557f, 0.518f, 0.890f, 0.78f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.659f, 0.616f, 0.941f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.557f, 0.518f, 0.890f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.557f, 0.518f, 0.890f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.659f, 0.616f, 0.941f, 0.95f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.122f, 0.110f, 0.302f, 0.86f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.443f, 0.404f, 0.760f, 0.80f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.353f, 0.322f, 0.612f, 1.00f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.078f, 0.078f, 0.110f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.165f, 0.149f, 0.400f, 1.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.557f, 0.518f, 0.890f, 0.30f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(0.659f, 0.616f, 0.941f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.557f, 0.518f, 0.890f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.60f, 0.65f, 0.70f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.10f, 0.10f, 0.15f, 0.35f);
}

void ApplyStyle(ImGuiStyle& style)
{
	style.WindowRounding    = 12.0f;
	style.ChildRounding     = 10.0f;
	style.FrameRounding     = 7.0f;
	style.GrabRounding      = 7.0f;
	style.ScrollbarRounding = 8.0f;
	style.TabRounding       = 6.0f;
	style.PopupRounding     = 8.0f;
	style.ItemSpacing       = mvec(12.0f, 8.0f);
	style.ItemInnerSpacing  = mvec(10.0f, 6.0f);
	style.FramePadding      = mvec(7.0f, 4.0f);
	style.WindowPadding     = mvec(14.0f, 12.0f);
}

int MenuTab = 1;
int VisualPage = 0;
int ExploitPage = 0;
int SettingPage = 0;

std::string TestText = TEXT("yeet is gay nga.");
bool ischeking = false;

namespace Preview
{
	namespace box3d {
		float topleftfront_x = -72.f;
		float topleftfront_y = 82.f;

		float topleftback_x = -54.f;
		float topleftback_y = 61.f;

		float toprightfront_x = 61.f;
		float toprightfront_y = 82.f;

		float toprightback_x = 74.f;
		float toprightback_y = 61.f;

		float bottomleftfront_x = -85.f;
		float bottomleftfront_y = 335.f;

		float bottomleftback_x = -67.f;
		float bottomleftback_y = 305.f;

		float bottomrightfront_x = 49.f;
		float bottomrightfront_y = 342.f;

		float bottomrightback_x = 64.f;
		float bottomrightback_y = 312.f;
	}

	float head_bone_x = 0;
	float head_bone_y = 94;

	float neck_bone_x = 4;
	float neck_bone_y = 114;

	float chest_bone_x = 2;
	float chest_bone_y = 122;

	float left_shoulder_bone_x = -24;
	float left_shoulder_bone_y = 124;

	float left_elbow_bone_x = -37;
	float left_elbow_bone_y = 153;

	float left_hand_bone_x = -51;
	float left_hand_bone_y = 196;

	float right_hand_bone_x = 47;
	float right_hand_bone_y = 190;

	float right_elbow_bone_x = 43;
	float right_elbow_bone_y = 167;

	float right_shoulder_bone_x = 20;
	float right_shoulder_bone_y = 124;

	float pelvis_bone_x = -6;
	float pelvis_bone_y = 184;

	float left_thigh_bone_x = -22;
	float left_thigh_bone_y = 186;

	float right_thigh_bone_x = 12;
	float right_thigh_bone_y = 186;

	float right_knee_bone_x = 25;
	float right_knee_bone_y = 253;

	float left_knee_bone_x = -37;
	float left_knee_bone_y = 245;

	float right_foot_bone_x = 31;
	float right_foot_bone_y = 310;

	float left_foot_bone_x = -47;
	float left_foot_bone_y = 308;

	float right_footfinger_bone_x = 37;
	float right_footfinger_bone_y = 324;

	float left_footfinger_bone_x = -57;
	float left_footfinger_bone_y = 322;

}

bool ColorsApplied = false;

void ApplyCheatColors() {

	{
		Options::PlayerColor::BoxVisible[0] = 0.0f;
		Options::PlayerColor::BoxVisible[1] = 1.0f;
		Options::PlayerColor::BoxVisible[2] = 0.0f;

		Options::PlayerColor::SkeletonVisible[0] = 0.0f;
		Options::PlayerColor::SkeletonVisible[1] = 1.0f;
		Options::PlayerColor::SkeletonVisible[2] = 0.0f;

		Options::PlayerColor::SnaplineVisible[0] = 0.0f;
		Options::PlayerColor::SnaplineVisible[1] = 1.0f;
		Options::PlayerColor::SnaplineVisible[2] = 0.0f;

		Options::PlayerColor::TopTextVisible[0] = 0.0f;
		Options::PlayerColor::TopTextVisible[1] = 1.0f;
		Options::PlayerColor::TopTextVisible[2] = 0.0f;

		Options::PlayerColor::BottomTextVisible[0] = 0.0f;
		Options::PlayerColor::BottomTextVisible[1] = 1.0f;
		Options::PlayerColor::BottomTextVisible[2] = 0.0f;

		Options::PlayerColor::BoxFillColorVisible[0] = 0.0f;
		Options::PlayerColor::BoxFillColorVisible[1] = 1.0f;
		Options::PlayerColor::BoxFillColorVisible[2] = 0.0f;
		Options::PlayerColor::BoxFillColorVisible[3] = 0.3f;
	}

	{
		Options::PlayerColor::BoxNotVisible[0] = 1.0f;
		Options::PlayerColor::BoxNotVisible[1] = 0.0f;
		Options::PlayerColor::BoxNotVisible[2] = 0.0f;

		Options::PlayerColor::SkeletonNotVisible[0] = 1.0f;
		Options::PlayerColor::SkeletonNotVisible[1] = 0.0f;
		Options::PlayerColor::SkeletonNotVisible[2] = 0.0f;

		Options::PlayerColor::SnaplineNotVisible[0] = 1.0f;
		Options::PlayerColor::SnaplineNotVisible[1] = 0.0f;
		Options::PlayerColor::SnaplineNotVisible[2] = 0.0f;

		Options::PlayerColor::TopTextNotVisible[0] = 1.0f;
		Options::PlayerColor::TopTextNotVisible[1] = 0.0f;
		Options::PlayerColor::TopTextNotVisible[2] = 0.0f;

		Options::PlayerColor::BottomTextNotVisible[0] = 1.0f;
		Options::PlayerColor::BottomTextNotVisible[1] = 0.0f;
		Options::PlayerColor::BottomTextNotVisible[2] = 0.0f;

		Options::PlayerColor::BoxFillColorNotVisible[0] = 1.0f;
		Options::PlayerColor::BoxFillColorNotVisible[1] = 0.0f;
		Options::PlayerColor::BoxFillColorNotVisible[2] = 0.0f;
		Options::PlayerColor::BoxFillColorNotVisible[3] = 0.3f;
	}

	ColorsApplied = true;
}

namespace Particles {
	struct Particle {
		float x, y;
		float trailX, trailY;
		float baseX, baseY;
		float vx, vy;
		float orbitRadius;
		float orbitSpeed;
		float orbitPhase;
		float size;
		float baseAlpha;
		float pulsePhase;
		float pulseSpeed;
		float hueShift;
	};

	static constexpr int COUNT = 100;
	static constexpr float CONNECT_DIST = 150.0f;
	static Particle particles[COUNT];
	static bool initialized = false;

	static float RandFloat(float lo, float hi) {
		return lo + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (hi - lo)));
	}

	static void Init(float screenW, float screenH) {
		srand((unsigned)time(nullptr));
		for (int i = 0; i < COUNT; i++) {
			auto& p = particles[i];
			p.baseX = RandFloat(0, screenW);
			p.baseY = RandFloat(0, screenH);
			p.x = p.baseX;
			p.y = p.baseY;
			p.trailX = p.x;
			p.trailY = p.y;
			p.vx = RandFloat(-15.0f, 15.0f);
			p.vy = RandFloat(-10.0f, 10.0f);
			p.orbitRadius = RandFloat(8.0f, 35.0f);
			p.orbitSpeed = RandFloat(0.3f, 1.5f);
			p.orbitPhase = RandFloat(0.0f, 6.28f);
			p.size = RandFloat(1.2f, 3.5f);
			p.baseAlpha = RandFloat(0.20f, 0.65f);
			p.pulsePhase = RandFloat(0.0f, 6.28f);
			p.pulseSpeed = RandFloat(0.5f, 2.5f);
			p.hueShift = RandFloat(-0.08f, 0.08f);
		}
		initialized = true;
	}

	static void UpdateAndDraw(ImDrawList* drawList, float dt, float screenW, float screenH, float time) {
		if (!initialized)
			Init(screenW, screenH);

		const float trailCatchUp = 1.0f - powf(0.001f, dt * 2.5f);

		for (int i = 0; i < COUNT; i++) {
			auto& p = particles[i];
			p.baseX += p.vx * dt;
			p.baseY += p.vy * dt;

			p.x = p.baseX + sinf(time * p.orbitSpeed + p.orbitPhase) * p.orbitRadius;
			p.y = p.baseY + cosf(time * p.orbitSpeed * 0.7f + p.orbitPhase) * p.orbitRadius * 0.6f;

			if (p.baseX < -40) { p.baseX = screenW + 40; p.trailX = p.x; p.trailY = p.y; }
			if (p.baseX > screenW + 40) { p.baseX = -40; p.trailX = p.x; p.trailY = p.y; }
			if (p.baseY < -40) { p.baseY = screenH + 40; p.trailX = p.x; p.trailY = p.y; }
			if (p.baseY > screenH + 40) { p.baseY = -40; p.trailX = p.x; p.trailY = p.y; }

			p.trailX += (p.x - p.trailX) * trailCatchUp;
			p.trailY += (p.y - p.trailY) * trailCatchUp;
		}

		for (int i = 0; i < COUNT; i++) {
			for (int j = i + 1; j < COUNT; j++) {
				float dx = particles[i].x - particles[j].x;
				float dy = particles[i].y - particles[j].y;
				float distSq = dx * dx + dy * dy;
				if (distSq < CONNECT_DIST * CONNECT_DIST) {
					float dist = sqrtf(distSq);
					float t = 1.0f - dist / CONNECT_DIST;
					float lineAlpha = t * t * 0.25f;

					int r = 142;
					int g = (int)(132 + 12 * sinf(time * 0.5f));
					int b = 227;
					drawList->AddLine(
						mvec(particles[i].x, particles[i].y),
						mvec(particles[j].x, particles[j].y),
						IM_COL32(r, g, b, (int)(255 * lineAlpha)), 0.6f);
				}
			}
		}

		for (int i = 0; i < COUNT; i++) {
			auto& p = particles[i];
			float pulse = 0.5f + 0.5f * sinf(time * p.pulseSpeed + p.pulsePhase);
			float sizeAnim = p.size * (0.7f + 0.5f * pulse);
			float finalAlpha = p.baseAlpha * (0.5f + 0.5f * pulse);

			int pr = (int)(142 + p.hueShift * 90);
			int pg = (int)(132 + p.hueShift * 110);
			int pb = (int)(227 + p.hueShift * 60);
			pr = pr < 0 ? 0 : (pr > 255 ? 255 : pr);
			pg = pg < 0 ? 0 : (pg > 255 ? 255 : pg);
			pb = pb < 0 ? 0 : (pb > 255 ? 255 : pb);

			float tdx = p.x - p.trailX;
			float tdy = p.y - p.trailY;
			float trailLenSq = tdx * tdx + tdy * tdy;
			if (trailLenSq > 0.3f) {
				const int segs = 10;
				float thick = sizeAnim * 1.0f;
				for (int s = 0; s < segs; s++) {
					float t0 = (float)s        / (float)segs;
					float t1 = (float)(s + 1)  / (float)segs;
					ImVec2 a = mvec(p.trailX + tdx * t0, p.trailY + tdy * t0);
					ImVec2 b = mvec(p.trailX + tdx * t1, p.trailY + tdy * t1);
					float segAlpha = finalAlpha * 0.75f * t1 * t1 * t1;
					ImU32 streakCol = IM_COL32(pr, pg, pb, (int)(255 * segAlpha));
					drawList->AddLine(a, b, streakCol, thick * (0.2f + 0.8f * t1));
				}
			}

			float twinkle = 0.7f + 0.3f * sinf(time * p.pulseSpeed * 1.7f + p.pulsePhase * 2.3f);

			ImU32 glowOuter = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * 0.18f * twinkle));
			drawList->AddCircleFilled(mvec(p.x, p.y), sizeAnim * 5.0f, glowOuter, 16);
			ImU32 glowInner = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * 0.40f * twinkle));
			drawList->AddCircleFilled(mvec(p.x, p.y), sizeAnim * 2.5f, glowInner, 14);

			float spikeLen = sizeAnim * 4.5f * twinkle;
			ImU32 spikeCol = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * 0.55f * twinkle));
			ImU32 spikeFade = IM_COL32(pr, pg, pb, 0);

			const int spikeSegs = 5;
			for (int s = 0; s < spikeSegs; s++) {
				float tA = (float)s / (float)spikeSegs;
				float tB = (float)(s + 1) / (float)spikeSegs;
				float aAlpha = (1.0f - tA) * (1.0f - tA);
				float bAlpha = (1.0f - tB) * (1.0f - tB);
				ImU32 cA = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * 0.55f * twinkle * aAlpha));
				ImU32 cB = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * 0.55f * twinkle * bAlpha));
				(void)cA; (void)cB;
				float mid = (aAlpha + bAlpha) * 0.5f;
				ImU32 segCol = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * 0.55f * twinkle * mid));

				drawList->AddLine(mvec(p.x + spikeLen * tA, p.y), mvec(p.x + spikeLen * tB, p.y), segCol, 1.0f);

				drawList->AddLine(mvec(p.x - spikeLen * tA, p.y), mvec(p.x - spikeLen * tB, p.y), segCol, 1.0f);

				drawList->AddLine(mvec(p.x, p.y + spikeLen * tA), mvec(p.x, p.y + spikeLen * tB), segCol, 1.0f);

				drawList->AddLine(mvec(p.x, p.y - spikeLen * tA), mvec(p.x, p.y - spikeLen * tB), segCol, 1.0f);
			}

			ImU32 coreCol = IM_COL32(pr, pg, pb, (int)(255 * finalAlpha * twinkle));
			drawList->AddCircleFilled(mvec(p.x, p.y), sizeAnim * 0.9f, coreCol, 10);

			ImU32 hotCol = IM_COL32(255, 255, 255, (int)(255 * finalAlpha * 0.8f * twinkle));
			drawList->AddCircleFilled(mvec(p.x, p.y), sizeAnim * 0.35f, hotCol, 8);
		}
	}
}

static void InfoSection(const char* title)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 230, 240, 255));
	ImGui::PushFont(Fonts::SegoeBold ? Fonts::SegoeBold : Fonts::TahomaBold);
	ImGui::TextUnformatted(title);
	ImGui::PopFont();
	ImGui::PopStyleColor();

	mvec textSize = ImGui::CalcTextSize(title);
	mvec a = ImGui::GetCursorScreenPos();
	float lineY = a.y + 2.0f;
	float lineStart = a.x;
	float lineEnd = lineStart + textSize.x + 20.0f;

	dl->AddLine(
		mvec(lineStart, lineY),
		mvec(lineEnd, lineY),
		IM_COL32(180, 175, 210, 200), 1.5f);

	dl->AddLine(
		mvec(lineStart, lineY + 1.0f),
		mvec(lineEnd, lineY + 1.0f),
		IM_COL32(142, 132, 227, 60), 2.5f);

	ImGui::Dummy(mvec(0, 10));
}

static void InfoRow(const char* label, const char* value, ImVec4 valueColor)
{
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 165, 180, 220));
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();
	ImGui::SameLine(150.0f);
	ImGui::TextColored(valueColor, "%s", value);
}

static void StatusChip(const char* text, ImVec4 col)
{
	ImDrawList* dl = ImGui::GetWindowDrawList();
	mvec textSize = ImGui::CalcTextSize(text);
	float padX = 10.0f, padY = 3.0f;
	mvec p = ImGui::GetCursorScreenPos();
	mvec a = mvec(p.x, p.y);
	mvec b = mvec(p.x + textSize.x + padX * 2, p.y + textSize.y + padY * 2);
	float rnd = (b.y - a.y) * 0.5f;
	ImU32 fill = IM_COL32((int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255), 40);
	ImU32 brd = IM_COL32((int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255), 140);
	dl->AddRectFilled(a, b, fill, rnd);
	dl->AddRect(a, b, brd, rnd, 0, 1.0f);
	dl->AddText(mvec(a.x + padX, a.y + padY),
		IM_COL32((int)(col.x * 255), (int)(col.y * 255), (int)(col.z * 255), 255), text);
	ImGui::Dummy(mvec(textSize.x + padX * 2, textSize.y + padY * 2));
}

void RenderAimbotTab();

void MakeMenu()
{
	using namespace Options;

	if (!ColorsApplied)
	ApplyCheatColors();

	float dt = ImGui::GetIO().DeltaTime;
	if (g_menuOpenAnim < 1.0f) {
		g_menuOpenAnim += dt * 4.0f;
		if (g_menuOpenAnim > 1.0f) g_menuOpenAnim = 1.0f;
	}

	float t = 1.0f - (1.0f - g_menuOpenAnim) * (1.0f - g_menuOpenAnim) * (1.0f - g_menuOpenAnim);

	if (g_lastMenuTab != MenuTab || g_lastVisualPage != VisualPage || g_lastSettingPage != SettingPage) {
		g_tabFadeT = 0.0f;
		g_lastMenuTab = MenuTab;
		g_lastVisualPage = VisualPage;
		g_lastSettingPage = SettingPage;
	}
	g_tabFadeT += dt / g_tabFadeDuration;
	if (g_tabFadeT > 1.0f) g_tabFadeT = 1.0f;

	float tabFade = 1.0f - (1.0f - g_tabFadeT) * (1.0f - g_tabFadeT) * (1.0f - g_tabFadeT);

	ImGui::PushFont(Fonts::SegoeSemi ? Fonts::SegoeSemi : Fonts::TahomaBold);
	ImDrawList* m = ImGui::GetForegroundDrawList();
	ImDrawList* u = ImGui::GetBackgroundDrawList();
	u->AddRectFilled(mvec(0, 0), mvec(CenterX * 2, CenterY * 2), IM_COL32(0, 0, 0, (int)(140 * t)), 0.0f, 0);

	Particles::UpdateAndDraw(u, dt, (float)(CenterX * 2), (float)(CenterY * 2), g_menuAnimTimer);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, t);

	static bool m_open = true;
	static int m_size_x = 750;
	static int m_size_y = 510;
	static int m_pos_x = CenterX - (m_size_x / 2);
	static int m_pos_y = CenterY - (m_size_y / 2);
	static int m_prev_pos_x = m_pos_x;
	static int m_prev_pos_y = m_pos_y;
	auto m_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	auto sidebar_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;
	auto& m_style = ImGui::GetStyle();
	ImGuiIO& m_io = ImGui::GetIO(); (void)m_io;
	ImVec4* m_colors = ImGui::GetStyle().Colors;

	ApplyColors(m_colors);
	ApplyStyle(m_style);

	const int screenW = CenterX * 2;
	const int screenH = CenterY * 2;
	const int previewW = 250 + 25;
	if (m_pos_x < -(m_size_x - 100)) m_pos_x = -(m_size_x - 100);
	if (m_pos_x > screenW - 100)      m_pos_x = screenW - 100;
	if (m_pos_y < -(m_size_y - 40))  m_pos_y = -(m_size_y - 40);
	if (m_pos_y > screenH - 40)       m_pos_y = screenH - 40;
	bool g_mirrorLayout = (m_pos_x + m_size_x + previewW > screenW - 4);

	g_menuAnimTimer += ImGui::GetIO().DeltaTime;
	g_glowPulse = 0.5f + 0.5f * sinf(g_menuAnimTimer * 2.0f);

	if (Options::RGBMode::bEnabled) {
		Options::RGBMode::rgbTime += ImGui::GetIO().DeltaTime * Options::RGBMode::fSpeed;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, mvec(4.0f, 4.0f));

	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(34, 31, 58, 200));
	if (ImGui::Begin(TEXT("hydra - buttons main"), &m_open, sidebar_flags)) {
		int SizeOfThisX = 80;
		int offset_y = 125;
		int test = 19;

		int sidebarX = g_mirrorLayout
			? (m_pos_x + m_size_x + 25)
			: (m_pos_x - (SizeOfThisX + 25));
		ImGui::SetWindowPos(mvec(sidebarX, m_pos_y + offset_y + (test / 2)));
		ImGui::SetWindowSize(mvec(SizeOfThisX, m_size_y - ((offset_y * 2) + test)));
		if (Fonts::TextureFont) ImGui::PushFont(Fonts::TextureFont);
		else ImGui::PushFont(Fonts::StandardFont ? Fonts::StandardFont : ImGui::GetFont());

		float btnSize = 42.0f;
		float contentW = (float)SizeOfThisX - 8.0f;
		float btnX = (contentW - btnSize) * 0.5f;
		float totalBtnH = btnSize * 4.0f + 8.0f * 3.0f;
		float windowH = (float)(m_size_y - ((offset_y * 2) + test)) - 8.0f;
		float startY = (windowH - totalBtnH) * 0.5f;

		ImGui::SetCursorPos(mvec(btnX, startY));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, mvec(0.0f, 8.0f));

		ImGui::EpicButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("c"), mvec(btnSize, btnSize), MenuTab == 1);
		if (ImGui::IsItemClicked()) {
			MenuTab = 1;
		}
		ImGui::SetCursorPosX(btnX);

		ImGui::EpicButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("d"), mvec(btnSize, btnSize), MenuTab == 3);
		if (ImGui::IsItemClicked()) {
			MenuTab = 3;
		}
		ImGui::SetCursorPosX(btnX);

		ImGui::EpicButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("e"), mvec(btnSize, btnSize), MenuTab == 4);
		if (ImGui::IsItemClicked()) {
			MenuTab = 4;
		}
		ImGui::SetCursorPosX(btnX);

		ImGui::EpicButton(ImVec4(0.85f, 0.15f, 0.15f, 1.0f), TEXT("a"), mvec(btnSize, btnSize), MenuTab == 5);
		if (ImGui::IsItemClicked()) {
			MenuTab = 5;
		}

		ImGui::PopStyleVar();
		ImGui::PopFont();
		ImGui::End();
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	if (MenuTab == 1 && (VisualPage == 0 || VisualPage == 1)) {

		int lenght = 250;
		int x1 = g_mirrorLayout
			? (m_pos_x - (lenght + 25))
			: (m_pos_x + m_size_x + 25);
		int y1 = m_pos_y;
		int x2 = x1 + lenght;
		int y2 = m_pos_y + m_size_y;
		int x3 = (x1 + lenght) - (lenght / 2);
		auto Color_Vec4 = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		auto Color_GCU32 = ImGui::GetColorU32({ 1.0f, 0.0f, 0.0f, 1.0f });
		auto Color_U32 = IM_COL32(255, 0, 0, 255);

		m->AddRectFilled(mvec(x1, y1), mvec(x2, y2), IM_COL32(20, 20, 28, 247), 12.0f, 0);

		m->AddRectFilled(
			mvec((float)x1 + 1, (float)y1 + 1), mvec((float)x2 - 1, (float)y1 + 6),
			IM_COL32(142, 132, 227, 22));

		m->AddRect(mvec(x1, y1), mvec(x2, y2), IM_COL32(42, 42, 58, 140), 12.0f, 0, 1.0f);

		{
			float lineY     = (float)y1 + 28.0f;
			float lineStart = (float)x1 + 12.0f;
			float lineEnd   = (float)x2 - 12.0f;
			float lineMid   = (lineStart + lineEnd) * 0.5f;
			float accentA   = 0.50f + 0.15f * g_glowPulse;
			m->AddRectFilledMultiColor(
				mvec(lineStart, lineY), mvec(lineMid, lineY + 1.0f),
				IM_COL32(142, 132, 227, 0),                    IM_COL32(142, 132, 227, (int)(255 * accentA)),
				IM_COL32(142, 132, 227, (int)(255 * accentA)), IM_COL32(142, 132, 227, 0));
			m->AddRectFilledMultiColor(
				mvec(lineMid, lineY), mvec(lineEnd, lineY + 1.0f),
				IM_COL32(142, 132, 227, (int)(255 * accentA)), IM_COL32(142, 132, 227, 0),
				IM_COL32(142, 132, 227, 0),                    IM_COL32(142, 132, 227, (int)(255 * accentA)));
		}

		static bool bShowNotVisiblePreview = false;
		{
			bool previewVisible = !bShowNotVisiblePreview;

			ImVec4 rgbColor = {1.0f, 1.0f, 1.0f, 1.0f};
			if (Options::RGBMode::bEnabled) {
				float hue = fmodf(Options::RGBMode::rgbTime, 1.0f);
				ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, rgbColor.x, rgbColor.y, rgbColor.z);
				rgbColor.w = 1.0f;
			}

			auto rv4 = [&](bool bRGB, float* vis, float* nvis) -> ImVec4 {
				if (Options::RGBMode::bEnabled && bRGB) return rgbColor;
				float* s = previewVisible ? vis : nvis;
				return ImVec4(s[0], s[1], s[2], 1.0f);
			};
			auto rv4a = [&](bool bRGB, float* vis, float* nvis) -> ImVec4 {
				if (Options::RGBMode::bEnabled && bRGB) return rgbColor;
				float* s = previewVisible ? vis : nvis;
				return ImVec4(s[0], s[1], s[2], s[3]);
			};
			ImVec4 ColorBox    = rv4(Options::PlayerColor::bBoxColorRGB, Options::PlayerColor::BoxVisible, Options::PlayerColor::BoxNotVisible);
			ImVec4 ColorSkel   = rv4(Options::PlayerColor::bSkeletonColorRGB, Options::PlayerColor::SkeletonVisible, Options::PlayerColor::SkeletonNotVisible);
			ImVec4 ColorSnap   = rv4(Options::PlayerColor::bSnaplineColorRGB, Options::PlayerColor::SnaplineVisible, Options::PlayerColor::SnaplineNotVisible);
			ImVec4 ColorTopTxt = rv4(Options::PlayerColor::bTopTextColorRGB, Options::PlayerColor::TopTextVisible, Options::PlayerColor::TopTextNotVisible);
			ImVec4 ColorBotTxt = rv4(Options::PlayerColor::bBottomTextColorRGB, Options::PlayerColor::BottomTextVisible, Options::PlayerColor::BottomTextNotVisible);

			ImU32 colBox        = ImGui::GetColorU32(ColorBox);
			ImU32 colSkel       = ImGui::GetColorU32(ColorSkel);
			ImU32 colSnap       = ImGui::GetColorU32(ColorSnap);
			ImU32 colTopText    = ImGui::GetColorU32(ColorTopTxt);
			ImU32 colBottomText = ImGui::GetColorU32(ColorBotTxt);

			ImFont* previewFont = Render::GetSelectedPlayerFont();
			if (previewFont) ImGui::PushFont(previewFont);

			const float TextY = ImGui::CalcTextSize("ABCDEFabcdef1234567890.,").y;

			float cx = (float)x3;

			float skelMid = (Preview::head_bone_y + (Preview::right_footfinger_bone_y > Preview::left_footfinger_bone_y ? Preview::right_footfinger_bone_y : Preview::left_footfinger_bone_y)) / 2.0f;
			float panelMidY = (float)y1 + (float)m_size_y / 2.0f;
			float cy = panelMidY - skelMid;

			auto drawPreviewText = [&](const char* text, float px, float py, ImU32 color) {
				mvec ts = ImGui::CalcTextSize(text);
				float fs = previewFont ? previewFont->FontSize : ImGui::GetFontSize();
				if (Options::PlayerFeatures::TextBackground) {
					m->AddRectFilled(mvec(px - 2, py - 1), mvec(px + ts.x + 2, py + ts.y + 1), IM_COL32(0, 0, 0, 178), 2.0f);
				}
				if (Options::PlayerFeatures::OutlinedText) {
					ImU32 sh = IM_COL32(0, 0, 0, 255);
					m->AddText(previewFont, fs, mvec(px + 1, py + 1), sh, text);
					m->AddText(previewFont, fs, mvec(px - 1, py - 1), sh, text);
					m->AddText(previewFont, fs, mvec(px + 1, py - 1), sh, text);
					m->AddText(previewFont, fs, mvec(px - 1, py + 1), sh, text);
				}
				m->AddText(previewFont, fs, mvec(px, py), color, text);
			};

			auto bp = [&](float bx, float by) -> mvec { return mvec(cx + bx, cy + by); };

			mvec headPos   = bp(Preview::head_bone_x, Preview::head_bone_y);
			float bottomY  = cy + (Preview::right_footfinger_bone_y > Preview::left_footfinger_bone_y ? Preview::right_footfinger_bone_y : Preview::left_footfinger_bone_y);
			mvec bottomPos = mvec(cx, bottomY);

			float lineGap = 3.0f;
			float TopTextOffset = TextY + 28.0f;
			float BottomTextOffset = 18.0f;

			if (Options::bSkeleton) {

				mvec ch_head[]      = { bp(Preview::head_bone_x, Preview::head_bone_y), bp(Preview::neck_bone_x, Preview::neck_bone_y) };
				mvec ch_spine[]     = { bp(Preview::neck_bone_x, Preview::neck_bone_y), bp(Preview::pelvis_bone_x, Preview::pelvis_bone_y) };
				mvec ch_larm[]      = { bp(Preview::left_shoulder_bone_x, Preview::left_shoulder_bone_y), bp(Preview::left_elbow_bone_x, Preview::left_elbow_bone_y), bp(Preview::left_hand_bone_x, Preview::left_hand_bone_y) };
				mvec ch_rarm[]      = { bp(Preview::right_shoulder_bone_x, Preview::right_shoulder_bone_y), bp(Preview::right_elbow_bone_x, Preview::right_elbow_bone_y), bp(Preview::right_hand_bone_x, Preview::right_hand_bone_y) };
				mvec ch_lleg[]      = { bp(Preview::pelvis_bone_x, Preview::pelvis_bone_y), bp(Preview::left_thigh_bone_x, Preview::left_thigh_bone_y), bp(Preview::left_knee_bone_x, Preview::left_knee_bone_y), bp(Preview::left_foot_bone_x, Preview::left_foot_bone_y) };
				mvec ch_rleg[]      = { bp(Preview::pelvis_bone_x, Preview::pelvis_bone_y), bp(Preview::right_thigh_bone_x, Preview::right_thigh_bone_y), bp(Preview::right_knee_bone_x, Preview::right_knee_bone_y), bp(Preview::right_foot_bone_x, Preview::right_foot_bone_y) };
				mvec ch_shoulders[] = { bp(Preview::left_shoulder_bone_x, Preview::left_shoulder_bone_y), bp(Preview::right_shoulder_bone_x, Preview::right_shoulder_bone_y) };
				mvec ch_lfoot[]     = { bp(Preview::left_knee_bone_x, Preview::left_knee_bone_y), bp(Preview::left_foot_bone_x, Preview::left_foot_bone_y), bp(Preview::left_foot_bone_x, Preview::left_foot_bone_y), bp(Preview::left_footfinger_bone_x, Preview::left_footfinger_bone_y) };
				mvec ch_rfoot[]     = { bp(Preview::right_knee_bone_x, Preview::right_knee_bone_y), bp(Preview::right_foot_bone_x, Preview::right_foot_bone_y), bp(Preview::right_foot_bone_x, Preview::right_foot_bone_y), bp(Preview::right_footfinger_bone_x, Preview::right_footfinger_bone_y) };

				float* skelOutSrc = previewVisible ? Options::PlayerColor::SkeletonOutlineVisible : Options::PlayerColor::SkeletonOutlineNotVisible;
				ImU32 skelOutCol  = IM_COL32((int)(skelOutSrc[0]*255), (int)(skelOutSrc[1]*255), (int)(skelOutSrc[2]*255), 255);

				struct BoneChain { mvec* pts; int count; };
				BoneChain chains[] = {
					{ch_head, 2}, {ch_spine, 2}, {ch_larm, 3}, {ch_rarm, 3},
					{ch_lleg, 4}, {ch_rleg, 4}, {ch_shoulders, 2},
					{ch_lfoot, 4}, {ch_rfoot, 4}
				};

				const int chainCount = sizeof(chains) / sizeof(chains[0]);

				if (Options::bSmoothSkeleton) {

					auto catmullRom = [](mvec p0, mvec p1, mvec p2, mvec p3, float t) -> mvec {
						float t2 = t * t, t3 = t2 * t;
						return mvec(
							0.5f*((2.f*p1.x)+(-p0.x+p2.x)*t+(2.f*p0.x-5.f*p1.x+4.f*p2.x-p3.x)*t2+(-p0.x+3.f*p1.x-3.f*p2.x+p3.x)*t3),
							0.5f*((2.f*p1.y)+(-p0.y+p2.y)*t+(2.f*p0.y-5.f*p1.y+4.f*p2.y-p3.y)*t2+(-p0.y+3.f*p1.y-3.f*p2.y+p3.y)*t3)
						);
					};
					for (int ci = 0; ci < chainCount; ci++) {
						BoneChain& chain = chains[ci];
						if (chain.count < 2) continue;
						mvec padded[8]; int pn = 0;
						padded[pn++] = chain.pts[0];
						for (int i = 0; i < chain.count; i++) padded[pn++] = chain.pts[i];
						padded[pn++] = chain.pts[chain.count - 1];
						mvec last = {}; bool first = true;
						for (int i = 0; i + 3 < pn; i++) {
							for (float t = 0.0f; t <= 1.0f; t += Options::SkeletonSmooth::fStep) {
								mvec pt = catmullRom(padded[i], padded[i+1], padded[i+2], padded[i+3], t);
								if (first) { last = pt; first = false; continue; }
								if (Options::PlayerFeatures::bSkeletonOutline) {
									float dx = pt.x - last.x, dy = pt.y - last.y;
									float len = sqrtf(dx * dx + dy * dy);
									float ox = 0.0f, oy = 0.0f;
									if (len > 0.0f) { ox = -dy / len; oy = dx / len; }
									m->AddLine(ImVec2(last.x+ox, last.y+oy), ImVec2(pt.x+ox, pt.y+oy), skelOutCol, 1.0f);
									m->AddLine(ImVec2(last.x-ox, last.y-oy), ImVec2(pt.x-ox, pt.y-oy), skelOutCol, 1.0f);
								}
								m->AddLine(last, pt, colSkel, Options::fSkeletonThickness);
								last = pt;
							}
						}
					}
				} else {

					for (int ci = 0; ci < chainCount; ci++) {
						BoneChain& chain = chains[ci];
						if (chain.count < 2) continue;
						if (Options::PlayerFeatures::bSkeletonOutline)
							for (int s = 0; s < chain.count - 1; s++) {
								float dx = chain.pts[s+1].x - chain.pts[s].x, dy = chain.pts[s+1].y - chain.pts[s].y;
								float len = sqrtf(dx * dx + dy * dy);
								float ox = 0.0f, oy = 0.0f;
								if (len > 0.0f) { ox = -dy / len; oy = dx / len; }
								m->AddLine(ImVec2(chain.pts[s].x+ox, chain.pts[s].y+oy), ImVec2(chain.pts[s+1].x+ox, chain.pts[s+1].y+oy), skelOutCol, 1.0f);
								m->AddLine(ImVec2(chain.pts[s].x-ox, chain.pts[s].y-oy), ImVec2(chain.pts[s+1].x-ox, chain.pts[s+1].y-oy), skelOutCol, 1.0f);
							}
						m->AddPolyline(chain.pts, chain.count, colSkel, ImDrawFlags_None, Options::fSkeletonThickness);
					}
				}
			}

			if (Options::bCharacters) {
				float bH = (bottomY) - (headPos.y);
				float bW = bH * 0.60f;
				float bX = cx - bW / 2.0f;
				float bY = headPos.y;

				float previewThickness = Options::CharacterModes::fBoxThickness;

				RenderBox(bX, bY, bW, bH, ColorBox,
				            previewThickness, true,
				            Options::CharacterModes::box_type, previewVisible, m);
			}

			if (Options::bSnapline) {
				mvec snapStart, snapEnd;
				if (Options::SnaplineStart::bBottom) {
					snapStart = mvec(cx, (float)y2 - 10.0f);
					snapEnd   = mvec(bottomPos.x, bottomPos.y + 1.0f);
				} else {
					snapStart = mvec(cx, (float)y1 + 10.0f);
					snapEnd   = mvec(headPos.x, headPos.y - 2.0f);
				}
				if (Options::PlayerFeatures::bSnaplineOutline)
					m->AddLine(snapStart, snapEnd, IM_COL32(0,0,0,255), Options::PlayerFeatures::fSnaplineOutlineThickness);
				m->AddLine(snapStart, snapEnd, colSnap, 1.0f);
			}

			if (Options::bplayername && Options::bPlayerDistance) {
				const char* combined = "Player (50m)";
				mvec cs = ImGui::CalcTextSize(combined);
				float tx = headPos.x - (cs.x / 2.0f);
				float ty = headPos.y - TopTextOffset - 4.0f;
				drawPreviewText(combined, tx, ty, colTopText);
				TopTextOffset += TextY + lineGap;
			}
			else if (Options::bplayername) {
				const char* nm = "Player";
				mvec ns = ImGui::CalcTextSize(nm);
				float tx = headPos.x - (ns.x / 2.0f);
				float ty = headPos.y - TopTextOffset - 4.0f;
				drawPreviewText(nm, tx, ty, colTopText);
				TopTextOffset += TextY + lineGap;
			}
			else if (Options::bPlayerDistance) {
				const char* ds = "(50m)";
				mvec dz = ImGui::CalcTextSize(ds);
				float tx = headPos.x - (dz.x / 2.0f);
				float ty = headPos.y - TopTextOffset - 4.0f;
				drawPreviewText(ds, tx, ty, colTopText);
				TopTextOffset += TextY + lineGap;
			}

			if (Options::bplattform) {
				const char* pl = "PC";
				mvec ps = ImGui::CalcTextSize(pl);
				float tx = headPos.x - (ps.x / 2.0f);
				float ty = headPos.y - TopTextOffset - 4.0f;
				drawPreviewText(pl, tx, ty, colTopText);
				TopTextOffset += TextY + lineGap;
			}

			if (Options::bSeasonRank) {
				const char* rk = "Gold III";
				mvec rs = ImGui::CalcTextSize(rk);
				float tx = headPos.x - (rs.x / 2.0f);
				float ty = headPos.y - TopTextOffset - 4.0f;
				drawPreviewText(rk, tx, ty, colTopText);
				TopTextOffset += TextY + lineGap;
			}

			if (Options::bPlayerInfo) {
				const char* info = previewVisible ? "Visible" : "Not Visible";
				mvec is = ImGui::CalcTextSize(info);
				float tx = bottomPos.x - (is.x / 2.0f);
				float ty = bottomPos.y + BottomTextOffset + 4.0f;
				drawPreviewText(info, tx, ty, colBottomText);
				BottomTextOffset += TextY + 3.0f;
			}
			if (Options::bTeamIndex) {
				const char* ti = "T:1";
				mvec ts = ImGui::CalcTextSize(ti);
				float tx = bottomPos.x - (ts.x / 2.0f);
				float ty = bottomPos.y + BottomTextOffset + 4.0f;
				drawPreviewText(ti, tx, ty, colBottomText);
				BottomTextOffset += TextY + 3.0f;
			}
			if (Options::bTeamScore) {
				const char* sc = "K:0";
				mvec ss = ImGui::CalcTextSize(sc);
				float tx = bottomPos.x - (ss.x / 2.0f);
				float ty = bottomPos.y + BottomTextOffset + 4.0f;
				drawPreviewText(sc, tx, ty, colBottomText);
				BottomTextOffset += TextY + 3.0f;
			}

			if (previewFont) ImGui::PopFont();
		}

		{
			float tX = (float)x1 + 5.0f;
			float tY = (float)y1 + (float)m_size_y - 30.0f;
			float tW = 140.0f;
			float tH = 26.0f;

			m->AddRectFilled(mvec(tX, tY), mvec(tX + tW, tY + tH), IM_COL32(20, 20, 28, 220), 8.0f);
			m->AddRect(mvec(tX, tY), mvec(tX + tW, tY + tH), IM_COL32(42, 42, 58, 120), 8.0f, 0, 1.0f);

			float cbX = tX + 10.0f;
			float cbY = tY + (tH - 12.0f) / 2.0f;
			float cbSz = 12.0f;
			m->AddRect(mvec(cbX, cbY), mvec(cbX + cbSz, cbY + cbSz), IM_COL32(142, 132, 227, 200), 3.0f, 0, 1.5f);
			if (bShowNotVisiblePreview)
				m->AddRectFilled(mvec(cbX + 2, cbY + 2), mvec(cbX + cbSz - 2, cbY + cbSz - 2), IM_COL32(142, 132, 227, 255), 2.0f);

			ImFont* tFont = Fonts::SegoeSemi ? Fonts::SegoeSemi : Fonts::TahomaBold;
			m->AddText(tFont, tFont->FontSize, mvec(cbX + cbSz + 6.0f, cbY - 1.0f), IM_COL32(232, 233, 242, 255), "Not Visible");

			ImVec2 mp = ImGui::GetMousePos();
			if (mp.x >= tX && mp.x <= tX + tW && mp.y >= tY && mp.y <= tY + tH) {
				if (ImGui::IsMouseClicked(0))
					bShowNotVisiblePreview = !bShowNotVisiblePreview;
			}
		}
	}

	if (ImGui::Begin(TEXT("C_Renderer_Main"), &m_open, m_flags)) {
		ImGui::SetWindowSize(mvec(m_size_x, m_size_y));
		static mvec last_pos = mvec(0, 0);
		mvec current_pos = ImGui::GetWindowPos();
		if (current_pos.x != last_pos.x || current_pos.y != last_pos.y) {
			m_pos_x = (int)current_pos.x;
			m_pos_y = (int)current_pos.y;
			last_pos = current_pos;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, t * tabFade);
		ImGui::Dummy(mvec(0, 6));
		ImGui::Text(TEXT("")); ImGui::SameLine();

		{
			if (MenuTab == 1) {

						if (ImGui::BeginChild(TEXT("C_Visuals_Tab"), mvec(730, 470), false, ImGuiWindowFlags_NoScrollbar)) {
					ImGui::Text(TEXT(""));

					if (bShowVisualButtons)
					{
						if (VisualPage == 0) {
							ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Player ESP"), mvec(170, 22));
							DrawActiveTabUnderline(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
						}
						else {
							ImGui::CoolButton(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), TEXT("Player ESP"), mvec(170, 22));
						}
						if (ImGui::IsItemClicked()) {
							VisualPage = 0;
						}
						if (ImGui::IsItemHovered()) {
							TestText = TEXT("Player Wallhack.");
						}

						ImGui::SameLine();
						if (VisualPage == 1) {
							ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Colors"), mvec(170, 22));
							DrawActiveTabUnderline(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
						}
						else {
							ImGui::CoolButton(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), TEXT("Colors"), mvec(170, 22));
						}
						if (ImGui::IsItemClicked()) {
							VisualPage = 1;
						}
						if (ImGui::IsItemHovered()) {
							TestText = TEXT("Customize the cheat's colors.");
						}

						ImGui::SameLine();
						if (VisualPage == 2) {
							ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Radar"), mvec(170, 22));
							DrawActiveTabUnderline(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
						}
						else {
							ImGui::CoolButton(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), TEXT("Radar"), mvec(170, 22));
						}
						if (ImGui::IsItemClicked()) {
							VisualPage = 2;
						}
						if (ImGui::IsItemHovered()) {
							TestText = TEXT("Set the cheat's render mode. This includes streamproof, colors saturation, etc.");
						}
						ImGui::SameLine();
						if (VisualPage == 3) {
							ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Crosshair"), mvec(170, 22));
							DrawActiveTabUnderline(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
						}
						else {
							ImGui::CoolButton(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), TEXT("Crosshair"), mvec(170, 22));
						}
						if (ImGui::IsItemClicked()) {
							VisualPage = 3;
						}
						if (ImGui::IsItemHovered()) {
							TestText = TEXT("Crosshair overlay settings.");
						}
					}

					if (VisualPage == 0) {
						ImGui::Separator();
						if (ImGui::BeginChild(TEXT("C_Player_Tab_one"), mvec(340, 400))) {
							InfoSection(TEXT("VISUALS"));
							static float id_hr = 104.0f;
							static float id_st = 101.0f;

							static float id_snaplineOT = 103.0f;
							ImGui::CoolCheckbox(TEXT("Box"), &bCharacters);
							WithTooltip("Draw a box around each player.");
							ImGui::CoolCheckbox(TEXT("Skeleton"), &bSkeleton);
							WithTooltip("Draw bone lines connecting each player's joints.");
							if (bSkeleton) {
								ImGui::Text(TEXT("Skeleton Thickness"));
								ImGui::SliderFloat(id_st, "##st", &Options::fSkeletonThickness, 0.5f, 2.5f);
								WithTooltip("Line width of the skeleton, in pixels.");
								ImGui::CoolCheckbox(TEXT("Smooth Skeleton"), &Options::bSmoothSkeleton);
								WithTooltip("Smooths skeleton lines with a Catmull-Rom curve. Slightly heavier on render.");
								ImGui::CoolCheckbox(TEXT("Skeleton Outline"), &Options::PlayerFeatures::bSkeletonOutline);
								WithTooltip("Adds a contrasting offset outline around the skeleton.");
							}
							ImGui::CoolCheckbox(TEXT("Snapline"), &Options::bSnapline);
							WithTooltip("Draw a line from your screen to each player.");
							if (bSnapline) {
								ImGui::CoolCheckbox(TEXT("Snapline Outline"), &Options::PlayerFeatures::bSnaplineOutline);
								WithTooltip("Adds a contrasting outline behind the snapline.");
								if (Options::PlayerFeatures::bSnaplineOutline) {
									ImGui::Text(TEXT("Snapline Outline Thickness"));
									ImGui::SliderFloat(id_snaplineOT, "##snaplineOT", &Options::PlayerFeatures::fSnaplineOutlineThickness, 1.0f, 5.0f);
									WithTooltip("Width of the snapline outline.");
								}
							}

							ImGui::CoolCheckbox(TEXT("Username"), &bplayername);
							WithTooltip("Display each player's in-game name above their box.");
							ImGui::CoolCheckbox(TEXT("Platform"), &bplattform);
							WithTooltip("Show what platform the player is on (PC, console, mobile).");
							ImGui::CoolCheckbox(TEXT("Display Distance"), &bPlayerDistance);
							WithTooltip("Show the distance from you to each player, in metres.");
							ImGui::CoolCheckbox(TEXT("Show Bots"), &bOnBots);
							WithTooltip("Render ESP on AI bots as well as real players.");
							ImGui::CoolCheckbox(TEXT("Show Teammates"), &bOnTeammate);
							WithTooltip("Show teammates on ESP. Uncheck to hide them.");
							ImGui::CoolCheckbox(TEXT("Player Information"), &bPlayerInfo);
							WithTooltip("Show extra info such as held weapon and shield/health.");
							ImGui::CoolCheckbox(TEXT("Team Index"), &bTeamIndex);
							WithTooltip("Display each player's team number.");
							ImGui::CoolCheckbox(TEXT("Kills"), &bTeamScore);
							WithTooltip("Display each player's current kill count.");
							ImGui::CoolCheckbox(TEXT("Season Rank (only works in ranked games)"), &bSeasonRank);
							WithTooltip("Display each player's ranked division (Bronze through Unreal). Only works in ranked game modes.");
							ImGui::CoolCheckbox(TEXT("Weapon Name"), &bWeaponName);
							WithTooltip("Display the name of the weapon the player is currently holding.");
						}

						ImGui::EndChild();
						ImGui::SameLine();

						if (ImGui::BeginChild(TEXT("C_Player_Tab_Two"), mvec(340, 400))) {
							InfoSection(TEXT("FONT"));
							static const char* fontItems[] = { "Arial", "Rajdhani", "Verdana", "Fortnite", "GTA", "Standard", "Tahoma Bold" };
							int selectedFont = 0;
							if (PlayerFonts::bRajdhani) selectedFont = 1;
							else if (PlayerFonts::bVerdana) selectedFont = 2;
							else if (PlayerFonts::bFortniteFont) selectedFont = 3;
							else if (PlayerFonts::bGTAFont) selectedFont = 4;
							else if (PlayerFonts::bStandardFont) selectedFont = 5;
							else if (PlayerFonts::bTahomaBold) selectedFont = 6;

							if (ImGui::Combo(TEXT("##FontSelect"), &selectedFont, fontItems, IM_ARRAYSIZE(fontItems))) {
								PlayerFonts::bArial        = (selectedFont == 0);
								PlayerFonts::bRajdhani     = (selectedFont == 1);
								PlayerFonts::bVerdana      = (selectedFont == 2);
								PlayerFonts::bFortniteFont = (selectedFont == 3);
								PlayerFonts::bGTAFont      = (selectedFont == 4);
								PlayerFonts::bStandardFont = (selectedFont == 5);
								PlayerFonts::bTahomaBold   = (selectedFont == 6);
							}
							WithTooltip("Font used for player names, distance and other ESP text.");
							ImGui::Dummy(mvec(0, 2));
							ImGui::Text(TEXT("Snapline start"));
							static const char* snaplineItems[] = { "Top", "Bottom" };
							int selectedSnapline = Options::SnaplineStart::bBottom ? 1 : 0;

							if (ImGui::Combo(TEXT("##SnaplineSelect"), &selectedSnapline, snaplineItems, IM_ARRAYSIZE(snaplineItems))) {
								Options::SnaplineStart::bTop = (selectedSnapline == 0);
								Options::SnaplineStart::bBottom = (selectedSnapline == 1);
							}
							WithTooltip("Where the snapline originates: top of the screen or bottom.");
							ImGui::Dummy(mvec(0, 2));
						ImGui::Text(TEXT("Box Type"));
						static const char* boxTypeItems[] = { "Standard", "Corner", "Filled Standard", "Filled Corner" };
						int selectedBoxType = Options::CharacterModes::box_type;

						if (ImGui::Combo(TEXT("##BoxTypeSelect"), &selectedBoxType, boxTypeItems, IM_ARRAYSIZE(boxTypeItems))) {
								Options::CharacterModes::box_type = selectedBoxType;
						}
						WithTooltip("Player box style. Corner draws only the corners; Filled variants add a translucent fill.");

						ImGui::Text(TEXT("Box Thickness"));
						static float id_boxThk = 234.0f;
						ImGui::SliderFloat(id_boxThk, "##boxThk", &Options::CharacterModes::fBoxThickness, 1.0f, 5.0f);
							WithTooltip("Pixel thickness of box lines (outline = thickness + 2).");
							ImGui::Dummy(mvec(0, 2));
							ImGui::CoolCheckbox(TEXT("Outlined"), &Options::PlayerFeatures::OutlinedText);
							WithTooltip("Adds a dark outline around player name text.");
							ImGui::SameLine();
							ImGui::CoolCheckbox(TEXT("Background"), &Options::PlayerFeatures::TextBackground);
							WithTooltip("Adds a solid background behind player name text.");
						ImGui::EndChild();
						}
					}
					else if (VisualPage == 1) {
						ImGui::Separator();
						ImGui::Dummy(mvec(0, 2));
						auto flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoTooltip |
						ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions;
						InfoSection(TEXT("RGB MODE"));
						ImGui::CoolCheckbox(TEXT("Enable RGB"), &Options::RGBMode::bEnabled);
						WithTooltip("Cycles all ESP colors through the rainbow. Use the exclusions panel to keep specific elements at their fixed color.");
						static float id_rgbspeed = 301.0f;
						ImGui::SliderFloat(id_rgbspeed, "##RGBSpeed", &Options::RGBMode::fSpeed, 0.05f, 1.0f);
						WithTooltip("How fast the RGB cycle moves through the spectrum.");
						ImGui::SameLine();
						ImGui::Text(TEXT("Speed"));
						ImGui::Separator();
						ImGui::BeginChild(TEXT("##ColorsContainer"), mvec(0, 420), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
						ImGui::BeginChild(TEXT("C_PlayerColor_Tab_One"), mvec(190, 460), false);
						ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), TEXT("Visible"));
						ImGui::ColorEdit4(TEXT("Box"), Options::PlayerColor::BoxVisible, flags);
						ImGui::ColorEdit4(TEXT("Skeleton"), Options::PlayerColor::SkeletonVisible, flags);
						ImGui::ColorEdit4(TEXT("Skeleton Outline"), Options::PlayerColor::SkeletonOutlineVisible, flags);
						ImGui::ColorEdit4(TEXT("Snapline"), Options::PlayerColor::SnaplineVisible, flags);
						ImGui::ColorEdit4(TEXT("Top text"), Options::PlayerColor::TopTextVisible, flags);
						ImGui::ColorEdit4(TEXT("Bottom text"), Options::PlayerColor::BottomTextVisible, flags);
						ImGui::ColorEdit4(TEXT("2D Fill"), Options::PlayerColor::BoxFillColorVisible, flags);
						ImGui::EndChild();
						ImGui::SameLine();

						ImGui::BeginChild(TEXT("C_PlayerColor_Tab_Two"), mvec(190, 460), false);
						ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), TEXT("Not Visible"));
						ImGui::ColorEdit4(TEXT("Box"), Options::PlayerColor::BoxNotVisible, flags);
						ImGui::ColorEdit4(TEXT("Skeleton"), Options::PlayerColor::SkeletonNotVisible, flags);
						ImGui::ColorEdit4(TEXT("Skeleton Outline"), Options::PlayerColor::SkeletonOutlineNotVisible, flags);
						ImGui::ColorEdit4(TEXT("Snapline"), Options::PlayerColor::SnaplineNotVisible, flags);
						ImGui::ColorEdit4(TEXT("Top text"), Options::PlayerColor::TopTextNotVisible, flags);
						ImGui::ColorEdit4(TEXT("Bottom text"), Options::PlayerColor::BottomTextNotVisible, flags);
						ImGui::ColorEdit4(TEXT("2D Fill"), Options::PlayerColor::BoxFillColorNotVisible, flags);
						ImGui::EndChild();
						ImGui::SameLine();

						ImGui::BeginChild(TEXT("C_PlayerColor_Tab_Three"), mvec(180, 460), false);
						InfoSection(TEXT("RGB EXCLUSIONS"));
						if (Options::RGBMode::bEnabled) {
							ImGui::CoolCheckbox(TEXT("##Box"), &Options::PlayerColor::bBoxColorRGB);
							WithTooltip("When ticked, the player box cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("Box"));
							ImGui::CoolCheckbox(TEXT("##Skeleton"), &Options::PlayerColor::bSkeletonColorRGB);
							WithTooltip("When ticked, the skeleton cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("Skeleton"));
							ImGui::CoolCheckbox(TEXT("##Snapline"), &Options::PlayerColor::bSnaplineColorRGB);
							WithTooltip("When ticked, the snapline cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("Snapline"));
							ImGui::CoolCheckbox(TEXT("##TopText"), &Options::PlayerColor::bTopTextColorRGB);
							WithTooltip("When ticked, the top text (name) cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("Top text"));
							ImGui::CoolCheckbox(TEXT("##BottomText"), &Options::PlayerColor::bBottomTextColorRGB);
							WithTooltip("When ticked, the bottom text (distance/weapon) cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("Bottom text"));
							ImGui::CoolCheckbox(TEXT("##TitleText"), &Options::PlayerColor::bTitleTextRGB);
							WithTooltip("When ticked, the menu title text cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("Title Text"));
							ImGui::CoolCheckbox(TEXT("##BoxFill"), &Options::PlayerColor::bBoxFillColorRGB);
							WithTooltip("When ticked, the 2D box fill cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("2D Fill"));
							ImGui::CoolCheckbox(TEXT("##FOVCircle"), &Options::AssaultRifle::bFOVCircleRGB);
							WithTooltip("When ticked, the aimbot FOV circle cycles with RGB.");
							ImGui::SameLine(); ImGui::Text(TEXT("FOV Circle"));
						} else {
							ImGui::Text(TEXT("Enable RGB"));
							ImGui::Text(TEXT("to see"));
							ImGui::Text(TEXT("exclusions"));
						}
						ImGui::EndChild();
						ImGui::EndChild();
					}
					if (VisualPage == 2) {
						ImGui::Separator();
						ImGui::Dummy(mvec(0, 3));
						bool radar1Open = ImGui::BeginChild(TEXT("C_Radar_Tab_One"), mvec(340, 390));
						if (radar1Open) {
							InfoSection(TEXT("RADAR OPTIONS"));
							ImGui::CoolCheckbox(TEXT("Radar"), &bRadar);
							WithTooltip("Show a radar overlay with nearby players.");
							if (bRadar) {
								ImGui::CoolCheckbox(TEXT("Display players"), &bRadarDisplayPlayers);
								WithTooltip("Show enemy player markers on the radar.");
								ImGui::CoolCheckbox(TEXT("Outlined"), &bRadarOutline);
								WithTooltip("Adds a contrasting border around the radar for better visibility.");
								ImGui::CoolCheckbox(TEXT("Circular"), &Options::bCircularRadar);
								WithTooltip("Switch the radar shape between square and circular.");
								ImGui::CoolCheckbox(TEXT("Line of sight"), &bShowLineOfSight);
								WithTooltip("Highlights players who currently have line of sight on you.");
								ImGui::CoolCheckbox(TEXT("Show distance"), &bUseDistanceText);
								WithTooltip("Print each player's distance next to their dot on the radar.");
							}
							if (bRadar) {
								ImGui::SliderFloat(fRadarRange, TEXT("Range"), &fRadarRange, 7.f, 50.f);
								WithTooltip("How far the radar reaches, in metres.");
								ImGui::SliderFloat(fPositionX, TEXT("Mx"), &fPositionX, 1.f, (float)(CenterX * 2));
								WithTooltip("Radar X position on screen.");
								ImGui::SliderFloat(fPositionY, TEXT("My"), &fPositionY, 1.f, (float)(CenterY * 2));
								WithTooltip("Radar Y position on screen.");
							}
						}
						ImGui::EndChild();
						ImGui::SameLine();
						bool radar2Open = ImGui::BeginChild(TEXT("C_Radar_Tab_Two"), mvec(340, 390));
						if (radar2Open) {
							InfoSection(TEXT("GENERAL"));
							if (bRadar) {
								ImGui::CoolCheckbox(TEXT("Include local player"), &bShowLocalPlayer);
								WithTooltip("Draw your own marker at the centre of the radar.");
								ImGui::SliderFloat(fSize, TEXT("Size"), &fSize, 1.f, 700);
								WithTooltip("Pixel size of the radar overlay.");
								ImGui::SliderFloat(fTransparency, TEXT("Transparent"), &fTransparency, 1.f, 255.f);
								WithTooltip("Background opacity of the radar (1 = invisible, 255 = solid).");
							}
						}
						ImGui::EndChild();
					}
					if (VisualPage == 3) {
						ImGui::Separator();
						bool crossOneOpen = ImGui::BeginChild(TEXT("C_Crosshair_Visuals"), mvec(340, 400));
						if (crossOneOpen) {
							ImGui::Dummy(mvec(0, 8));
							InfoSection(TEXT("CROSSHAIR"));
							ImGui::CoolCheckbox(TEXT("Enabled"), &Options::bCrosshair);
							WithTooltip("Toggle the center-screen crosshair.");
							if (Options::bCrosshair) {
								static float id_crossLen = 501.0f;
								static float id_crossThk = 502.0f;
								ImGui::Text(TEXT("Length"));
								ImGui::SliderFloat(id_crossLen, "##CrosshairLen", &Options::fCrosshairLength, 2.0f, 50.0f);
								WithTooltip("Total length of the crosshair lines.");
								ImGui::Text(TEXT("Thickness"));
								ImGui::SliderFloat(id_crossThk, "##CrosshairThk", &Options::fCrosshairThickness, 1.0f, 10.0f);
								WithTooltip("Pixel thickness of the crosshair lines.");
							}
						}
						ImGui::EndChild();
						ImGui::SameLine();
						bool menuKeyOpen = ImGui::BeginChild(TEXT("C_MenuKey_Panel"), mvec(340, 400));
						if (menuKeyOpen) {
							ImGui::Dummy(mvec(0, 8));
							InfoSection(TEXT("MENU KEY"));
							static const struct { const char* name; int vk; } menuKeys[] = {
								{ "Insert", VK_INSERT }, { "Home", VK_HOME }, { "End", VK_END },
								{ "Delete", VK_DELETE }, { "Page Up", VK_PRIOR }, { "Page Down", VK_NEXT },
								{ "F1", VK_F1 }, { "F2", VK_F2 }, { "F3", VK_F3 }, { "F4", VK_F4 },
								{ "F5", VK_F5 }, { "F6", VK_F6 }, { "F7", VK_F7 }, { "F8", VK_F8 },
								{ "F9", VK_F9 }, { "F10", VK_F10 }, { "F11", VK_F11 }, { "F12", VK_F12 },
							};
							static const char* menuKeyNames[] = {
								"Insert","Home","End","Delete","Page Up","Page Down",
								"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"
							};
							int selectedKey = 0;
							for (int i = 0; i < IM_ARRAYSIZE(menuKeys); i++) {
								if (menuKeys[i].vk == Options::iMenuKey) { selectedKey = i; break; }
							}
							ImGui::Text(TEXT("Toggle menu with:"));
							if (ImGui::Combo(TEXT("##MenuKeySelect"), &selectedKey, menuKeyNames, IM_ARRAYSIZE(menuKeyNames))) {
								Options::iMenuKey = menuKeys[selectedKey].vk;
							}
							WithTooltip("Key that opens and closes this menu.");
						}
						ImGui::EndChild();
					}
				}
				ImGui::EndChild();
			}
		}
	}
	{
		if (MenuTab == 3) {
			if (ImGui::BeginChild(TEXT("C_Settings_Tab"), mvec(730, 470))) {
				ImGui::Text(TEXT(""));

				if (bShowVisualButtons)
				{
					bool infoActive = (SettingPage == 0);
					if (infoActive) {
						ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Information"), mvec(120, 22));
					}
					else {
						ImGui::CoolButton(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), TEXT("Information"), mvec(120, 22));
					}

					if (infoActive) {
						DrawActiveTabUnderline(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
					}
					if (ImGui::IsItemClicked()) {
						SettingPage = 0;
					}
					if (ImGui::IsItemHovered()) {
					TestText = TEXT("General cheat information.");
				}
			}
			if (SettingPage == 0) {
					bool infoOneOpen = ImGui::BeginChild(TEXT("C_Info_One"), mvec(340, 400));
					if (infoOneOpen) {
						ImGui::Dummy(mvec(0, 8));

						InfoSection(TEXT("INFORMATION"));
						InfoRow(TEXT("Cheat version"), TEXT("1.1"), ImVec4(0.557f, 0.518f, 0.890f, 1.0f));
						InfoRow(TEXT("Game version"),  TEXT("41.00"), ImVec4(0.557f, 0.518f, 0.890f, 1.0f));

						ImGui::Dummy(mvec(0, 12));

						InfoSection(TEXT("STATUS"));
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 165, 180, 220));
						ImGui::TextUnformatted(TEXT("Ban chance"));
						ImGui::PopStyleColor();
						ImGui::SameLine(150.0f);
						StatusChip(TEXT("Low"), ImVec4(0.25f, 0.90f, 0.55f, 1.0f));

						ImGui::Dummy(mvec(0, 12));


					}
					ImGui::EndChild();
					ImGui::SameLine();
					bool infoTwoOpen = ImGui::BeginChild(TEXT("C_Info_Two"), mvec(340, 400));
					if (infoTwoOpen) {
						ImGui::Dummy(mvec(0, 8));
						InfoSection(TEXT("CHANGELOG"));
						ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 210, 220, 230));
						ImGui::BulletText(TEXT("Updated to the latest game patch"));
						ImGui::BulletText(TEXT("Redesigned menu UI"));
						ImGui::BulletText(TEXT("Improved sidebar animations"));
						ImGui::PopStyleColor();
					}
					ImGui::EndChild();
				}
			}
			ImGui::EndChild();
		}

		if (MenuTab == 4) {
			if (ImGui::BeginChild(TEXT("C_Config_Tab"), mvec(730, 480))) {

				ImGui::Dummy(mvec(0, 6));
				InfoSection(TEXT("CONFIG MANAGEMENT"));
				ImGui::Dummy(mvec(0, 6));

				static int selectedConfig = 0;
				static const char* feedbackMsg = nullptr;
				static int feedbackTimer = 0;
				static bool configListDirty = true;
				static std::vector<std::string> configNames;
				static std::vector<const char*> configItems;

				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::system_clock::to_time_t(now);
				char timeBuf[64];
				struct tm timeInfo;
				localtime_s(&timeInfo, &time);
				strftime(timeBuf, sizeof(timeBuf), "%Y_%m_%d_%H_%M_%S", &timeInfo);
				std::string autoConfigName = std::string("config_") + timeBuf;

				if (configListDirty) {
					configListDirty = false;
					configNames.clear();
					configItems.clear();

					Config::EnsureDirectory();
					configNames = Config::List();

					if (configNames.empty()) {
						Config::Save("default");
						configNames.push_back("default");
						Config::SaveSelectedConfig("default");
					}

					std::string savedName = Config::LoadSelectedConfig();

					for (size_t i = 0; i < configNames.size(); i++) {
						configItems.push_back(configNames[i].c_str());
						if (configNames[i] == savedName) {
							selectedConfig = i;
						}
					}

					if (selectedConfig >= (int)configItems.size()) {
						selectedConfig = 0;
					}
				}

				float listW = 420.0f;
				float availW = ImGui::GetContentRegionAvail().x;
				ImGui::SetCursorPosX((availW - listW) * 0.5f);
				ImGui::PushItemWidth(listW);
				if (ImGui::ListBox("##ConfigSelect", &selectedConfig, configItems.data(), (int)configItems.size(), 8)) {
					if (selectedConfig >= 0 && selectedConfig < (int)configNames.size()) {
						if (Config::Load(configNames[selectedConfig].c_str())) {
							ConfigUI::syncBoneOnLoad = true;
							ImGui::ClearCheckboxAnimations();
							Config::SaveSelectedConfig(configNames[selectedConfig].c_str());
							feedbackMsg = "Loaded!";
						} else {
							feedbackMsg = "Load Failed!";
						}
						feedbackTimer = 60;
					}
				}
				ImGui::PopItemWidth();

				ImGui::Dummy(mvec(0, 10));

				float btnH = 30.0f;
				float btnPad = 8.0f;
				float rowW = 120.0f + btnPad + 90.0f + btnPad + 90.0f + btnPad + 90.0f;
				ImGui::SetCursorPosX((availW - rowW) * 0.5f);

				if (ImGui::CoolButton(ImVec4(0.0f, 0.7f, 0.45f, 1.0f), TEXT("New Config"), mvec(120, btnH))) {
					Config::Save(autoConfigName.c_str());
					Config::Load(autoConfigName.c_str());
					ConfigUI::syncBoneOnLoad = true;
					ImGui::ClearCheckboxAnimations();
					Config::SaveSelectedConfig(autoConfigName.c_str());
					configListDirty = true;
					feedbackMsg = "Created!";
					feedbackTimer = 60;
				}
				ImGui::SameLine(0.0f, btnPad);
				if (ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Save"), mvec(90, btnH))) {
					if (selectedConfig >= 0 && selectedConfig < (int)configNames.size()) {
						if (Config::Save(configNames[selectedConfig].c_str())) {
							Config::SaveSelectedConfig(configNames[selectedConfig].c_str());
							feedbackMsg = "Saved!";
						} else {
							feedbackMsg = "Save Failed!";
						}
						feedbackTimer = 60;
					}
				}
				ImGui::SameLine(0.0f, btnPad);
				if (ImGui::CoolButton(ImVec4(0.557f, 0.518f, 0.890f, 1.0f), TEXT("Load"), mvec(90, btnH))) {
					if (selectedConfig >= 0 && selectedConfig < (int)configNames.size()) {
						if (Config::Load(configNames[selectedConfig].c_str())) {
							ConfigUI::syncBoneOnLoad = true;
							ImGui::ClearCheckboxAnimations();
							Config::SaveSelectedConfig(configNames[selectedConfig].c_str());
							feedbackMsg = "Loaded!";
						} else {
							feedbackMsg = "Load Failed!";
						}
						feedbackTimer = 60;
					}
				}
				ImGui::SameLine(0.0f, btnPad);
				if (ImGui::CoolButton(ImVec4(0.85f, 0.25f, 0.20f, 1.0f), TEXT("Delete"), mvec(90, btnH))) {
					if (selectedConfig >= 0 && selectedConfig < (int)configNames.size() && configNames.size() > 1) {
						std::string toDelete = configNames[selectedConfig];
						Config::Delete(toDelete.c_str());
						configListDirty = true;
						feedbackMsg = "Deleted!";
						feedbackTimer = 60;
					}
				}

				ImGui::Dummy(mvec(0, 8));
				if (feedbackTimer > 0) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 220, 140, 255));
					ImVec2 fbSize = ImGui::CalcTextSize(feedbackMsg);
					ImGui::SetCursorPosX((availW - fbSize.x) * 0.5f);
					ImGui::TextUnformatted(feedbackMsg);
					ImGui::PopStyleColor();
					feedbackTimer--;
				}

				ImGui::EndChild();
			}
		}
		if (MenuTab == 5) {
			RenderAimbotTab();
		}
		ImGui::PopStyleVar();
	}
	ImGui::End();

	{
		for (int i = 1; i <= 5; i++) {
			float off = (float)i;
			int a = (int)(38 * (1.0f - (i - 1) / 5.0f));
			m->AddRect(
				mvec((float)m_pos_x - off, (float)m_pos_y - off + 2.0f),
				mvec((float)(m_pos_x + m_size_x) + off, (float)(m_pos_y + m_size_y) + off + 2.0f),
				IM_COL32(0, 0, 0, a), 12.0f + off, 0, 1.0f);
		}
	}

	ImVec4 titleRgbColor;
	if (Options::RGBMode::bEnabled && Options::PlayerColor::bTitleTextRGB) {
		float hue = fmodf(Options::RGBMode::rgbTime, 1.0f);
		ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, titleRgbColor.x, titleRgbColor.y, titleRgbColor.z);
		titleRgbColor.w = 1.0f;
	}

	ImFont* titleFontMain = Fonts::SegoeBold ? Fonts::SegoeBold
	                      : (Fonts::TahomaBold ? Fonts::TahomaBold : ImGui::GetFont());
	float titleSizeMain = titleFontMain->FontSize;
	mvec Size66 = titleFontMain->CalcTextSizeA(titleSizeMain, FLT_MAX, 0.0f, TEXT("Fortnite Hydra"), nullptr);
	ImVec4 titleColor = (Options::RGBMode::bEnabled && Options::PlayerColor::bTitleTextRGB) ? titleRgbColor : ImVec4(Options::PlayerColor::TitleText[0], Options::PlayerColor::TitleText[1], Options::PlayerColor::TitleText[2], Options::PlayerColor::TitleText[3]);
	m->AddText(titleFontMain, titleSizeMain,
		mvec(m_pos_x + (m_size_x / 2) - (Size66.x / 2), m_pos_y + 7),
		ImGui::GetColorU32(titleColor), TEXT("Fortnite Hydra"));

	{
		float lineY = (float)(m_pos_y + 28);
		float lineStart = (float)(m_pos_x + 15);
		float lineEnd = (float)(m_pos_x + m_size_x - 15);
		float lineMid = (lineStart + lineEnd) / 2.0f;
		float halfLen = (lineEnd - lineStart) / 2.0f;
		float accentAlpha = 0.55f + 0.15f * g_glowPulse;

		m->AddRectFilledMultiColor(
			mvec(lineStart, lineY), mvec(lineMid, lineY + 1.0f),
			IM_COL32(142, 132, 227, 0), IM_COL32(142, 132, 227, (int)(255 * accentAlpha)),
			IM_COL32(142, 132, 227, (int)(255 * accentAlpha)), IM_COL32(142, 132, 227, 0));

		m->AddRectFilledMultiColor(
			mvec(lineMid, lineY), mvec(lineEnd, lineY + 1.0f),
			IM_COL32(142, 132, 227, (int)(255 * accentAlpha)), IM_COL32(142, 132, 227, 0),
			IM_COL32(142, 132, 227, 0), IM_COL32(142, 132, 227, (int)(255 * accentAlpha)));
	}

	if (MenuTab == 1 && (VisualPage == 0 || VisualPage == 1)) {

		{
			int lenght = 250;
			float x1 = g_mirrorLayout
				? (float)(m_pos_x - (lenght + 25))
				: (float)(m_pos_x + m_size_x + 25);
			float y1 = (float)m_pos_y;
			float x2 = x1 + (float)lenght;
			float y2 = (float)(m_pos_y + m_size_y);
			for (int i = 1; i <= 5; i++) {
				float off = (float)i;
				int a = (int)(38 * (1.0f - (i - 1) / 5.0f));
				m->AddRect(
					mvec(x1 - off, y1 - off + 2.0f),
					mvec(x2 + off, y2 + off + 2.0f),
					IM_COL32(0, 0, 0, a), 12.0f + off, 0, 1.0f);
			}
		}

		{
			int lenght = 250;
			float px1 = g_mirrorLayout
				? (float)(m_pos_x - (lenght + 25))
				: (float)(m_pos_x + m_size_x + 25);
			float py1 = (float)m_pos_y;
			float px2 = px1 + (float)lenght;
			float py2 = (float)(m_pos_y + m_size_y);
			float bracketLen = 20.0f + 5.0f * g_glowPulse;
			ImU32 bracketCol = IM_COL32(142, 132, 227, (int)(180 * g_glowPulse));
			float bThick = 1.5f;

			m->AddLine(mvec(px1, py1), mvec(px1 + bracketLen, py1), bracketCol, bThick);
			m->AddLine(mvec(px1, py1), mvec(px1, py1 + bracketLen), bracketCol, bThick);

			m->AddLine(mvec(px2, py1), mvec(px2 - bracketLen, py1), bracketCol, bThick);
			m->AddLine(mvec(px2, py1), mvec(px2, py1 + bracketLen), bracketCol, bThick);

			m->AddLine(mvec(px1, py2), mvec(px1 + bracketLen, py2), bracketCol, bThick);
			m->AddLine(mvec(px1, py2), mvec(px1, py2 - bracketLen), bracketCol, bThick);

			m->AddLine(mvec(px2, py2), mvec(px2 - bracketLen, py2), bracketCol, bThick);
			m->AddLine(mvec(px2, py2), mvec(px2, py2 - bracketLen), bracketCol, bThick);
		}
	}
	ImGui::PopStyleVar();
	ImGui::PopFont();
}

void RenderAimbotTab() {
	using namespace Options;

	if (ImGui::BeginChild(TEXT("C_Aimbot_Tab"), mvec(730, 470), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
		ImGui::Text(TEXT(""));
		if (bShowVisualButtons)
		{
			float tabW = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX((tabW - 120.0f) * 0.5f);
			if (AimbotPage == 0) {
				ImGui::CoolButton(ImVec4(0.85f, 0.15f, 0.15f, 1.0f), TEXT("All weapons"), mvec(120, 22));
				DrawActiveTabUnderline(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			}
			else {
				ImGui::CoolButton(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), TEXT("All weapons"), mvec(120, 22));
			}
			if (ImGui::IsItemClicked()) {
				AimbotPage = 0;
			}
			if (ImGui::IsItemHovered()) {
				TestText = TEXT("Mouse Aimbot Config for all weapons.");
			}
		}
		if (AimbotPage == 0) {
			ImGui::Dummy(mvec(0, 3));
			ImGui::Separator();
			ImGui::Dummy(mvec(0, 3));

			using namespace Options::AssaultRifle;

			auto aimColorFlags = ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoTooltip |
				ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions;

			if (ImGui::BeginChild(TEXT("C_Aimbot_Left"), mvec(355, 0))) {
				InfoSection(TEXT("AIMBOT"));
				ImGui::CoolCheckbox(TEXT("Enable"), &bAimbot);
				WithTooltip("PlayerController + PlayerAimOffset (cooking path, 0x2220).");
				ImGui::CoolCheckbox(TEXT("Skip Knocked Players"), &bKnockedPlayers);
				WithTooltip("Ignore downed players when picking targets.");
				ImGui::CoolCheckbox(TEXT("Skip Teammates"), &bSkipTeammates);
				WithTooltip("Ignore players on your team when picking targets.");
				ImGui::CoolCheckbox(TEXT("Visible Only"), &bVisibleOnly);
				WithTooltip("Only target enemies your character can see.");

				ImGui::CoolCheckbox(TEXT("Prediction"), &bPrediction);
				WithTooltip("Predicts target movement and bullet drop using projectile speed and gravity.");
				ImGui::CoolCheckbox(TEXT("Shoot Through Walls"), &bShootThroughWall);
				WithTooltip("Three-ways to shit on kids");
				ImGui::CoolCheckbox(TEXT("Raycast Visibility"), &bRaycastVisibility);
				WithTooltip("Chaos physics raycasting for precise wall/cover detection");
				ImGui::CoolCheckbox(TEXT("Debug Raycast Geo"), &bRenderRaycastGeometry);
				WithTooltip("Render wireframe of cached Chaos geometry");

				ImGui::Dummy(mvec(0, 6));
				ImGui::Text(TEXT("Field of View"));
				static float id_aimFov = 701.0f;
				ImGui::SliderFloat(id_aimFov, "##AimFOV", &fAimbotFOV, 5.0f, 460.0f);
				WithTooltip("Maximum pixel radius from crosshair to acquire a target.");

				ImGui::Text(TEXT("Smoothness"));
				static float id_aimSmooth = 702.0f;
				ImGui::SliderFloat(id_aimSmooth, "##AimSmooth", &fAimbotSmooth, 3.0f, 30.0f);
				WithTooltip("Lower = locky / snappier. Higher = smoother / more delayed.");

				ImGui::Dummy(mvec(0, 6));
				InfoSection(TEXT("TARGET BONE"));
				static const char* boneItems[] = { "Closest Bone", "Head", "Neck", "Body", "Right Thigh", "Left Thigh" };
				static int selectedBone = 0;
				if (ConfigUI::syncBoneOnLoad || AimTarget::bClosestBone) {
					selectedBone = 0;
					ConfigUI::syncBoneOnLoad = false;
				} else {
					if (current_hitbox == BoneIDS::head) selectedBone = 1;
					else if (current_hitbox == BoneIDS::neck) selectedBone = 2;
					else if (current_hitbox == BoneIDS::chest) selectedBone = 3;
					else if (current_hitbox == BoneIDS::rightThigh) selectedBone = 4;
					else if (current_hitbox == BoneIDS::leftThigh) selectedBone = 5;
					else selectedBone = 0;
				}
				if (ImGui::Combo(TEXT("##TargetBone"), &selectedBone, boneItems, IM_ARRAYSIZE(boneItems))) {
					if (selectedBone == 0) {
						AimTarget::bClosestBone = true;
					} else {
						AimTarget::bClosestBone = false;
						switch (selectedBone) {
						case 1: current_hitbox = BoneIDS::head; break;
						case 2: current_hitbox = BoneIDS::neck; break;
						case 3: current_hitbox = BoneIDS::chest; break;
						case 4: current_hitbox = BoneIDS::rightThigh; break;
						case 5: current_hitbox = BoneIDS::leftThigh; break;
						}
					}
				}
				WithTooltip("Bone the aimbot targets. Closest Bone auto-picks whichever bone is nearest your crosshair.");
			}
			ImGui::EndChild();
			ImGui::SameLine();

			if (ImGui::BeginChild(TEXT("C_Aimbot_Right"), mvec(355, 0))) {
				InfoSection(TEXT("FOV CIRCLE"));
				ImGui::CoolCheckbox(TEXT("Show"), &bFillFOV);
				WithTooltip("Draws a circle at your crosshair representing the aimbot's reach.");
				if (bFillFOV) {
					ImGui::CoolCheckbox(TEXT("Filled"), &bFillFOVInside);
					WithTooltip("Tints the inside of the FOV circle.");
					ImGui::ColorEdit4(TEXT("Color"), FOVCircleColor, aimColorFlags);
					WithTooltip("Color and opacity of the FOV circle (when RGB exclusion is enabled for FOV).");
					ImGui::Text(TEXT("Thickness"));
					static float id_fovThk = 703.0f;
					ImGui::SliderFloat(id_fovThk, "##FOVThk", &fFOVCircleThickness, 1.0f, 5.0f);
					WithTooltip("Pixel thickness of the FOV circle outline.");
				}

				ImGui::Dummy(mvec(0, 6));
				InfoSection(TEXT("AIMBOT LINE"));
				ImGui::CoolCheckbox(TEXT("Enable"), &bAimbotLine);
				WithTooltip("Draws a line from your crosshair to the active target bone.");
				if (bAimbotLine) {
					ImGui::CoolCheckbox(TEXT("Only While Aiming"), &bAimbotLineHoldWhileAiming);
					WithTooltip("Only show the line while your aim key is held.");
					ImGui::Text(TEXT("Thickness"));
					static float id_lineThk = 704.0f;
					ImGui::SliderFloat(id_lineThk, "##LineThk", &fAimbotLineThickness, 1.0f, 5.0f);
					WithTooltip("Pixel thickness of the aimbot line.");
				}

			ImGui::Dummy(mvec(0, 6));
			InfoSection(TEXT("KEYBINDS"));
				ImGui::CoolCheckbox(TEXT("Shift"), &Aimkey::bShift); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("Ctrl"), &Aimkey::bCtrl); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("Tab"), &Aimkey::bTab); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("Caps"), &Aimkey::bCapsLock);
				ImGui::CoolCheckbox(TEXT("M4"), &Aimkey::bMouse4); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("M5"), &Aimkey::bMouse5); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("RMB"), &Aimkey::bMouseRight); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("Q"), &Aimkey::bQ); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("E"), &Aimkey::bE);
				ImGui::CoolCheckbox(TEXT("R"), &Aimkey::bR); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("F"), &Aimkey::bF); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("T"), &Aimkey::bT); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("G"), &Aimkey::bG);
				ImGui::CoolCheckbox(TEXT("V"), &Aimkey::bV); ImGui::SameLine();
				ImGui::CoolCheckbox(TEXT("B"), &Aimkey::bB);
			}
			ImGui::EndChild();
		}
	}
	ImGui::EndChild();
}

namespace Menu {
	VOID RenderMenu(HWND Window) {
		if (GetAsyncKeyState(Options::iMenuKey) & 1) {
			bIsMenuOpen = !bIsMenuOpen;
		}

		if (bIsMenuOpen) {
			if (!g_menuWasOpen) {
				g_menuOpenAnim = 0.0f;
				g_menuWasOpen = true;
				ClipCursor(NULL);
				while (ShowCursor(TRUE) < 0);
				SetCursor(LoadCursor(NULL, IDC_ARROW));
			}
			MakeMenu();
		}
		else {
			if (g_menuWasOpen) {
				while (ShowCursor(FALSE) >= 0);
			}
			g_menuWasOpen = false;
		}
	}
}
