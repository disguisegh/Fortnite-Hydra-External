#pragma once
#include "../settings/settings.h"
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <functional>
#include "../Menu/imgui/useless-shit-lowkey/CheatEncryption.h"

namespace Config {
    const std::string configPath = TEXT("C:\\5ayIJAtJEe_configs\\");
    std::string currentConfig = "default";
    std::string selectedConfigName = "default";

    void EnsureDirectory() {
        if (!std::filesystem::exists(configPath)) {
            std::filesystem::create_directories(configPath);
        }
    }

    void SaveSelectedConfig(const std::string& name) {
        selectedConfigName = name;
        std::ofstream file(configPath + "selected.cfg");
        if (file.is_open()) {
            file << name;
            file.close();
        }
    }

    std::string LoadSelectedConfig() {
        std::ifstream file(configPath + "selected.cfg");
        if (file.is_open()) {
            std::string name;
            std::getline(file, name);
            file.close();
            return name;
        }
        return "default";
    }

    bool Exists(const std::string& name) {
        std::string filename = configPath + name + ".json";
        return std::filesystem::exists(filename);
    }

    std::vector<std::string> List() {
        std::vector<std::string> configs;
        EnsureDirectory();

        for (const auto& entry : std::filesystem::directory_iterator(configPath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(".json") != std::string::npos) {
                    std::string name = filename.substr(0, filename.length() - 5);
                    configs.push_back(name);
                }
            }
        }
        return configs;
    }

    namespace {
        std::string boolToJson(bool b) {
            return b ? "true" : "false";
        }

        std::string floatToJson(float f) {
            std::ostringstream oss;
            oss << std::setprecision(7) << std::fixed << f;
            std::string s = oss.str();
            if (s.find('.') == std::string::npos && s.find('e') == std::string::npos) {
                s += ".0";
            }
            return s;
        }

        std::string colorToJson(float c[4]) {
            return "[" + floatToJson(c[0]) + "," + floatToJson(c[1]) + "," + floatToJson(c[2]) + "," + floatToJson(c[3]) + "]";
        }

        std::string trim(const std::string& s) {
            size_t start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \t\n\r");
            return s.substr(start, end - start + 1);
        }

        size_t findKey(const std::string& json, const std::string& key) {
            std::string searchKey = "\"" + key + "\"";
            size_t pos = 0;
            while ((pos = json.find(searchKey, pos)) != std::string::npos) {
                size_t after = pos + searchKey.length();
                if (after < json.length() && (json[after] == ':' || json[after] == '"'))
                    return pos;
                ++pos;
            }
            return std::string::npos;
        }

        bool extractBool(const std::string& json, const std::string& key, bool defaultVal) {
            size_t pos = findKey(json, key);
            if (pos == std::string::npos) return defaultVal;

            size_t colonPos = json.find(":", pos);
            if (colonPos == std::string::npos) return defaultVal;

            size_t start = colonPos + 1;
            size_t end = json.find_first_of(",}", start);
            if (end == std::string::npos) return defaultVal;

            std::string valueStr = trim(json.substr(start, end - start));
            if (valueStr == "true") return true;
            if (valueStr == "false") return false;
            return defaultVal;
        }

        float extractFloat(const std::string& json, const std::string& key, float defaultVal) {
            size_t pos = findKey(json, key);
            if (pos == std::string::npos) return defaultVal;

            size_t colonPos = json.find(":", pos);
            if (colonPos == std::string::npos) return defaultVal;

            size_t start = colonPos + 1;
            size_t end = json.find_first_of(",}", start);
            if (end == std::string::npos) return defaultVal;

            std::string valueStr = trim(json.substr(start, end - start));
            if (valueStr == "true") return 1.0f;
            if (valueStr == "false") return 0.0f;

            try {
                return std::stof(valueStr);
            } catch (...) {
                return defaultVal;
            }
        }

        int extractInt(const std::string& json, const std::string& key, int defaultVal) {
            size_t pos = findKey(json, key);
            if (pos == std::string::npos) return defaultVal;

            size_t colonPos = json.find(":", pos);
            if (colonPos == std::string::npos) return defaultVal;

            size_t start = colonPos + 1;
            size_t end = json.find_first_of(",}", start);
            if (end == std::string::npos) return defaultVal;

            std::string valueStr = trim(json.substr(start, end - start));
            try {
                return std::stoi(valueStr);
            } catch (...) {
                return defaultVal;
            }
        }

        void extractColor(const std::string& json, const std::string& key, float out[4]) {
            size_t pos = findKey(json, key);
            if (pos == std::string::npos) return;

            size_t colonPos = json.find(":", pos);
            if (colonPos == std::string::npos) return;

            size_t start = colonPos + 1;
            size_t end = json.find(']', start);
            if (end == std::string::npos) return;

            std::string arrayStr = json.substr(start, end - start + 1);
            if (arrayStr[0] == '[') arrayStr = arrayStr.substr(1);

            std::replace(arrayStr.begin(), arrayStr.end(), '[', ',');
            std::replace(arrayStr.begin(), arrayStr.end(), ']', ',');

            std::stringstream ss(arrayStr);
            std::string token;
            int i = 0;
            while (std::getline(ss, token, ',') && i < 4) {
                token = trim(token);
                if (!token.empty()) {
                    try {
                        out[i] = std::stof(token);
                    } catch (...) {

                    }
                    i++;
                }
            }
        }
    }

    std::string SerializeSettings() {
        std::string json = "{";

        json += "\"radar.enabled\":" + boolToJson(Options::bRadar) + ",";
        json += "\"radar.rectangle\":" + boolToJson(Options::bRectangle) + ",";
        json += "\"radar.useTeamColor\":" + boolToJson(Options::bUseTeamColor) + ",";
        json += "\"radar.useVisibleColor\":" + boolToJson(Options::bUseVisibleColor) + ",";
        json += "\"radar.useDistanceText\":" + boolToJson(Options::bUseDistanceText) + ",";
        json += "\"radar.showLocalPlayer\":" + boolToJson(Options::bShowLocalPlayer) + ",";
        json += "\"radar.showLineOfSight\":" + boolToJson(Options::bShowLineOfSight) + ",";
        json += "\"radar.outline\":" + boolToJson(Options::bRadarOutline) + ",";
        json += "\"radar.displayPlayers\":" + boolToJson(Options::bRadarDisplayPlayers) + ",";
        json += "\"radar.posX\":" + floatToJson(Options::fPositionX) + ",";
        json += "\"radar.posY\":" + floatToJson(Options::fPositionY) + ",";
        json += "\"radar.size\":" + floatToJson(Options::fSize) + ",";
        json += "\"radar.transparency\":" + floatToJson(Options::fTransparency) + ",";
        json += "\"radar.range\":" + floatToJson(Options::fRadarRange) + ",";

        json += "\"player.characters\":" + boolToJson(Options::bCharacters) + ",";
        json += "\"player.playerInfo\":" + boolToJson(Options::bPlayerInfo) + ",";
        json += "\"player.viewAngles\":" + boolToJson(Options::bViewAngles) + ",";
        json += "\"player.skeleton\":" + boolToJson(Options::bSkeleton) + ",";
        json += "\"player.skeletonFeet\":" + boolToJson(Options::bSkeletonFeet) + ",";
        json += "\"player.smoothSkeleton\":" + boolToJson(Options::bSmoothSkeleton) + ",";
        json += "\"player.skeletonStep\":" + floatToJson(Options::SkeletonSmooth::fStep) + ",";
        json += "\"player.snapline\":" + boolToJson(Options::bSnapline) + ",";
        json += "\"player.playerDistance\":" + boolToJson(Options::bPlayerDistance) + ",";
        json += "\"player.onBots\":" + boolToJson(Options::bOnBots) + ",";
        json += "\"player.onTeammate\":" + boolToJson(Options::bOnTeammate) + ",";
        json += "\"player.teamBar\":" + boolToJson(Options::bTeamBar) + ",";
        json += "\"player.playername\":" + boolToJson(Options::bplayername) + ",";
        json += "\"player.platform\":" + boolToJson(Options::bplattform) + ",";
        json += "\"player.skeletonThickness\":" + floatToJson(Options::fSkeletonThickness) + ",";
        json += "\"player.teamIndex\":" + boolToJson(Options::bTeamIndex) + ",";
        json += "\"player.teamScore\":" + boolToJson(Options::bTeamScore) + ",";
        json += "\"player.seasonRank\":" + boolToJson(Options::bSeasonRank) + ",";
        json += "\"player.weaponName\":" + boolToJson(Options::bWeaponName) + ",";
        json += "\"player.maxDistance\":" + floatToJson(Options::fMaxPlayerDistance) + ",";

        json += "\"player.fontStandard\":" + boolToJson(Options::PlayerFonts::bStandardFont) + ",";
        json += "\"player.fontGTA\":" + boolToJson(Options::PlayerFonts::bGTAFont) + ",";
        json += "\"player.fontFortnite\":" + boolToJson(Options::PlayerFonts::bFortniteFont) + ",";
        json += "\"player.fontArial\":" + boolToJson(Options::PlayerFonts::bArial) + ",";
        json += "\"player.fontRajdhani\":" + boolToJson(Options::PlayerFonts::bRajdhani) + ",";
        json += "\"player.fontVerdana\":" + boolToJson(Options::PlayerFonts::bVerdana) + ",";
        json += "\"player.fontTahomaBold\":" + boolToJson(Options::PlayerFonts::bTahomaBold) + ",";
        json += "\"player.outlinedText\":" + boolToJson(Options::PlayerFeatures::OutlinedText) + ",";
        json += "\"player.textBackground\":" + boolToJson(Options::PlayerFeatures::TextBackground) + ",";
        json += "\"player.skeletonOutline\":" + boolToJson(Options::PlayerFeatures::bSkeletonOutline) + ",";

        json += "\"player.snaplineOutline\":" + boolToJson(Options::PlayerFeatures::bSnaplineOutline) + ",";
        json += "\"player.snaplineOutlineThickness\":" + floatToJson(Options::PlayerFeatures::fSnaplineOutlineThickness) + ",";

        json += "\"player.boxType\":" + floatToJson((float)Options::CharacterModes::box_type) + ",";
        json += "\"player.boxThickness\":" + floatToJson(Options::CharacterModes::fBoxThickness) + ",";
        json += "\"player.boxFillColorVisible\":" + colorToJson(Options::PlayerColor::BoxFillColorVisible) + ",";
        json += "\"player.boxFillColorNotVisible\":" + colorToJson(Options::PlayerColor::BoxFillColorNotVisible) + ",";
        json += "\"player.boxFillRGBColor\":" + colorToJson(Options::PlayerColor::BoxFillRGBColor) + ",";
        json += "\"player.bBoxFillColorRGB\":" + boolToJson(Options::PlayerColor::bBoxFillColorRGB) + ",";
        json += "\"player.circularRadar\":" + boolToJson(Options::bCircularRadar) + ",";

        json += "\"player.snaplineTop\":" + boolToJson(Options::SnaplineStart::bTop) + ",";
        json += "\"player.snaplineBottom\":" + boolToJson(Options::SnaplineStart::bBottom) + ",";

        json += "\"player.boxVisible\":" + colorToJson(Options::PlayerColor::BoxVisible) + ",";
        json += "\"player.boxNotVisible\":" + colorToJson(Options::PlayerColor::BoxNotVisible) + ",";
        json += "\"player.bBoxColorRGB\":" + boolToJson(Options::PlayerColor::bBoxColorRGB) + ",";
        json += "\"player.bSkeletonColorRGB\":" + boolToJson(Options::PlayerColor::bSkeletonColorRGB) + ",";
        json += "\"player.bSkeletonOutlineColorRGB\":" + boolToJson(Options::PlayerColor::bSkeletonOutlineColorRGB) + ",";
        json += "\"player.bSnaplineColorRGB\":" + boolToJson(Options::PlayerColor::bSnaplineColorRGB) + ",";
        json += "\"player.bTopTextColorRGB\":" + boolToJson(Options::PlayerColor::bTopTextColorRGB) + ",";
        json += "\"player.bBottomTextColorRGB\":" + boolToJson(Options::PlayerColor::bBottomTextColorRGB) + ",";
        json += "\"player.bTitleTextRGB\":" + boolToJson(Options::PlayerColor::bTitleTextRGB) + ",";

        json += "\"player.skeletonRGBColor\":" + colorToJson(Options::PlayerColor::SkeletonRGBColor) + ",";
        json += "\"player.skeletonOutlineRGBColor\":" + colorToJson(Options::PlayerColor::SkeletonOutlineRGBColor) + ",";
        json += "\"player.snaplineRGBColor\":" + colorToJson(Options::PlayerColor::SnaplineRGBColor) + ",";
        json += "\"player.topTextRGBColor\":" + colorToJson(Options::PlayerColor::TopTextRGBColor) + ",";
        json += "\"player.bottomTextRGBColor\":" + colorToJson(Options::PlayerColor::BottomTextRGBColor) + ",";

        json += "\"player.skeletonVisible\":" + colorToJson(Options::PlayerColor::SkeletonVisible) + ",";
        json += "\"player.skeletonNotVisible\":" + colorToJson(Options::PlayerColor::SkeletonNotVisible) + ",";
        json += "\"player.skeletonOutlineVisible\":" + colorToJson(Options::PlayerColor::SkeletonOutlineVisible) + ",";
        json += "\"player.skeletonOutlineNotVisible\":" + colorToJson(Options::PlayerColor::SkeletonOutlineNotVisible) + ",";
        json += "\"player.snaplineVisible\":" + colorToJson(Options::PlayerColor::SnaplineVisible) + ",";
        json += "\"player.snaplineNotVisible\":" + colorToJson(Options::PlayerColor::SnaplineNotVisible) + ",";
        json += "\"player.skeleton\":" + colorToJson(Options::PlayerColor::Skeleton) + ",";
        json += "\"player.snapline\":" + colorToJson(Options::PlayerColor::Snapline) + ",";
        json += "\"player.topText\":" + colorToJson(Options::PlayerColor::TopText) + ",";
        json += "\"player.bottomText\":" + colorToJson(Options::PlayerColor::BottomText) + ",";

        json += "\"player.titleText\":" + colorToJson(Options::PlayerColor::TitleText) + ",";
        json += "\"player.topTextVisible\":" + colorToJson(Options::PlayerColor::TopTextVisible) + ",";
        json += "\"player.topTextNotVisible\":" + colorToJson(Options::PlayerColor::TopTextNotVisible) + ",";
        json += "\"player.bottomTextVisible\":" + colorToJson(Options::PlayerColor::BottomTextVisible) + ",";
        json += "\"player.bottomTextNotVisible\":" + colorToJson(Options::PlayerColor::BottomTextNotVisible) + ",";

        json += "\"aimbot.enabled\":" + boolToJson(Options::AssaultRifle::bAimbot) + ",";
        json += "\"aimbot.smooth\":" + floatToJson(Options::AssaultRifle::fAimbotSmooth) + ",";
        json += "\"aimbot.fov\":" + floatToJson(Options::AssaultRifle::fAimbotFOV) + ",";
        json += "\"aimbot.fillFOV\":" + boolToJson(Options::AssaultRifle::bFillFOV) + ",";
        json += "\"aimbot.fovCircleColor\":" + colorToJson(Options::AssaultRifle::FOVCircleColor) + ",";
        json += "\"aimbot.fovCircleRGBColor\":" + colorToJson(Options::AssaultRifle::FOVCircleRGBColor) + ",";
        json += "\"aimbot.fovCircleRGB\":" + boolToJson(Options::AssaultRifle::bFOVCircleRGB) + ",";
        json += "\"aimbot.fovCircleThickness\":" + floatToJson(Options::AssaultRifle::fFOVCircleThickness) + ",";
        json += "\"aimbot.fovFillInside\":" + boolToJson(Options::AssaultRifle::bFillFOVInside) + ",";
        json += "\"aimbot.visibleOnly\":" + boolToJson(Options::AssaultRifle::bVisibleOnly) + ",";
        json += "\"aimbot.knockedPlayers\":" + boolToJson(Options::AssaultRifle::bKnockedPlayers) + ",";
json += "\"aimbot.skipTeammates\":" + boolToJson(Options::AssaultRifle::bSkipTeammates) + ",";
        json += "\"aimbot.shootThroughWall\":" + boolToJson(Options::AssaultRifle::bShootThroughWall) + ",";
        json += "\"aimbot.raycastVisibility\":" + boolToJson(Options::AssaultRifle::bRaycastVisibility) + ",";
        json += "\"aimbot.renderRaycastGeometry\":" + boolToJson(Options::AssaultRifle::bRenderRaycastGeometry) + ",";

        json += "\"aimbot.targetClosestBone\":" + boolToJson(AimTarget::bClosestBone) + ",";
        json += "\"aimbot.currentHitbox\":" + std::to_string(Options::AssaultRifle::current_hitbox) + ",";

        json += "\"aimbot.keyShift\":" + boolToJson(Aimkey::bShift) + ",";
        json += "\"aimbot.keyMouse4\":" + boolToJson(Aimkey::bMouse4) + ",";
        json += "\"aimbot.keyMouse5\":" + boolToJson(Aimkey::bMouse5) + ",";
json += "\"aimbot.keyMouseRight\":" + boolToJson(Aimkey::bMouseRight) + ",";
        json += "\"aimbot.keyCapsLock\":" + boolToJson(Aimkey::bCapsLock) + ",";
        json += "\"aimbot.keyR\":" + boolToJson(Aimkey::bR) + ",";
        json += "\"aimbot.keyF\":" + boolToJson(Aimkey::bF) + ",";
        json += "\"aimbot.keyE\":" + boolToJson(Aimkey::bE) + ",";
        json += "\"aimbot.keyG\":" + boolToJson(Aimkey::bG) + ",";
        json += "\"aimbot.keyQ\":" + boolToJson(Aimkey::bQ) + ",";
        json += "\"aimbot.keyT\":" + boolToJson(Aimkey::bT) + ",";
        json += "\"aimbot.keyV\":" + boolToJson(Aimkey::bV) + ",";
        json += "\"aimbot.keyB\":" + boolToJson(Aimkey::bB) + ",";
        json += "\"aimbot.keyCtrl\":" + boolToJson(Aimkey::bCtrl) + ",";
        json += "\"aimbot.keyTab\":" + boolToJson(Aimkey::bTab) + ",";

        json += "\"aimbot.aimbotLine\":" + boolToJson(Options::AssaultRifle::bAimbotLine) + ",";
        json += "\"aimbot.aimbotLineThickness\":" + floatToJson(Options::AssaultRifle::fAimbotLineThickness) + ",";
        json += "\"aimbot.aimbotLineHoldWhileAiming\":" + boolToJson(Options::AssaultRifle::bAimbotLineHoldWhileAiming) + ",";
        json += "\"aimbot.aimbotLineColor\":" + colorToJson(PlayerColor::AimbotLineVisible) + ",";
        json += "\"aimbot.aimbotLineNotVisible\":" + colorToJson(PlayerColor::AimbotLineNotVisible) + ",";
        json += "\"aimbot.aimbotLineRGBColor\":" + colorToJson(PlayerColor::AimbotLineRGBColor) + ",";
        json += "\"aimbot.bAimbotLineColorRGB\":" + boolToJson(PlayerColor::bAimbotLineColorRGB) + ",";

        json += "\"rgb.enabled\":" + boolToJson(Options::RGBMode::bEnabled) + ",";
        json += "\"rgb.speed\":" + floatToJson(Options::RGBMode::fSpeed) + ",";

        json += "\"misc.streamProof\":" + boolToJson(Options::bStreamProof) + ",";
        json += "\"misc.crosshair\":" + boolToJson(Options::bCrosshair) + ",";
        json += "\"misc.disableGameCrosshair\":" + boolToJson(Options::bDisableGameCrosshair) + ",";
        json += "\"misc.crosshairLength\":" + floatToJson(Options::fCrosshairLength) + ",";
        json += "\"misc.crosshairThickness\":" + floatToJson(Options::fCrosshairThickness) + ",";
        json += "\"misc.menuKey\":" + std::to_string(Options::iMenuKey);

        json += "}";
        return json;
    }

    void DeserializeSettings(const std::string& jsonStr) {
        if (jsonStr.empty()) return;

        Options::bRadar = extractBool(jsonStr, "radar.enabled", Options::bRadar);
        Options::bRectangle = extractBool(jsonStr, "radar.rectangle", Options::bRectangle);
        Options::bUseTeamColor = extractBool(jsonStr, "radar.useTeamColor", Options::bUseTeamColor);
        Options::bUseVisibleColor = extractBool(jsonStr, "radar.useVisibleColor", Options::bUseVisibleColor);
        Options::bUseDistanceText = extractBool(jsonStr, "radar.useDistanceText", Options::bUseDistanceText);
        Options::bShowLocalPlayer = extractBool(jsonStr, "radar.showLocalPlayer", Options::bShowLocalPlayer);
        Options::bShowLineOfSight = extractBool(jsonStr, "radar.showLineOfSight", Options::bShowLineOfSight);
        Options::bRadarOutline = extractBool(jsonStr, "radar.outline", Options::bRadarOutline);
        Options::bRadarDisplayPlayers = extractBool(jsonStr, "radar.displayPlayers", Options::bRadarDisplayPlayers);
        Options::fPositionX = extractFloat(jsonStr, "radar.posX", Options::fPositionX);
        Options::fPositionY = extractFloat(jsonStr, "radar.posY", Options::fPositionY);
        Options::fSize = extractFloat(jsonStr, "radar.size", Options::fSize);
        Options::fTransparency = extractFloat(jsonStr, "radar.transparency", Options::fTransparency);
        Options::fRadarRange = extractFloat(jsonStr, "radar.range", Options::fRadarRange);

        Options::bCharacters = extractBool(jsonStr, "player.characters", Options::bCharacters);
        Options::bPlayerInfo = extractBool(jsonStr, "player.playerInfo", Options::bPlayerInfo);
        Options::bViewAngles = extractBool(jsonStr, "player.viewAngles", Options::bViewAngles);
        Options::bSkeleton = extractBool(jsonStr, "player.skeleton", Options::bSkeleton);
        Options::bSkeletonFeet = extractBool(jsonStr, "player.skeletonFeet", Options::bSkeletonFeet);
        Options::bSmoothSkeleton = extractBool(jsonStr, "player.smoothSkeleton", Options::bSmoothSkeleton);
        Options::SkeletonSmooth::fStep = extractFloat(jsonStr, "player.skeletonStep", Options::SkeletonSmooth::fStep);
        Options::bSnapline = extractBool(jsonStr, "player.snapline", Options::bSnapline);
        Options::bPlayerDistance = extractBool(jsonStr, "player.playerDistance", Options::bPlayerDistance);
        Options::bOnBots = extractBool(jsonStr, "player.onBots", Options::bOnBots);
        Options::bOnTeammate = extractBool(jsonStr, "player.onTeammate", Options::bOnTeammate);
        Options::bTeamBar = extractBool(jsonStr, "player.teamBar", Options::bTeamBar);
        Options::bplayername = extractBool(jsonStr, "player.playername", Options::bplayername);
        Options::bplattform = extractBool(jsonStr, "player.platform", Options::bplattform);
        Options::fSkeletonThickness = extractFloat(jsonStr, "player.skeletonThickness", Options::fSkeletonThickness);
        Options::bTeamIndex = extractBool(jsonStr, "player.teamIndex", Options::bTeamIndex);
        Options::bTeamScore = extractBool(jsonStr, "player.teamScore", Options::bTeamScore);
        Options::bSeasonRank = extractBool(jsonStr, "player.seasonRank", Options::bSeasonRank);
        Options::bWeaponName = extractBool(jsonStr, "player.weaponName", Options::bWeaponName);
        Options::fMaxPlayerDistance = extractFloat(jsonStr, "player.maxDistance", Options::fMaxPlayerDistance);

        Options::PlayerFonts::bStandardFont = extractBool(jsonStr, "player.fontStandard", Options::PlayerFonts::bStandardFont);
        Options::PlayerFonts::bGTAFont = extractBool(jsonStr, "player.fontGTA", Options::PlayerFonts::bGTAFont);
        Options::PlayerFonts::bFortniteFont = extractBool(jsonStr, "player.fontFortnite", Options::PlayerFonts::bFortniteFont);
        Options::PlayerFonts::bArial = extractBool(jsonStr, "player.fontArial", Options::PlayerFonts::bArial);
        Options::PlayerFonts::bRajdhani = extractBool(jsonStr, "player.fontRajdhani", Options::PlayerFonts::bRajdhani);
        Options::PlayerFonts::bVerdana = extractBool(jsonStr, "player.fontVerdana", Options::PlayerFonts::bVerdana);
        Options::PlayerFonts::bTahomaBold = extractBool(jsonStr, "player.fontTahomaBold", Options::PlayerFonts::bTahomaBold);
        Options::PlayerFeatures::OutlinedText = extractBool(jsonStr, "player.outlinedText", Options::PlayerFeatures::OutlinedText);
        Options::PlayerFeatures::TextBackground = extractBool(jsonStr, "player.textBackground", Options::PlayerFeatures::TextBackground);
        Options::PlayerFeatures::bSkeletonOutline = extractBool(jsonStr, "player.skeletonOutline", Options::PlayerFeatures::bSkeletonOutline);

        Options::PlayerFeatures::bSnaplineOutline = extractBool(jsonStr, "player.snaplineOutline", Options::PlayerFeatures::bSnaplineOutline);
        Options::PlayerFeatures::fSnaplineOutlineThickness = extractFloat(jsonStr, "player.snaplineOutlineThickness", Options::PlayerFeatures::fSnaplineOutlineThickness);

        Options::CharacterModes::box_type = extractInt(jsonStr, "player.boxType", Options::CharacterModes::box_type);
        Options::CharacterModes::fBoxThickness = extractFloat(jsonStr, "player.boxThickness", Options::CharacterModes::fBoxThickness);
        Options::bCircularRadar = extractBool(jsonStr, "player.circularRadar", Options::bCircularRadar);

        Options::SnaplineStart::bTop = extractBool(jsonStr, "player.snaplineTop", Options::SnaplineStart::bTop);
        Options::SnaplineStart::bBottom = extractBool(jsonStr, "player.snaplineBottom", Options::SnaplineStart::bBottom);

        extractColor(jsonStr, "player.boxVisible", Options::PlayerColor::BoxVisible);
        extractColor(jsonStr, "player.boxNotVisible", Options::PlayerColor::BoxNotVisible);
        extractColor(jsonStr, "player.boxFillColorVisible", Options::PlayerColor::BoxFillColorVisible);
        extractColor(jsonStr, "player.boxFillColorNotVisible", Options::PlayerColor::BoxFillColorNotVisible);
        Options::PlayerColor::bBoxFillColorRGB = extractBool(jsonStr, "player.bBoxFillColorRGB", Options::PlayerColor::bBoxFillColorRGB);
        Options::PlayerColor::bBoxColorRGB = extractBool(jsonStr, "player.bBoxColorRGB", Options::PlayerColor::bBoxColorRGB);
        Options::PlayerColor::bSkeletonColorRGB = extractBool(jsonStr, "player.bSkeletonColorRGB", Options::PlayerColor::bSkeletonColorRGB);
        Options::PlayerColor::bSkeletonOutlineColorRGB = extractBool(jsonStr, "player.bSkeletonOutlineColorRGB", Options::PlayerColor::bSkeletonOutlineColorRGB);
        Options::PlayerColor::bSnaplineColorRGB = extractBool(jsonStr, "player.bSnaplineColorRGB", Options::PlayerColor::bSnaplineColorRGB);
        Options::PlayerColor::bTopTextColorRGB = extractBool(jsonStr, "player.bTopTextColorRGB", Options::PlayerColor::bTopTextColorRGB);
        Options::PlayerColor::bBottomTextColorRGB = extractBool(jsonStr, "player.bBottomTextColorRGB", Options::PlayerColor::bBottomTextColorRGB);
        Options::PlayerColor::bTitleTextRGB = extractBool(jsonStr, "player.bTitleTextRGB", Options::PlayerColor::bTitleTextRGB);

        extractColor(jsonStr, "player.boxFillRGBColor", Options::PlayerColor::BoxFillRGBColor);
        extractColor(jsonStr, "player.skeletonRGBColor", Options::PlayerColor::SkeletonRGBColor);
        extractColor(jsonStr, "player.skeletonOutlineRGBColor", Options::PlayerColor::SkeletonOutlineRGBColor);
        extractColor(jsonStr, "player.snaplineRGBColor", Options::PlayerColor::SnaplineRGBColor);
        extractColor(jsonStr, "player.topTextRGBColor", Options::PlayerColor::TopTextRGBColor);
        extractColor(jsonStr, "player.bottomTextRGBColor", Options::PlayerColor::BottomTextRGBColor);

        extractColor(jsonStr, "player.skeletonVisible", Options::PlayerColor::SkeletonVisible);
        extractColor(jsonStr, "player.skeletonNotVisible", Options::PlayerColor::SkeletonNotVisible);
        extractColor(jsonStr, "player.skeletonOutlineVisible", Options::PlayerColor::SkeletonOutlineVisible);
        extractColor(jsonStr, "player.skeletonOutlineNotVisible", Options::PlayerColor::SkeletonOutlineNotVisible);
        extractColor(jsonStr, "player.snaplineVisible", Options::PlayerColor::SnaplineVisible);
        extractColor(jsonStr, "player.snaplineNotVisible", Options::PlayerColor::SnaplineNotVisible);
        extractColor(jsonStr, "player.skeleton", Options::PlayerColor::Skeleton);
        extractColor(jsonStr, "player.snapline", Options::PlayerColor::Snapline);
        extractColor(jsonStr, "player.topText", Options::PlayerColor::TopText);
        extractColor(jsonStr, "player.bottomText", Options::PlayerColor::BottomText);

        extractColor(jsonStr, "player.titleText", Options::PlayerColor::TitleText);
        extractColor(jsonStr, "player.topTextVisible", Options::PlayerColor::TopTextVisible);
        extractColor(jsonStr, "player.topTextNotVisible", Options::PlayerColor::TopTextNotVisible);
        extractColor(jsonStr, "player.bottomTextVisible", Options::PlayerColor::BottomTextVisible);
        extractColor(jsonStr, "player.bottomTextNotVisible", Options::PlayerColor::BottomTextNotVisible);

        Options::AssaultRifle::bAimbot = extractBool(jsonStr, "aimbot.enabled", Options::AssaultRifle::bAimbot);
        Options::AssaultRifle::fAimbotSmooth = extractFloat(jsonStr, "aimbot.smooth", Options::AssaultRifle::fAimbotSmooth);
        Options::AssaultRifle::fAimbotFOV = (std::max)(extractFloat(jsonStr, "aimbot.fov", Options::AssaultRifle::fAimbotFOV), 5.0f);
        Options::AssaultRifle::bFillFOV = extractBool(jsonStr, "aimbot.fillFOV", Options::AssaultRifle::bFillFOV);
        extractColor(jsonStr, "aimbot.fovCircleColor", Options::AssaultRifle::FOVCircleColor);
        extractColor(jsonStr, "aimbot.fovCircleRGBColor", Options::AssaultRifle::FOVCircleRGBColor);
        Options::AssaultRifle::bFOVCircleRGB = extractBool(jsonStr, "aimbot.fovCircleRGB", Options::AssaultRifle::bFOVCircleRGB);
        Options::AssaultRifle::fFOVCircleThickness = extractFloat(jsonStr, "aimbot.fovCircleThickness", Options::AssaultRifle::fFOVCircleThickness);
        Options::AssaultRifle::bFillFOVInside = extractBool(jsonStr, "aimbot.fovFillInside", Options::AssaultRifle::bFillFOVInside);
        Options::AssaultRifle::bVisibleOnly = extractBool(jsonStr, "aimbot.visibleOnly", Options::AssaultRifle::bVisibleOnly);
        Options::AssaultRifle::bKnockedPlayers = extractBool(jsonStr, "aimbot.knockedPlayers", Options::AssaultRifle::bKnockedPlayers);
Options::AssaultRifle::bSkipTeammates = extractBool(jsonStr, "aimbot.skipTeammates", Options::AssaultRifle::bSkipTeammates);
        Options::AssaultRifle::bShootThroughWall = extractBool(jsonStr, "aimbot.shootThroughWall", Options::AssaultRifle::bShootThroughWall);
        Options::AssaultRifle::bRaycastVisibility = extractBool(jsonStr, "aimbot.raycastVisibility", Options::AssaultRifle::bRaycastVisibility);
        Options::AssaultRifle::bRenderRaycastGeometry = extractBool(jsonStr, "aimbot.renderRaycastGeometry", Options::AssaultRifle::bRenderRaycastGeometry);

        AimTarget::bClosestBone = extractBool(jsonStr, "aimbot.targetClosestBone", AimTarget::bClosestBone);
        int hitboxInt = (int)Options::AssaultRifle::current_hitbox;
        hitboxInt = (int)extractFloat(jsonStr, "aimbot.currentHitbox", (float)hitboxInt);
        Options::AssaultRifle::current_hitbox = (BoneIDS)hitboxInt;

        Aimkey::bShift = extractBool(jsonStr, "aimbot.keyShift", Aimkey::bShift);
        Aimkey::bMouse4 = extractBool(jsonStr, "aimbot.keyMouse4", Aimkey::bMouse4);
        Aimkey::bMouse5 = extractBool(jsonStr, "aimbot.keyMouse5", Aimkey::bMouse5);
Aimkey::bMouseRight = extractBool(jsonStr, "aimbot.keyMouseRight", Aimkey::bMouseRight);
        Aimkey::bCapsLock = extractBool(jsonStr, "aimbot.keyCapsLock", Aimkey::bCapsLock);
        Aimkey::bR = extractBool(jsonStr, "aimbot.keyR", Aimkey::bR);
        Aimkey::bF = extractBool(jsonStr, "aimbot.keyF", Aimkey::bF);
        Aimkey::bE = extractBool(jsonStr, "aimbot.keyE", Aimkey::bE);
        Aimkey::bG = extractBool(jsonStr, "aimbot.keyG", Aimkey::bG);
        Aimkey::bQ = extractBool(jsonStr, "aimbot.keyQ", Aimkey::bQ);
        Aimkey::bT = extractBool(jsonStr, "aimbot.keyT", Aimkey::bT);
        Aimkey::bV = extractBool(jsonStr, "aimbot.keyV", Aimkey::bV);
        Aimkey::bB = extractBool(jsonStr, "aimbot.keyB", Aimkey::bB);
        Aimkey::bCtrl = extractBool(jsonStr, "aimbot.keyCtrl", Aimkey::bCtrl);
        Aimkey::bTab = extractBool(jsonStr, "aimbot.keyTab", Aimkey::bTab);

        Options::AssaultRifle::bAimbotLine = extractBool(jsonStr, "aimbot.aimbotLine", Options::AssaultRifle::bAimbotLine);
        extractColor(jsonStr, "aimbot.aimbotLineColor", PlayerColor::AimbotLineVisible);
        Options::AssaultRifle::fAimbotLineThickness = extractFloat(jsonStr, "aimbot.aimbotLineThickness", Options::AssaultRifle::fAimbotLineThickness);
        PlayerColor::bAimbotLineColorRGB = extractBool(jsonStr, "aimbot.bAimbotLineColorRGB", PlayerColor::bAimbotLineColorRGB);
        Options::AssaultRifle::bAimbotLineHoldWhileAiming = extractBool(jsonStr, "aimbot.aimbotLineHoldWhileAiming", Options::AssaultRifle::bAimbotLineHoldWhileAiming);
        extractColor(jsonStr, "aimbot.aimbotLineNotVisible", PlayerColor::AimbotLineNotVisible);
        extractColor(jsonStr, "aimbot.aimbotLineRGBColor", PlayerColor::AimbotLineRGBColor);

        Options::RGBMode::bEnabled = extractBool(jsonStr, "rgb.enabled", Options::RGBMode::bEnabled);
        Options::RGBMode::fSpeed = extractFloat(jsonStr, "rgb.speed", Options::RGBMode::fSpeed);

        Options::bStreamProof = extractBool(jsonStr, "misc.streamProof", Options::bStreamProof);
        Options::bCrosshair = extractBool(jsonStr, "misc.crosshair", Options::bCrosshair);
        Options::bDisableGameCrosshair = extractBool(jsonStr, "misc.disableGameCrosshair", Options::bDisableGameCrosshair);
        Options::fCrosshairLength = extractFloat(jsonStr, "misc.crosshairLength", Options::fCrosshairLength);
        Options::fCrosshairThickness = extractFloat(jsonStr, "misc.crosshairThickness", Options::fCrosshairThickness);
        Options::iMenuKey = extractInt(jsonStr, "misc.menuKey", Options::iMenuKey);
    }

    bool Save(const std::string& name) {
        EnsureDirectory();
        currentConfig = name;
        std::string filename = configPath + name + ".json";

        std::ofstream file(filename);
        if (file.is_open()) {
            std::string json = SerializeSettings();
            file << json;
            file.close();
            return true;
        }
        return false;
    }

    bool Load(const std::string& name) {
        std::string filename = configPath + name + ".json";
        std::ifstream file(filename);

        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string jsonStr = buffer.str();
            file.close();

            currentConfig = name;
            DeserializeSettings(jsonStr);
            return true;
        }
        return false;
    }

    void Delete(const std::string& name) {
        std::string filename = configPath + name + ".json";
        if (std::filesystem::exists(filename)) {
            std::filesystem::remove(filename);
        }
    }

    void Refresh() {
        if (Exists(currentConfig)) {
            Load(currentConfig);
        }
    }
}
