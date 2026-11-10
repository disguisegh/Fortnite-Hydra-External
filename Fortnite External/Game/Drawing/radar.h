#pragma once
#include "../math/math.h"
#include "../sdk-fname-offsets/sdk/sdk.h"
#include "../settings/settings.h"

namespace Radar {
    void RadarRange(float* x, float* y, float range)
    {
        if (fabs((*x)) > range || fabs((*y)) > range)
        {
            if ((*y) > (*x))
            {
                if ((*y) > -(*x))
                {
                    (*x) = range * (*x) / (*y);
                    (*y) = range;
                }
                else
                {
                    (*y) = -range * (*y) / (*x);
                    (*x) = -range;
                }
            }
            else
            {
                if ((*y) > -(*x))
                {
                    (*y) = range * (*y) / (*x);
                    (*x) = range;
                }
                else
                {
                    (*x) = -range * (*x) / (*y);
                    (*y) = -range;
                }
            }
        }
    }

    void CalcRadarPoint(Vector3 vOrigin, int& screenx, int& screeny)
    {
        Vector3 vAngle = Vector3{ camera.Rotation.x, camera.Rotation.y, camera.Rotation.z };
        auto fYaw = vAngle.y * std::numbers::pi / 180.0f;
        float dx = vOrigin.x - camera.Location.x;
        float dy = vOrigin.y - camera.Location.y;

        float fsin_yaw = sinf(fYaw);
        float fminus_cos_yaw = -cosf(fYaw);

        float x = dy * fminus_cos_yaw + dx * fsin_yaw;
        x = -x;
        float y = dx * fminus_cos_yaw - dy * fsin_yaw;

        float range = (float)Options::fRadarRange * 1000.f;

        RadarRange(&x, &y, range);

        mvec DrawPos = mvec(Options::fPositionX, Options::fPositionY);
        mvec DrawSize = mvec(Options::fSize, Options::fSize);

        int rad_x = (int)DrawPos.x;
        int rad_y = (int)DrawPos.y;

        float r_siz_x = DrawSize.x;
        float r_siz_y = DrawSize.y;

        int x_max = (int)r_siz_x + rad_x - 5;
        int y_max = (int)r_siz_y + rad_y - 5;

        screenx = rad_x + ((int)r_siz_x / 2 + int(x / range * r_siz_x));
        screeny = rad_y + ((int)r_siz_y / 2 + int(y / range * r_siz_y));

        if (screenx > x_max)
            screenx = x_max;

        if (screenx < rad_x)
            screenx = rad_x;

        if (screeny > y_max)
            screeny = y_max;

        if (screeny < rad_y)
            screeny = rad_y;

        if (Options::bCircularRadar) {
            float centerX = rad_x + r_siz_x / 2.0f;
            float centerY = rad_y + r_siz_y / 2.0f;
            float radius = r_siz_x / 2.0f - 5.0f;
            float dx = screenx - centerX;
            float dy = screeny - centerY;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > radius && dist > 0) {
                screenx = centerX + (dx / dist) * radius;
                screeny = centerY + (dy / dist) * radius;
            }
        }
    }

    VOID DrawRadarBackground() {
        mvec RadarCenter = mvec(Options::fPositionX + (Options::fSize / 2), Options::fPositionY + (Options::fSize / 2));
        mvec TopLeftRadar = mvec(Options::fPositionX, Options::fPositionY);
        mvec TopRightRadar = mvec(Options::fPositionX + Options::fSize, Options::fPositionY);
        mvec BottomRadar = mvec(RadarCenter.x, Options::fPositionY + Options::fSize);

        if (Options::bCircularRadar) {
            ImGui::GetBackgroundDrawList()->AddCircleFilled(RadarCenter, Options::fSize / 2, IM_COL32(35, 35, 35, Options::fTransparency), 32);
        } else {
            ImGui::GetBackgroundDrawList()->AddRectFilled(mvec(Options::fPositionX, Options::fPositionY), mvec(Options::fPositionX + Options::fSize, Options::fPositionY + Options::fSize), IM_COL32(35, 35, 35, Options::fTransparency), 0.f, 0);
        }

        if (Options::bShowLineOfSight) {
            if (Options::bCircularRadar) {
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, mvec(RadarCenter.x, Options::fPositionY), ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, mvec(Options::fPositionX + Options::fSize, RadarCenter.y), ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, mvec(RadarCenter.x, Options::fPositionY + Options::fSize), ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, mvec(Options::fPositionX, RadarCenter.y), ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
            } else {
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, TopLeftRadar, ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, TopRightRadar, ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
                ImGui::GetBackgroundDrawList()->AddLine(RadarCenter, BottomRadar, ImGui::GetColorU32({ 0.8f, 0.8f, 0.8f, 1.0f }), 1.f);
            }
        }

        if (Options::bRadarOutline) {
            if (Options::bCircularRadar) {
                ImGui::GetBackgroundDrawList()->AddCircle(RadarCenter, Options::fSize / 2, IM_COL32(0, 0, 0, 255), 32, 1.f);
            } else {
                ImGui::GetBackgroundDrawList()->AddRect(mvec(Options::fPositionX, Options::fPositionY), mvec(Options::fPositionX + Options::fSize, Options::fPositionY + Options::fSize), IM_COL32(0, 0, 0, 255), 0.f, 0, 1.f);
            }
        }

        if (Options::bShowLocalPlayer) {
            if (Options::bRectangle) {
                ImGui::GetBackgroundDrawList()->AddRectFilled(mvec(RadarCenter.x - 2.5, RadarCenter.y - 2.5), mvec(RadarCenter.x + 2.5, RadarCenter.y + 2.5), ImGui::GetColorU32({ 0.0f, 0.4f, 1.0f, 1.0f }), 0.f, 0);
            }
        }
    }

    VOID AddToRadar(ImU32 Color, Vector3 WorldLocation, float fDistance) {
        int ScreenX, ScreenY = 0;
        CalcRadarPoint(WorldLocation, ScreenX, ScreenY);

        if (Options::bRectangle) {
            ImGui::GetBackgroundDrawList()->AddRectFilled(mvec((float)ScreenX - 2.5, (float)ScreenY - 2.5), mvec((float)ScreenX + 2.5, (float)ScreenY + 2.5), Color);
        }

        if (Options::bUseDistanceText) {
            char Dist[256];
            sprintf_s(Dist, "%.fm", fDistance);
            mvec SizeOfText = ImGui::CalcTextSize(Dist);
            if (Options::bRadarOutline) {
                int iX = (float)ScreenX - (SizeOfText.x / 2);
                int iY = ScreenY;
                ImGui::GetBackgroundDrawList()->AddText(mvec(iX + 1, iY + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Dist);
                ImGui::GetBackgroundDrawList()->AddText(mvec(iX - 1, iY - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Dist);
                ImGui::GetBackgroundDrawList()->AddText(mvec(iX + 1, iY - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Dist);
                ImGui::GetBackgroundDrawList()->AddText(mvec(iX - 1, iY + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Dist);
            }
            ImGui::GetBackgroundDrawList()->AddText(mvec((float)ScreenX - (SizeOfText.x / 2), ScreenY), Color, Dist);
        }
    }
}
