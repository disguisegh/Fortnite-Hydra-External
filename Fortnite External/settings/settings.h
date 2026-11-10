#pragma once
#include "../Menu/imgui/imgui.h"
#include <string>

namespace settings {
	inline int width = GetSystemMetrics(SM_CXSCREEN);
	inline int height = GetSystemMetrics(SM_CYSCREEN);
	inline const float CenterX = static_cast<float>(width) / 2.0f;
	inline const float CenterY = static_cast<float>(height) / 2.0f;
	inline int screen_center_x = width / 2;
	inline int screen_center_y = height / 2;
	ImColor VisibleColor = ImColor(255, 255, 255, 255);
	ImColor NVisibleColor = ImColor(255, 0, 0, 255);
	bool Box = true;
	bool Snapline = false;
	bool Distance = false;
	bool Menu = false;
	bool VSync = true;
}

enum BoneIDS {
	head = 110,
	neck = 67,
	pelvis = 2,
	stomach = 5,
	chest = 7,
	rightChest = 8,
	rightFoot = 76,
	rightThigh = 77,
	leftFoot = 83,
	leftThigh = 84,
};

namespace Options {

	bool bRadar = true;
	bool bRectangle = true;
	bool bUseTeamColor = true;
	bool bUseVisibleColor = false;
	bool bUseDistanceText = true;
	bool bShowLocalPlayer = true;
	bool bShowLineOfSight = true;
	bool bRadarOutline = true;
	bool bRadarDisplayPlayers = true;
	float fPositionX = 28.9470005f;
	float fPositionY = 48.1409988f;
	float fSize = 174.0529938f;
	float fTransparency = 230.0000000f;
	float fRadarRange = 10.0000000f;

	bool bCharacters = true;
	bool bPlayerInfo = true;
	bool bViewAngles = true;
	bool bSkeleton = true;
	bool bSkeletonFeet = true;
	bool bSmoothSkeleton = true;
	namespace SkeletonSmooth {
		float fStep = 0.0200000f;
	}
	bool bSnapline = false;
	bool bPlayerDistance = true;
	bool bOnBots = true;
	bool bOnTeammate = false;
	bool bTeamBar = false;
	bool bplayername = true;
	bool bplattform = true;
	float fSkeletonThickness = 1.5000000f;
	bool bTeamIndex = true;
	bool bTeamScore = false;
	bool bSeasonRank = false;
	bool bWeaponName = true;

	float fMaxPlayerDistance = 200.0000000f;
	namespace PlayerFonts {
		bool bStandardFont = false;
		bool bGTAFont = false;
		bool bFortniteFont = false;
		bool bArial = false;
		bool bRajdhani = false;
		bool bVerdana = false;
		bool bTahomaBold = true;
	}
	namespace PlayerFeatures {
		bool OutlinedText = true;
		bool TextBackground = false;
		bool bSkeletonOutline = true;
		bool bSnaplineOutline = true;
		float fSnaplineOutlineThickness = 2.9219999f;
	}
	namespace PlayerColor {
		float BoxVisible[4]                = {0.6094676f, 0.9903846f, 0.0000000f, 1.0000000f};
		float BoxNotVisible[4]             = {1.0000000f, 0.0000000f, 0.9807696f, 1.0000000f};
		float BoxFillColorVisible[4]        = {0.0000000f, 0.0000000f, 0.0000000f, 0.3000000f};
		float BoxFillColorNotVisible[4]    = {0.0000000f, 0.0000000f, 0.0000000f, 0.3000000f};
		float BoxFillRGBColor[4]           = {1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f};
		bool bBoxFillColorRGB = false;
		bool bBoxColorRGB = false;
		bool bSkeletonColorRGB = false;
		bool bSkeletonOutlineColorRGB = true;
		bool bSnaplineColorRGB = false;
		bool bTopTextColorRGB = false;
		bool bBottomTextColorRGB = false;
		bool bTitleTextRGB = true;

		float Skeleton[4]                  = {0.8777738f, 0.9711539f, 0.0000000f, 1.0000000f};
		float Snapline[4]                  = {1.0000000f, 0.9230769f, 0.0000000f, 1.0000000f};
		float TopText[4]                   = {1.0000000f, 0.9230769f, 0.0000000f, 1.0000000f};
		float BottomText[4]                = {1.0000000f, 0.9230769f, 0.0000000f, 1.0000000f};

		float SkeletonVisible[4]           = {0.8777738f, 0.9711539f, 0.0000000f, 1.0000000f};
		float SkeletonNotVisible[4]        = {0.7884612f, 0.0000000f, 1.0000000f, 1.0000000f};
		float SkeletonOutlineVisible[4]     = {0.0000000f, 0.0000000f, 0.0000000f, 1.0000000f};
		float SkeletonOutlineNotVisible[4]  = {0.0000000f, 0.0000000f, 0.0000000f, 1.0000000f};
		float SkeletonRGBColor[4]          = {1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f};
		float SkeletonOutlineRGBColor[4]   = {0.0000000f, 0.0000000f, 0.0000000f, 1.0000000f};

		float SnaplineVisible[4]           = {1.0000000f, 0.9230769f, 0.0000000f, 1.0000000f};
		float SnaplineNotVisible[4]        = {0.7884612f, 0.0000000f, 1.0000000f, 1.0000000f};
		float SnaplineRGBColor[4]          = {1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f};

		float TopTextVisible[4]            = {0.7307692f, 0.0000000f, 1.0000000f, 1.0000000f};
		float TopTextNotVisible[4]        = {0.9038463f, 0.0000000f, 1.0000000f, 1.0000000f};
		float TopTextRGBColor[4]           = {1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f};

		float BottomTextVisible[4]        = {0.9038463f, 1.0000000f, 0.0000000f, 1.0000000f};
		float BottomTextNotVisible[4]       = {0.9615383f, 0.0000000f, 1.0000000f, 1.0000000f};
		float BottomTextRGBColor[4]        = {1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f};

		float TitleText[4]                 = {0.9200000f, 0.9200000f, 0.3000000f, 1.0000000f};
	}
	namespace RGBMode {
		bool bEnabled = true;
		float fSpeed = 0.3000000f;
		float rgbTime = 0.0f;
	}
namespace CharacterModes {
	int box_type = 2;
	float fBoxThickness = 0.3f;
}
	bool bCircularRadar = true;
	namespace SnaplineStart {
		bool bTop = true;
		bool bBottom = false;
	}
	bool bStreamProof = false;
	bool bCrosshair = true;
	bool bDisableGameCrosshair = false;
	float fCrosshairLength = 10.0f;
	float fCrosshairThickness = 2.0f;
	int iMenuKey = VK_INSERT;

 }

namespace Options::AssaultRifle {
	bool bAimbot = false;
	float fAimbotFOV = 180.0f;
	float fAimbotSmooth = 8.0f;
	bool bAimbotLine = false;
	float fAimbotLineThickness = 1.0f;
	bool bAimbotLineHoldWhileAiming = false;
	bool bFillFOV = false;
	bool bVisibleOnly = false;
	bool bPrediction = false;
	bool bShootThroughWall = false;
	bool bRaycastVisibility = false;
	bool bRenderRaycastGeometry = false;
	bool bKnockedPlayers = false;
	bool bSkipTeammates = false;
	bool bAimbotDebug = false;
	bool bMouseAim = false;
	bool bMemoryAim = true;
	bool bZeroReticleOffsets = false;

	bool bInvertPitch = false;
	bool bInvertYaw = false;
	float fAimHeightOffset = 0.f;
	float fAimDeadzone = 2.f;
	float fAimMaxPixels = 90.f;
	float fAimMaxDegrees = 6.f;
	BoneIDS current_hitbox = BoneIDS::head;
	float FOVCircleColor[4] = { 1.0f, 1.0f, 1.0f, 0.6f };
	float FOVCircleRGBColor[4] = { 1.0f, 1.0f, 1.0f, 0.6f };
	bool bFOVCircleRGB = false;
	float fFOVCircleThickness = 2.0f;
	bool bFillFOVInside = false;
}

namespace Aimkey {

	bool bShift = false, bMouse4 = false, bMouse5 = true;
	bool bMouseRight = true;
	bool bCapsLock = false, bR = false, bF = false, bE = false;
	bool bG = false, bQ = false, bT = false, bV = false;
	bool bB = false, bCtrl = false, bTab = false;
}

namespace AimTarget {
	bool bClosestBone = true;
}

namespace PlayerColor {
	float AimbotLineVisible[4]    = {1.0000000f, 0.9807692f, 0.0000000f, 1.0000000f};
	float AimbotLineNotVisible[4]  = {1.0000000f, 0.0000000f, 0.9807696f, 1.0000000f};
	float AimbotLineRGBColor[4]   = {1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f};
	bool bAimbotLineColorRGB = false;
	bool bAimbotLineRGB = false;
}

namespace Fonts
{
	ImFont* StandardFont;
	ImFont* GTAFont;
	ImFont* FortniteFont;
	ImFont* Arial;
	ImFont* Rajdhani;
	ImFont* Verdana;
	ImFont* TextureFont;
	ImFont* NormalFont;
	ImFont* HydraFont;
	ImFont* TahomaBold;
	ImFont* SegoeRegular;
	ImFont* SegoeSemi;
	ImFont* SegoeBold;
	ImFont* DisplayBold;
}
