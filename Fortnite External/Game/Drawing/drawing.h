#pragma once
#include <utility>
#include <vector>
#include "../../settings/settings.h"
#include "../../sdk-fname-offsets/Sdk/sdk.h"
#include <overlay.h>

namespace draw {
    void box(int x, int y, int w, int h, ImU32 color, float thickness, bool outline, ImDrawList* dl = nullptr) {
        ImDrawList* d = dl ? dl : ImGui::GetBackgroundDrawList();
        if (outline) {
            ImU32 outlineCol = IM_COL32(0, 0, 0, 255);
            d->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), outlineCol, 0.0f, 0, 1.0f);
        }
        d->AddLine(ImVec2(x, y), ImVec2(x + w, y), color, thickness);
        d->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + h), color, thickness);
        d->AddLine(ImVec2(x + w, y + h), ImVec2(x, y + h), color, thickness);
        d->AddLine(ImVec2(x, y + h), ImVec2(x, y), color, thickness);
    }

    void corner_box(int x, int y, int w, int h, ImU32 color, float thickness, bool outline, ImDrawList* dl = nullptr) {
        ImDrawList* d = dl ? dl : ImGui::GetBackgroundDrawList();
        if (outline) {
            ImU32 outlineCol = IM_COL32(0, 0, 0, 255);
            d->AddLine(ImVec2(x - 1, y - 1), ImVec2(x - 1, y + h/3 + 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x - 1, y - 1), ImVec2(x + w/3 + 1, y - 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x + w - w/3 - 1, y - 1), ImVec2(x + w + 1, y - 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x + w + 1, y - 1), ImVec2(x + w + 1, y + h/3 + 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x - 1, y + h - h/3 - 1), ImVec2(x - 1, y + h + 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x - 1, y + h + 1), ImVec2(x + w/3 + 1, y + h + 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x + w - w/3 - 1, y + h + 1), ImVec2(x + w + 1, y + h + 1), outlineCol, 1.0f);
            d->AddLine(ImVec2(x + w + 1, y + h - h/3 - 1), ImVec2(x + w + 1, y + h + 1), outlineCol, 1.0f);
        }
        d->AddLine(ImVec2(x, y), ImVec2(x, y + h/3), color, thickness);
        d->AddLine(ImVec2(x, y), ImVec2(x + w/3, y), color, thickness);
        d->AddLine(ImVec2(x+w-w/3, y), ImVec2(x+w, y), color, thickness);
        d->AddLine(ImVec2(x+w, y), ImVec2(x+w, y+h/3), color, thickness);
        d->AddLine(ImVec2(x, y+h-h/3), ImVec2(x, y+h), color, thickness);
        d->AddLine(ImVec2(x, y+h), ImVec2(x+w/3, y+h), color, thickness);
        d->AddLine(ImVec2(x+w-w/3, y+h), ImVec2(x+w, y+h), color, thickness);
        d->AddLine(ImVec2(x+w, y+h-h/3), ImVec2(x+w, y+h), color, thickness);
    }
}

void RenderBox(int X, int Y, int W, int H, ImVec4 Color, float thickness, bool outline, int box_type, bool isVisible = false, ImDrawList* dl = nullptr) {
    ImU32 col = ImGui::GetColorU32({Color.x, Color.y, Color.z, Color.w});
    if (box_type == 2 || box_type == 3) {
        float* fillColorSrc;
        if (Options::RGBMode::bEnabled && Options::PlayerColor::bBoxFillColorRGB) {
            fillColorSrc = Options::PlayerColor::BoxFillRGBColor;
        } else if (isVisible) {
            fillColorSrc = Options::PlayerColor::BoxFillColorVisible;
        } else {
            fillColorSrc = Options::PlayerColor::BoxFillColorNotVisible;
        }
        ImU32 fillCol32 = IM_COL32(fillColorSrc[0]*255, fillColorSrc[1]*255, fillColorSrc[2]*255, fillColorSrc[3]*255);
        ImDrawList* fill_dl = dl ? dl : ImGui::GetBackgroundDrawList();
        fill_dl->AddRectFilled(ImVec2(X,Y), ImVec2(X+W,Y+H), fillCol32);
    }
    switch (box_type) {
        case 0: case 2: draw::box(X,Y,W,H,col,thickness,outline,dl); break;
        case 1: case 3: draw::corner_box(X,Y,W,H,col,thickness,outline,dl); break;
    }
}

namespace Render {
	ImFont* GetSelectedPlayerFont() {
		if (Options::PlayerFonts::bArial)
			return Fonts::Arial;
		else if (Options::PlayerFonts::bRajdhani)
			return Fonts::Rajdhani;
		else if (Options::PlayerFonts::bVerdana)
			return Fonts::Verdana;
		else if (Options::PlayerFonts::bFortniteFont)
			return Fonts::FortniteFont;
		else if (Options::PlayerFonts::bGTAFont)
			return Fonts::GTAFont;
		else if (Options::PlayerFonts::bTahomaBold)
			return Fonts::TahomaBold;
		else
			return Fonts::StandardFont;
	}

	ImFont* GetSelectedItemFont() {
		if (Options::PlayerFonts::bArial)
			return Fonts::Arial;
		else if (Options::PlayerFonts::bRajdhani)
			return Fonts::Rajdhani;
		else if (Options::PlayerFonts::bVerdana)
			return Fonts::Verdana;
		else if (Options::PlayerFonts::bFortniteFont)
			return Fonts::FortniteFont;
		else if (Options::PlayerFonts::bGTAFont)
			return Fonts::GTAFont;
		else if (Options::PlayerFonts::bTahomaBold)
			return Fonts::TahomaBold;
		else
			return Fonts::StandardFont;
	}

	ImFont* GetSelectedWorldFont() {
		if (Options::PlayerFonts::bArial)
			return Fonts::Arial;
		else if (Options::PlayerFonts::bRajdhani)
			return Fonts::Rajdhani;
		else if (Options::PlayerFonts::bVerdana)
			return Fonts::Verdana;
		else if (Options::PlayerFonts::bFortniteFont)
			return Fonts::FortniteFont;
		else if (Options::PlayerFonts::bGTAFont)
			return Fonts::GTAFont;
		else if (Options::PlayerFonts::bTahomaBold)
			return Fonts::TahomaBold;
		else
			return Fonts::StandardFont;
	}

	mvec CalcTextSizeWithFont(const char* Text, ImFont* Font = nullptr) {
		ImFont* fontToUse = Font ? Font : GetSelectedPlayerFont();
		bool pushedFont = false;
		if (fontToUse) {
			ImGui::PushFont(fontToUse);
			pushedFont = true;
		}
		mvec textSize = ImGui::CalcTextSize(Text);
		if (pushedFont) {
			ImGui::PopFont();
		}
		return textSize;
	}

	mvec CalcTextSizeWithWorldFont(const char* Text, ImFont* Font = nullptr) {
		ImFont* fontToUse = Font ? Font : GetSelectedWorldFont();
		bool pushedFont = false;
		if (fontToUse) {
			ImGui::PushFont(fontToUse);
			pushedFont = true;
		}
		mvec textSize = ImGui::CalcTextSize(Text);
		if (pushedFont) {
			ImGui::PopFont();
		}
		return textSize;
	}

	VOID MakeTextWithBackground(const char* Text, mvec ScreenLocation, ImU32 Color, ImU32 BackgroundColor, bool Background, bool Outlined, ImFont* Font = nullptr) {
		ImDrawList* Draw = Background ? ImGui::GetBackgroundDrawList() : ImGui::GetBackgroundDrawList();
		ImFont* fontToUse = Font ? Font : GetSelectedPlayerFont();
		bool pushedFont = false;
		if (fontToUse) {
			ImGui::PushFont(fontToUse);
			pushedFont = true;
		}
		mvec TextSize = ImGui::CalcTextSize(Text);
		mvec BackgroundStart = mvec(ScreenLocation.x - 2, ScreenLocation.y - 1);
		mvec BackgroundEnd = mvec(ScreenLocation.x + TextSize.x + 2, ScreenLocation.y + TextSize.y + 1);
		Draw->AddRectFilled(BackgroundStart, BackgroundEnd, BackgroundColor, 2.0f);
		float fontSize = fontToUse ? fontToUse->FontSize : ImGui::GetFontSize();
		if (Outlined) {
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x + 1, ScreenLocation.y + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x - 1, ScreenLocation.y - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x + 1, ScreenLocation.y - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x - 1, ScreenLocation.y + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
		}

		Draw->AddText(fontToUse, fontSize, ScreenLocation, Color, Text);
		if (pushedFont) {
			ImGui::PopFont();
		}
	}

	VOID MakeText(const char* Text, mvec ScreenLocation, ImU32 Color, bool Background, bool Outlined, ImFont* Font = nullptr) {
		if (Options::PlayerFeatures::TextBackground) {
			ImU32 BackgroundColor = ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 0.7f });
			ImFont* fontToUse = Font ? Font : Render::GetSelectedPlayerFont();
			MakeTextWithBackground(Text, ScreenLocation, Color, BackgroundColor, Background, Outlined, fontToUse);
			return;
		}

		ImDrawList* Draw = ImGui::GetForegroundDrawList();
		if (Background)
		Draw = ImGui::GetBackgroundDrawList();

		ImFont* fontToUse = Font ? Font : Render::GetSelectedPlayerFont();
		bool pushedFont = false;
		if (fontToUse) {
			ImGui::PushFont(fontToUse);
			pushedFont = true;
		}

		float fontSize = fontToUse ? fontToUse->FontSize : ImGui::GetFontSize();
		if (Outlined) {
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x + 1, ScreenLocation.y + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x - 1, ScreenLocation.y - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x + 1, ScreenLocation.y - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x - 1, ScreenLocation.y + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
		}

		Draw->AddText(fontToUse, fontSize, ScreenLocation, Color, Text);
		if (pushedFont) {
			ImGui::PopFont();
		}
	}

	VOID MakeTextworldesp(const char* Text, mvec ScreenLocation, ImU32 Color, bool Background, bool Outlined, ImFont* Font = nullptr) {
		if (Options::PlayerFeatures::TextBackground) {
			ImU32 BackgroundColor = ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 0.7f });
			ImFont* fontToUse = Font ? Font : Render::GetSelectedWorldFont();
			MakeTextWithBackground(Text, ScreenLocation, Color, BackgroundColor, Background, Outlined, fontToUse);
			return;
		}

		ImDrawList* Draw = ImGui::GetBackgroundDrawList();
		if (Background)
		Draw = ImGui::GetBackgroundDrawList();

		ImFont* fontToUse = Font ? Font : Render::GetSelectedWorldFont();
		bool pushedFont = false;
		if (fontToUse) {
			ImGui::PushFont(fontToUse);
			pushedFont = true;
		}

		float fontSize = fontToUse ? fontToUse->FontSize : ImGui::GetFontSize();
		if (Outlined) {
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x + 1, ScreenLocation.y + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x - 1, ScreenLocation.y - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x + 1, ScreenLocation.y - 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
			Draw->AddText(fontToUse, fontSize, mvec(ScreenLocation.x - 1, ScreenLocation.y + 1), ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.0f }), Text);
		}

		Draw->AddText(fontToUse, fontSize, ScreenLocation, Color, Text);
		if (pushedFont) {
			ImGui::PopFont();
		}
	}

	VOID MakeLine(mvec ScreenStart, mvec ScreenEnd, ImU32 Color, float Thickness, bool Background) {
		ImDrawList* Draw = ImGui::GetForegroundDrawList();
		if (Background)
			Draw = ImGui::GetBackgroundDrawList();

		Draw->AddLine(ScreenStart, ScreenEnd, Color, Thickness);
	}

	VOID MakeRect(mvec ScreenStart, mvec ScreenEnd, ImU32 Color, ImVec4 FilledColor, float Rounding, float Thickness, bool Background, bool Filled) {
		ImDrawList* Draw = ImGui::GetForegroundDrawList();
		if (Background)
			Draw = ImGui::GetBackgroundDrawList();

		if (Filled) {
			auto Converted = IM_COL32(FilledColor.x * 255, FilledColor.y * 255, FilledColor.z * 255, 40);
			Draw->AddRectFilled(ScreenStart, ScreenEnd, Converted, Rounding, NULL);
		}

		Draw->AddRect(ScreenStart, ScreenEnd, Color, Rounding, NULL, Thickness);
	}
}

bool ret = false;
ImTextureID TestNigger;
ID3D11ShaderResourceView* my_texture = nullptr;

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_texture, int* out_width, int* out_height)
{

	return false;
}

void Drawcourser(float x, float y, float w, float h, float r, float g, float b, float a, bool filled) {
	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	ImU32 color = ImGui::GetColorU32(ImVec4(r, g, b, a));
	if (filled) {
		draw_list->AddRectFilled(mvec(x, y), mvec(x + w, y + h), color);
	}
	else {
		draw_list->AddRect(mvec(x, y), mvec(x + w, y + h), color);
	}
}

void draw_text(ImFont* impact, float fontSize, mvec position, ImColor color, const char* text, bool outlined = true)
{
	ImFont* selectedFont = Render::GetSelectedWorldFont();
	ImGui::PushFont(selectedFont);
	if (outlined) {
		ImGui::GetBackgroundDrawList()->AddText(selectedFont, fontSize, mvec(position.x - 1, position.y - 1), ImColor(0, 0, 0), text);
		ImGui::GetBackgroundDrawList()->AddText(selectedFont, fontSize, mvec(position.x + 1, position.y - 1), ImColor(0, 0, 0), text);
		ImGui::GetBackgroundDrawList()->AddText(selectedFont, fontSize, mvec(position.x - 1, position.y + 1), ImColor(0, 0, 0), text);
		ImGui::GetBackgroundDrawList()->AddText(selectedFont, fontSize, mvec(position.x + 1, position.y + 1), ImColor(0, 0, 0), text);
	}
	ImGui::GetBackgroundDrawList()->AddText(selectedFont, fontSize, position, color, text);
	ImGui::PopFont();
}
