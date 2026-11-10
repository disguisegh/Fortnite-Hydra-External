#pragma once
#include "../Drawing/drawing.h"
#include "../Drawing/radar.h"
#include "../Aimbot/Aimbot.h"
#include "../Raycast/raycast.h"

#include <algorithm>
#include <cmath>
#include <string>

inline double getCrossDistance(double x1, double y1, double x2, double y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

inline const char* GetRankName(int rank) {
	switch (rank) {
		case 0:  return "Bronze I";
		case 1:  return "Bronze II";
		case 2:  return "Bronze III";
		case 3:  return "Silver I";
		case 4:  return "Silver II";
		case 5:  return "Silver III";
		case 6:  return "Gold I";
		case 7:  return "Gold II";
		case 8:  return "Gold III";
		case 9:  return "Platinum I";
		case 10: return "Platinum II";
		case 11: return "Platinum III";
		case 12: return "Diamond I";
		case 13: return "Diamond II";
		case 14: return "Diamond III";
		case 15: return "Elite I";
		case 16: return "Elite II";
		case 17: return "Elite III";
		case 18: return "Champion I";
		case 19: return "Champion II";
		case 20: return "Champion III";
		case 21: return "Unreal";
		default: return "Unranked";
	}
}

inline ImVec4 GetRankColor(int rank) {
	switch (rank) {
		case 0: case 1: case 2:  return ImVec4(0.62f, 0.32f, 0.18f, 1.0f);
		case 3: case 4: case 5:  return ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
		case 6: case 7: case 8:  return ImVec4(1.00f, 0.84f, 0.00f, 1.0f);
		case 9: case 10: case 11: return ImVec4(0.29f, 0.51f, 0.95f, 1.0f);
		case 12: case 13: case 14: return ImVec4(0.15f, 0.33f, 0.82f, 1.0f);
		case 15: case 16: case 17: return ImVec4(0.82f, 0.41f, 0.82f, 1.0f);
		case 18: case 19: case 20: return ImVec4(1.00f, 0.55f, 0.00f, 1.0f);
		case 21: return ImVec4(0.50f, 0.00f, 1.00f, 1.0f);
		default: return ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
	}
}

struct SniperOverride {
	float speed;
	float gravity;
};

inline SniperOverride GetSniperOverride(const std::string& name)
{
	if (name.find("Reaper Sniper Rifle") != std::string::npos)
		return { 50000.f, 3.5f };

	if (name.find("Heavy Sniper Rifle") != std::string::npos ||
		name.find("hunter bolt-action sniper") != std::string::npos)
		return { 45000.f, 0.12f };

	if (name.find("DMR") != std::string::npos)
		return { 53000.f, 0.15f };

	if (name.find("Automatic Sniper Rifle") != std::string::npos)
		return { 50000.f, 0.12f };

	const char* sniperNames[] = {
		"Dragon's Breath Sniper", "Storm Scout", "Storm Scout Sniper Rifle",
		"Hunting Rifle", "Explosive Repeater Rifle", "Bolt-Action Sniper Rifle",
		"Suppressed Sniper Rifle", "Lever Action Rifle", "Boom Sniper Rifle",
		"Sniper",
	};
	for (auto* pattern : sniperNames) {
		if (name.find(pattern) != std::string::npos)
			return { 30000.f, 0.12f };
	}

	return { 0.f, 0.f };
}

void ActorRead() {
	IncrementFrameCounter();

	GameDataBatch gameData = ReadGameDataBatch(base_address);
	if (!gameData.Gworld) return;

	LocalPtrs::Gworld = gameData.Gworld;
	LocalPtrs::GameInstance = gameData.GameInstance;
	LocalPtrs::LocalPlayers = gameData.LocalPlayers;
	LocalPtrs::PlayerController = gameData.PlayerController;
	LocalPtrs::GameState = gameData.GameState;
	LocalPtrs::myplayer = gameData.myplayer;
	LocalPtrs::RootComponent = gameData.RootComponent;

	static uintptr_t lastWeaponAddr = 0;
	static bool bIsSniper = false;
	static float s_sniperSpeed = 0.f;
	static float s_sniperGravity = 0.f;

	if (gameData.myplayer) {
		uintptr_t weapon = read<uintptr_t>(gameData.myplayer + Offsets::CurrentWeapon);
		if (weapon) {
			g_localProjectileSpeed = read<float>(weapon + Offsets::ProjectileSpeed);
			g_localProjectileGravity = read<float>(weapon + Offsets::ProjectileGravityScale);

			if (bIsSniper) {
				g_localProjectileSpeed = s_sniperSpeed;
				g_localProjectileGravity = s_sniperGravity;
			}

			if (weapon != lastWeaponAddr) {
				lastWeaponAddr = weapon;
				g_bIsLocalPickaxe = false;
				uintptr_t weaponData = read<uintptr_t>(weapon + Offsets::WeaponData);
				if (weaponData) {
					uintptr_t textDataPtr = read<uintptr_t>(weaponData + Offsets::ItemName);
					if (textDataPtr) {
						uintptr_t fstringData = read<uintptr_t>(textDataPtr + 0x20);
						int length = read<int>(textDataPtr + 0x28);
						if (length > 0 && length < 200) {
							std::vector<wchar_t> buf(static_cast<size_t>(length));
							g_driver->Read(fstringData, buf.data(), static_cast<DWORD>(length * sizeof(wchar_t)));
							std::wstring wstr(buf.begin(), buf.end());
							std::string name(wstr.begin(), wstr.end());
							if (name.find("Pickaxe") != std::string::npos ||
								name.find("pickaxe") != std::string::npos)
								g_bIsLocalPickaxe = true;

							SniperOverride ov = GetSniperOverride(name);
							bIsSniper = (ov.speed > 0.f);
							if (bIsSniper) {
								s_sniperSpeed = ov.speed;
								s_sniperGravity = ov.gravity;
							}
						}
					}
				}
			}

			if (Options::AssaultRifle::bShootThroughWall) {
				Vector3 magicOffset = { 230.0f, 0.0f, 230.0f };
				write<Vector3>(weapon + Offsets::TargetSourceOffset, magicOffset);
				write<Vector3>(weapon + Offsets::TargetSourceOffsetWhileCrouched, magicOffset);
				write<Vector3>(weapon + Offsets::TargetSourceOffsetWhileTargeting, magicOffset);
			}
		} else {
			lastWeaponAddr = 0;
			g_localProjectileSpeed = 0.f;
			g_localProjectileGravity = 0.f;
			g_bIsLocalPickaxe = false;
		}
	}

	static auto s_lastRebuild = std::chrono::steady_clock::now();
	if (g_raycast && LocalPtrs::Gworld &&
		(Options::AssaultRifle::bRaycastVisibility || Options::AssaultRifle::bRenderRaycastGeometry)) {
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastRebuild).count() >= 100) {
			s_lastRebuild = now;
			g_raycast->rebuild_cache(LocalPtrs::Gworld);
		}
	} else {

		s_lastRebuild = std::chrono::steady_clock::now() - std::chrono::hours(24);
	}

	lockfree_bone_cache.next_frame();

	uintptr_t PlayerArrayOffset = Offsets::PlayerArray;
	uintptr_t PlayerArray = read<uintptr_t>(LocalPtrs::GameState + PlayerArrayOffset);
	int Num = read<int>(LocalPtrs::GameState + (PlayerArrayOffset + sizeof(uintptr_t)));
	if (Num <= 0 || Num > 200) return;

	std::vector<uintptr_t> playerStatePtrs(Num);
	read_buffer(PlayerArray, playerStatePtrs.data(), Num * sizeof(uintptr_t));

	int32_t localTeamIndex = -1;
	for (int i = 0; i < Num; i++) {
		uintptr_t ps = playerStatePtrs[i];
		if (!ps) continue;
		PlayerDataBatch* pData = ReadPlayerDataBatchCached(ps);
		if (!pData || !pData->Player) {
			if (pData) ReleasePlayerData(pData);
			continue;
		}
		if (pData->Player == LocalPtrs::myplayer) {
			localTeamIndex = read<int32_t>(ps + Offsets::teamindex);
			ReleasePlayerData(pData);
			break;
		}
		ReleasePlayerData(pData);
	}

	struct CachedPlayer {
		uintptr_t playerStatePtr;
		uintptr_t Player;
		uintptr_t PlayerState;
		uintptr_t CurrentWeapon;
		uintptr_t Mesh;
		bool isBot;
	};
	std::vector<CachedPlayer> validPlayers;
	std::vector<uintptr_t> currentPlayerStates;
	validPlayers.reserve(Num);
	currentPlayerStates.reserve(Num);

	for (int i = 0; i < Num; i++) {
		uintptr_t playerStatePtr = playerStatePtrs[i];
		if (!playerStatePtr) continue;

		uint8_t botByte = read<uint8_t>(playerStatePtr + Offsets::bIsABot);
		bool isBot = (botByte & 0x8) != 0;
		if (isBot && !Options::bOnBots) continue;

		if (!Options::bOnTeammate && localTeamIndex != -1) {
			int32_t theirTeam = read<int32_t>(playerStatePtr + Offsets::teamindex);
			if (theirTeam == localTeamIndex) continue;
		}

		PlayerDataBatch* pData = ReadPlayerDataBatchCached(playerStatePtr);
		if (!pData || !pData->Player) {
			if (pData) ReleasePlayerData(pData);
			continue;
		}

		if (pData->Player == LocalPtrs::myplayer) {
			ReleasePlayerData(pData);
			continue;
		}

		if (!pData->Mesh) {
			ReleasePlayerData(pData);
			continue;
		}

		if (is_dead(pData->Player)) {
			ReleasePlayerData(pData);
			continue;
		}

		validPlayers.push_back({playerStatePtr, pData->Player, pData->PlayerState,
			pData->CurrentWeapon, pData->Mesh, isBot});
		currentPlayerStates.push_back(playerStatePtr);
		ReleasePlayerData(pData);
	}

	std::vector<EntityRenderData> entities;
	entities.reserve(validPlayers.size());

	for (auto& cp : validPlayers) {
		LocalPtrs::PlayerState = cp.PlayerState;
		LocalPtrs::Player = cp.Player;
		LocalPtrs::CurrentWeapon = cp.CurrentWeapon;
		LocalPtrs::Mesh = cp.Mesh;

		PlayerInfoBatch pinfo = ReadPlayerInfoBatchCached(LocalPtrs::PlayerState, LocalPtrs::Player, LocalPtrs::CurrentWeapon);
		BulkBoneData allBones = GetAllBones(LocalPtrs::Mesh);
		bool isVisible;
		if (Options::AssaultRifle::bRaycastVisibility) {
			Vector3 headPos = GetBoneWithRotation(LocalPtrs::Mesh, 110);
			isVisible = g_raycast->line_trace(headPos) == 0;
		} else {
			isVisible = IsEnemyVisible(LocalPtrs::Mesh);
		}

		EntityRenderData erd;
		erd.playerStatePtr = cp.playerStatePtr;
		erd.Player = cp.Player;
		erd.PlayerState = cp.PlayerState;
		erd.CurrentWeapon = cp.CurrentWeapon;
		erd.Mesh = cp.Mesh;
		erd.isBot = cp.isBot;
		erd.isVisible = isVisible;
		erd.allBones = allBones;
		erd.playerInfo = pinfo;
		erd.Head3D = allBones.bone_valid[0] ? allBones.bone_positions[0] : Vector3{};
		erd.Bottom3D = allBones.bone_valid[1] ? allBones.bone_positions[1] : Vector3{};
		{
			uintptr_t root = read<uintptr_t>(cp.Player + Offsets::RootComponent);
			erd.TargetVelocity = root ? read<Vector3>(root + Offsets::ComponentVelocity) : Vector3{};
		}
		erd.aimbotBones[0] = allBones.bone_valid[0] ? allBones.bone_positions[0] : Vector3{};
		erd.aimbotBones[1] = allBones.bone_valid[2] ? allBones.bone_positions[2] : Vector3{};
		erd.aimbotBones[2] = allBones.bone_valid[18] ? allBones.bone_positions[18] : Vector3{};
		erd.aimbotBones[3] = allBones.bone_valid[19] ? allBones.bone_positions[19] : Vector3{};
		erd.aimbotBones[4] = allBones.bone_valid[20] ? allBones.bone_positions[20] : Vector3{};
		erd.displayName = CachedGetPlayerName(cp.playerStatePtr);
		erd.displayWeapon = GetWeaponNameCached(cp.Player);
		{ ImColor c; erd.displayPlatform = GetPlatformCached(c, cp.playerStatePtr); erd.platformColor = c; }
		entities.push_back(erd);
	}

	UpdateCamera(LocalPtrs::Gworld);

	for (auto& e : entities) {
		e.cameraDistance = read_camera.Location.Distance(e.Bottom3D) / 100;
	}

	int next = (g_latestSnapshot.load(std::memory_order_acquire) + 1 + SNAPSHOT_COUNT) % SNAPSHOT_COUNT;
	if (next == g_readingSlot.load(std::memory_order_acquire)) {
		return;
	}
	g_snapshots[next].entities = std::move(entities);
	g_snapshots[next].cam = read_camera;
	g_snapshots[next].currentPlayerStates = std::move(currentPlayerStates);
	g_snapshots[next].localTeamIndex = localTeamIndex;
	g_snapshots[next].playerController = LocalPtrs::PlayerController;

	g_latestSnapshot.store(next, std::memory_order_release);

	PurgeStalePlayers(g_snapshots[next].currentPlayerStates);
}

void ActorRender() {
	ImDrawList* m = ImGui::GetForegroundDrawList();
	ImDrawList* bg_draw_list = ImGui::GetBackgroundDrawList();

	if (Options::bRadar) {
		Radar::DrawRadarBackground();
	}

	{
		char pcName[256 + 1]; DWORD pcNameLen = 256 + 1;
		GetUserNameA(pcName, &pcNameLen);
		std::string username = pcName;
		std::string barText = "Hydra | " + username;
		mvec textSize = ImGui::CalcTextSize(barText.c_str());
		float padX = 12.0f, padY = 6.0f;
		float barX = 10.0f, barY = 10.0f;
		float barW = textSize.x + padX * 2.0f;
		float barH = textSize.y + padY * 2.0f;
		float rounding = 6.0f;
		bg_draw_list->AddRectFilled(mvec(barX, barY), mvec(barX + barW, barY + barH), IM_COL32(15, 15, 20, 220), rounding);
		float lineH = 2.0f;
		float mid = barX + barW * 0.5f;
		bg_draw_list->AddRectFilledMultiColor(mvec(barX + 2, barY), mvec(mid, barY + lineH), IM_COL32(80, 40, 180, 255), IM_COL32(25, 178, 217, 255), IM_COL32(25, 178, 217, 255), IM_COL32(80, 40, 180, 255));
		bg_draw_list->AddRectFilledMultiColor(mvec(mid, barY), mvec(barX + barW - 2, barY + lineH), IM_COL32(25, 178, 217, 255), IM_COL32(80, 40, 180, 255), IM_COL32(80, 40, 180, 255), IM_COL32(25, 178, 217, 255));
		bg_draw_list->AddText(mvec(barX + padX, barY + padY), IM_COL32(210, 215, 225, 240), barText.c_str());
	}

	if (Options::bCrosshair) {
		float cx = (float)settings::CenterX;
		float cy = (float)settings::CenterY;
		float halfLen = Options::fCrosshairLength * 0.5f;
		float thk = Options::fCrosshairThickness;
		ImU32 col = IM_COL32(255, 255, 255, 255);
		m->AddLine(ImVec2(cx - halfLen, cy), ImVec2(cx + halfLen, cy), col, thk);
		m->AddLine(ImVec2(cx, cy - halfLen), ImVec2(cx, cy + halfLen), col, thk);
	}

	int idx = g_latestSnapshot.load(std::memory_order_acquire);
	if (idx < 0) return;
	g_readingSlot.store(idx, std::memory_order_release);
	FrameSnapshot& snap = g_snapshots[idx];

	camera = g_cameraBuffer[g_cameraFrontIdx.load(std::memory_order_acquire)];

	static uintptr_t lastAimTarget = 0;
	LocalPtrs::closest_mesh = 0;
	LocalPtrs::closest_distance = FLT_MAX;

	float closest_score = 0.0f;
	const float TextYFrame = ImGui::CalcTextSize("ABCDEFabcdef1234567890.,").y;

	for (auto& e : snap.entities) {
		if (!e.allBones.bone_valid[0] || !e.allBones.bone_valid[1] || e.allBones.bone_count < 2) continue;

		Vector3& Head3D = e.Head3D;
		Vector3& Bottom3D = e.Bottom3D;
		bool isVisible = e.isVisible;
		auto& allBones = e.allBones;
		auto& playerInfo = e.playerInfo;
		uintptr_t playerStatePtr = e.playerStatePtr;
		auto Distance = e.cameraDistance;

		if (Distance > 400.0f) continue;

		if (Options::bRadar && Options::bRadarDisplayPlayers) {
			auto radarColor = isVisible
				? ImVec4(Options::PlayerColor::BoxVisible[0], Options::PlayerColor::BoxVisible[1], Options::PlayerColor::BoxVisible[2], 1.0f)
				: ImVec4(Options::PlayerColor::BoxNotVisible[0], Options::PlayerColor::BoxNotVisible[1], Options::PlayerColor::BoxNotVisible[2], 1.0f);
			Radar::AddToRadar(ImGui::GetColorU32(radarColor), Bottom3D, Distance);
		}

		Vector2 Head2D = ProjectWorldToScreen(Vector3(Head3D.x, Head3D.y, Head3D.z + 20));
		Vector2 Bottom2D = ProjectWorldToScreen(Bottom3D);

		if ((Head2D.x == 0.0f && Head2D.y == 0.0f) && (Bottom2D.x == 0.0f && Bottom2D.y == 0.0f)) continue;

		if (Options::AssaultRifle::bAimbot) {
			const double distToCross = getCrossDistance(Head2D.x, Head2D.y, settings::screen_center_x, settings::screen_center_y);
			if (Options::AssaultRifle::bVisibleOnly && !isVisible) {}
			else {
				bool isKnocked = (playerInfo.bIsDBNO >> 7) & 1;
				if (Options::AssaultRifle::bKnockedPlayers && isKnocked) {}
				else if (Options::AssaultRifle::bSkipTeammates && snap.localTeamIndex != -1 && playerInfo.teamIndex == snap.localTeamIndex) {}
				else if (distToCross <= Options::AssaultRifle::fAimbotFOV) {
					float score = (float)(Options::AssaultRifle::fAimbotFOV - distToCross);
					if (e.Mesh == lastAimTarget)
						score += Options::AssaultRifle::fAimbotFOV * 0.15f;
					if (score > closest_score) {
						closest_score = score;
						LocalPtrs::closest_mesh = e.Mesh;
						LocalPtrs::closest_distance = (float)distToCross;
					}
				}
			}
		}

		if (Options::bSnapline) {
			ImU32 snaplineColor;
			if (Options::RGBMode::bEnabled && Options::PlayerColor::bSnaplineColorRGB) {
				float hue = fmodf(Options::RGBMode::rgbTime, 1.0f);
				ImVec4 rgbCol = {1.0f, 1.0f, 1.0f, 1.0f};
				ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, rgbCol.x, rgbCol.y, rgbCol.z);
				snaplineColor = ImGui::GetColorU32(rgbCol);
			} else {
				snaplineColor = isVisible
					? ImGui::GetColorU32({ Options::PlayerColor::SnaplineVisible[0], Options::PlayerColor::SnaplineVisible[1], Options::PlayerColor::SnaplineVisible[2], 1.0f })
					: ImGui::GetColorU32({ Options::PlayerColor::SnaplineNotVisible[0], Options::PlayerColor::SnaplineNotVisible[1], Options::PlayerColor::SnaplineNotVisible[2], 1.0f });
			}
			if (Options::SnaplineStart::bBottom) {
				if (Options::PlayerFeatures::bSnaplineOutline)
					bg_draw_list->AddLine(mvec(CenterX, (CenterY * 2) - 10), mvec(Bottom2D.x, Bottom2D.y + 1), IM_COL32(0, 0, 0, 255), Options::PlayerFeatures::fSnaplineOutlineThickness);
				bg_draw_list->AddLine(mvec(CenterX, (CenterY * 2) - 10), mvec(Bottom2D.x, Bottom2D.y + 1), snaplineColor, 1.0f);
			} else if (Options::SnaplineStart::bTop) {
				if (Options::PlayerFeatures::bSnaplineOutline)
					bg_draw_list->AddLine(mvec(CenterX, 10), mvec(Head2D.x, Head2D.y - 2), IM_COL32(0, 0, 0, 255), Options::PlayerFeatures::fSnaplineOutlineThickness);
				bg_draw_list->AddLine(mvec(CenterX, 10), mvec(Head2D.x, Head2D.y - 2), snaplineColor, 1.0f);
			}
		}

		if (!is_in_screen_vector2(Head2D) && !is_in_screen_vector2(Bottom2D) && !is_in_screen_vector3(Head3D) && !is_in_screen_vector3(Bottom3D)) continue;

		float BoxHeight = (float)(Head2D.y - Bottom2D.y);
		float CornerHeight = abs(Head2D.y - Bottom2D.y);
		float BoxWidth = BoxHeight * 0.60;

		const float TextY = TextYFrame;
		int BottomTextOffset = 0;
		int TopTextOffset = TextY;

		float BoxLeftX = Bottom2D.x - (BoxWidth / 2);
		float BoxRightX = Bottom2D.x + (BoxWidth / 2);
		float BoxCenterY = (Head2D.y + Bottom2D.y) / 2.0f;

		int topTextCount = 0;
		int bottomTextCount = 0;
		if (Options::bplayername) topTextCount++;
		if (Options::bplattform) topTextCount++;
		if (Options::bPlayerDistance) topTextCount++;
		if (Options::bSeasonRank) topTextCount++;
		if (Options::bPlayerInfo) bottomTextCount++;
		if (Options::bTeamIndex) bottomTextCount++;
		if (Options::bTeamScore) bottomTextCount++;
		if (Options::bWeaponName) bottomTextCount++;

		float textSpacing = 2.0f;
		float textGap = 4.0f;
		float topTextBlockHeight = topTextCount * TextY + ((topTextCount > 0 ? topTextCount - 1 : 0) * textSpacing);
		float bottomTextBlockHeight = bottomTextCount * TextY + ((bottomTextCount > 0 ? bottomTextCount - 1 : 0) * textSpacing);
		float topStartY = BoxCenterY - (topTextBlockHeight / 2.0f);
		float bottomStartY = BoxCenterY + (textGap / 2.0f);
		float currentTopSideY = topStartY;
		float currentBottomSideY = bottomStartY;

		auto ColorDistance = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		auto Colorskeleton = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		auto ColorBox = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		auto ColorSnapline = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		auto ColorBottomText = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		auto ColorTopText = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		static float rgbTime = 0.0f;
		ImVec4 rgbColor = {1.0f, 1.0f, 1.0f, 1.0f};
		if (Options::RGBMode::bEnabled) {
			rgbTime = Options::RGBMode::rgbTime;
			float hue = fmodf(rgbTime, 1.0f);
			ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, rgbColor.x, rgbColor.y, rgbColor.z);
			rgbColor.w = 1.0f;

			bool IsVis = isVisible;
			if (Options::PlayerColor::bBoxColorRGB) ColorBox = rgbColor;
			else if (IsVis) ColorBox = ImVec4(Options::PlayerColor::BoxVisible[0], Options::PlayerColor::BoxVisible[1], Options::PlayerColor::BoxVisible[2], 1.0f);
			else ColorBox = ImVec4(Options::PlayerColor::BoxNotVisible[0], Options::PlayerColor::BoxNotVisible[1], Options::PlayerColor::BoxNotVisible[2], 1.0f);
			if (Options::PlayerColor::bSkeletonColorRGB) Colorskeleton = rgbColor;
			else if (IsVis) Colorskeleton = ImVec4(Options::PlayerColor::SkeletonVisible[0], Options::PlayerColor::SkeletonVisible[1], Options::PlayerColor::SkeletonVisible[2], 1.0f);
			else Colorskeleton = ImVec4(Options::PlayerColor::SkeletonNotVisible[0], Options::PlayerColor::SkeletonNotVisible[1], Options::PlayerColor::SkeletonNotVisible[2], 1.0f);
			if (Options::PlayerColor::bSnaplineColorRGB) ColorSnapline = rgbColor;
			else if (IsVis) ColorSnapline = ImVec4(Options::PlayerColor::SnaplineVisible[0], Options::PlayerColor::SnaplineVisible[1], Options::PlayerColor::SnaplineVisible[2], 1.0f);
			else ColorSnapline = ImVec4(Options::PlayerColor::SnaplineNotVisible[0], Options::PlayerColor::SnaplineNotVisible[1], Options::PlayerColor::SnaplineNotVisible[2], 1.0f);
			if (Options::PlayerColor::bTopTextColorRGB) ColorTopText = rgbColor;
			else if (IsVis) ColorTopText = ImVec4(Options::PlayerColor::TopTextVisible[0], Options::PlayerColor::TopTextVisible[1], Options::PlayerColor::TopTextVisible[2], 1.0f);
			else ColorTopText = ImVec4(Options::PlayerColor::TopTextNotVisible[0], Options::PlayerColor::TopTextNotVisible[1], Options::PlayerColor::TopTextNotVisible[2], 1.0f);
			if (Options::PlayerColor::bBottomTextColorRGB) ColorBottomText = rgbColor;
			else if (IsVis) ColorBottomText = ImVec4(Options::PlayerColor::BottomTextVisible[0], Options::PlayerColor::BottomTextVisible[1], Options::PlayerColor::BottomTextVisible[2], 1.0f);
			else ColorBottomText = ImVec4(Options::PlayerColor::BottomTextNotVisible[0], Options::PlayerColor::BottomTextNotVisible[1], Options::PlayerColor::BottomTextNotVisible[2], 1.0f);
			ColorDistance = rgbColor;
			if (Options::PlayerColor::bBoxFillColorRGB) {
				Options::PlayerColor::BoxFillRGBColor[0] = rgbColor.x;
				Options::PlayerColor::BoxFillRGBColor[1] = rgbColor.y;
				Options::PlayerColor::BoxFillRGBColor[2] = rgbColor.z;
			}
		} else if (isVisible) {
			ColorDistance = ImVec4(Options::PlayerColor::BottomTextVisible[0], Options::PlayerColor::BottomTextVisible[1], Options::PlayerColor::BottomTextVisible[2], 1.0);
			Colorskeleton = ImVec4(Options::PlayerColor::SkeletonVisible[0], Options::PlayerColor::SkeletonVisible[1], Options::PlayerColor::SkeletonVisible[2], 1.0);
			ColorBox = ImVec4(Options::PlayerColor::BoxVisible[0], Options::PlayerColor::BoxVisible[1], Options::PlayerColor::BoxVisible[2], 1.0f);
			ColorSnapline = ImVec4(Options::PlayerColor::SnaplineVisible[0], Options::PlayerColor::SnaplineVisible[1], Options::PlayerColor::SnaplineVisible[2], 1.0f);
			ColorBottomText = ImVec4(Options::PlayerColor::BottomTextVisible[0], Options::PlayerColor::BottomTextVisible[1], Options::PlayerColor::BottomTextVisible[2], 1.0f);
			ColorTopText = ImVec4(Options::PlayerColor::TopTextVisible[0], Options::PlayerColor::TopTextVisible[1], Options::PlayerColor::TopTextVisible[2], 1.0f);
		} else {
			ColorDistance = ImVec4(Options::PlayerColor::BottomTextNotVisible[0], Options::PlayerColor::BottomTextNotVisible[1], Options::PlayerColor::BottomTextNotVisible[2], 1.0);
			Colorskeleton = ImVec4(Options::PlayerColor::SkeletonNotVisible[0], Options::PlayerColor::SkeletonNotVisible[1], Options::PlayerColor::SkeletonNotVisible[2], 1.0);
			ColorBox = ImVec4(Options::PlayerColor::BoxNotVisible[0], Options::PlayerColor::BoxNotVisible[1], Options::PlayerColor::BoxNotVisible[2], 1.0f);
			ColorSnapline = ImVec4(Options::PlayerColor::SnaplineNotVisible[0], Options::PlayerColor::SnaplineNotVisible[1], Options::PlayerColor::SnaplineNotVisible[2], 1.0f);
			ColorBottomText = ImVec4(Options::PlayerColor::BottomTextNotVisible[0], Options::PlayerColor::BottomTextNotVisible[1], Options::PlayerColor::BottomTextNotVisible[2], 1.0f);
			ColorTopText = ImVec4(Options::PlayerColor::TopTextNotVisible[0], Options::PlayerColor::TopTextNotVisible[1], Options::PlayerColor::TopTextNotVisible[2], 1.0f);
			isVisible = false;
		}

		ImU32 colTopText = ImGui::GetColorU32(ColorTopText);
		ImU32 colBottomText = ImGui::GetColorU32(ColorBottomText);
		ImU32 colSnapline = ImGui::GetColorU32(ColorSnapline);
		ImU32 colBoxU32 = ImGui::GetColorU32(ColorBox);

		if (Options::bplayername || Options::bPlayerDistance) {
			char displayText[256];
			displayText[0] = '\0';
			if (Options::bplayername && Options::bPlayerDistance) {
				float adjustedDistance = Distance - 4.0f;
				if (adjustedDistance < 0) adjustedDistance = 0;
				snprintf(displayText, sizeof(displayText), "%s (%.0fm)", e.displayName.c_str(), adjustedDistance);
			} else if (Options::bplayername) {
				snprintf(displayText, sizeof(displayText), "%s", e.displayName.c_str());
			} else if (Options::bPlayerDistance) {
				float adjustedDistance = Distance - 4.0f;
				if (adjustedDistance < 0) adjustedDistance = 0;
				snprintf(displayText, sizeof(displayText), "(%.0fm)", adjustedDistance);
			}
			mvec text_size = Render::CalcTextSizeWithFont(displayText);
			float textX = Head2D.x - (text_size.x / 2);
			float textYPos = Head2D.y - TopTextOffset - 4;
			Render::MakeText(displayText, mvec(textX, textYPos), colTopText, true, Options::PlayerFeatures::OutlinedText);
			TopTextOffset += TextY;
		}

		if (Options::bplattform) {
			mvec platform_text_size = Render::CalcTextSizeWithFont(e.displayPlatform.c_str());
			float textX = Head2D.x - (platform_text_size.x / 2);
			float textY = Head2D.y - TopTextOffset - 4;
			Render::MakeText(e.displayPlatform.c_str(), mvec(textX, textY), ImGui::GetColorU32((ImVec4)e.platformColor), true, Options::PlayerFeatures::OutlinedText);
			TopTextOffset += TextY;
		}

		if (Options::bSeasonRank) {
			const char* rankName = GetRankName(playerInfo.seasonRank);
			ImVec4 rankColor = GetRankColor(playerInfo.seasonRank);
			mvec rank_text_size = Render::CalcTextSizeWithFont(rankName);
			float textX = Head2D.x - (rank_text_size.x / 2);
			float textY = Head2D.y - TopTextOffset - 4;
			Render::MakeText(rankName, mvec(textX, textY), ImGui::GetColorU32(rankColor), true, Options::PlayerFeatures::OutlinedText);
			TopTextOffset += TextY;
		}

		ImU32 skeletonColor = ImGui::GetColorU32(Colorskeleton);
		static const EBoneIndex bonesNeeded[] = {
			EBoneIndex::Neck, EBoneIndex::Pelvis,
			EBoneIndex::LShoulder, EBoneIndex::LElbow, EBoneIndex::LHand,
			EBoneIndex::RShoulder, EBoneIndex::RElbow, EBoneIndex::RHand,
			EBoneIndex::LHip, EBoneIndex::LKnee, EBoneIndex::LFoot,
			EBoneIndex::RHip, EBoneIndex::RKnee, EBoneIndex::RFoot,
			EBoneIndex::Test, EBoneIndex::Test2
		};
		constexpr int bonesCount = (int)(sizeof(bonesNeeded) / sizeof(bonesNeeded[0]));
		std::array<mvec, bonesCount> projBones;
		std::array<char, bonesCount> projValid;

		auto indexOf = [&](EBoneIndex b) -> int {
			for (int i = 0; i < bonesCount; ++i) if (bonesNeeded[i] == b) return i;
			return -1;
		};

		auto DrawBoneChain = [&](const std::vector<EBoneIndex>& boneIndices) {
			mvec lastPoint;
			bool hasLast = false;
			for (size_t i = 0; i < boneIndices.size(); ++i) {
				int idx = indexOf(boneIndices[i]);
				if (idx >= 0 && projValid[idx]) {
					mvec current = projBones[idx];
					if (hasLast) {
						if (Options::PlayerFeatures::bSkeletonOutline) {
							float* outlineColorSrc = isVisible ? Options::PlayerColor::SkeletonOutlineVisible : Options::PlayerColor::SkeletonOutlineNotVisible;
							ImU32 outlineColor = IM_COL32(outlineColorSrc[0] * 255, outlineColorSrc[1] * 255, outlineColorSrc[2] * 255, 255);
							float dx = current.x - lastPoint.x, dy = current.y - lastPoint.y;
							float len = sqrtf(dx * dx + dy * dy);
							float ox = 0.0f, oy = 0.0f;
							if (len > 0.0f) { ox = -dy / len; oy = dx / len; }
							bg_draw_list->AddLine(ImVec2(lastPoint.x+ox, lastPoint.y+oy), ImVec2(current.x+ox, current.y+oy), outlineColor, 1.0f);
							bg_draw_list->AddLine(ImVec2(lastPoint.x-ox, lastPoint.y-oy), ImVec2(current.x-ox, current.y-oy), outlineColor, 1.0f);
						}
						bg_draw_list->AddLine(lastPoint, current, skeletonColor, Options::fSkeletonThickness);
					}
					lastPoint = current;
					hasLast = true;
				} else {
					hasLast = false;
				}
			}
		};

		auto catmullRom = [](mvec p0, mvec p1, mvec p2, mvec p3, float t) {
			float t2 = t * t;
			float t3 = t2 * t;
			return mvec(
				0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
				0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3)
			);
		};

		auto DrawSmoothBoneChain = [&](const std::vector<EBoneIndex>& boneIndices) {
			std::vector<mvec> chainPoints;
			chainPoints.reserve(boneIndices.size());
			for (size_t i = 0; i < boneIndices.size(); ++i) {
				int idx = indexOf(boneIndices[i]);
				if (idx >= 0 && projValid[idx]) chainPoints.push_back(projBones[idx]);
			}
			if (chainPoints.size() < 2) return;
			std::vector<mvec> smoothPoints;
			mvec last;
			bool first = true;
			if (chainPoints.size() < 4) {
				chainPoints.insert(chainPoints.begin(), chainPoints.front());
				chainPoints.push_back(chainPoints.back());
			} else {
				chainPoints.insert(chainPoints.begin(), chainPoints.front());
				chainPoints.push_back(chainPoints.back());
			}
			for (size_t i = 0; i + 3 < chainPoints.size(); i++) {
				for (float t = 0.0f; t <= 1.0f; t += Options::SkeletonSmooth::fStep) {
					mvec pt = catmullRom(chainPoints[i], chainPoints[i + 1], chainPoints[i + 2], chainPoints[i + 3], t);
					if (first) { last = pt; first = false; continue; }
					if (Options::PlayerFeatures::bSkeletonOutline) {
						float* outlineColorSrc = isVisible ? Options::PlayerColor::SkeletonOutlineVisible : Options::PlayerColor::SkeletonOutlineNotVisible;
						ImU32 outlineColor = IM_COL32(outlineColorSrc[0] * 255, outlineColorSrc[1] * 255, outlineColorSrc[2] * 255, 255);
						float dx = pt.x - last.x, dy = pt.y - last.y;
						float len = sqrtf(dx * dx + dy * dy);
						float ox = 0.0f, oy = 0.0f;
						if (len > 0.0f) { ox = -dy / len; oy = dx / len; }
						bg_draw_list->AddLine(ImVec2(last.x+ox, last.y+oy), ImVec2(pt.x+ox, pt.y+oy), outlineColor, 1.0f);
						bg_draw_list->AddLine(ImVec2(last.x-ox, last.y-oy), ImVec2(pt.x-ox, pt.y-oy), outlineColor, 1.0f);
					}
					bg_draw_list->AddLine(last, pt, skeletonColor, Options::fSkeletonThickness);
					last = pt;
				}
			}
		};

		if (Options::bSkeleton) {
			for (int iBone = 0; iBone < bonesCount; ++iBone) {
				Vector2 v = ProjectWorldToScreen(allBones.bone_positions[iBone + 2]);
				projValid[iBone] = (v.x > 0 && v.y > 0) ? 1 : 0;
				projBones[iBone] = mvec(v.x, v.y);
			}

			if (Options::bSmoothSkeleton) {
				DrawSmoothBoneChain({ EBoneIndex::Neck, EBoneIndex::Pelvis });
				DrawSmoothBoneChain({ EBoneIndex::LShoulder, EBoneIndex::LElbow, EBoneIndex::LHand });
				DrawSmoothBoneChain({ EBoneIndex::RShoulder, EBoneIndex::RElbow, EBoneIndex::RHand });
				DrawSmoothBoneChain({ EBoneIndex::Pelvis, EBoneIndex::LHip, EBoneIndex::LKnee, EBoneIndex::LFoot });
				DrawSmoothBoneChain({ EBoneIndex::Pelvis, EBoneIndex::RHip, EBoneIndex::RKnee, EBoneIndex::RFoot });
				DrawSmoothBoneChain({ EBoneIndex::LShoulder, EBoneIndex::RShoulder });
				DrawSmoothBoneChain({ EBoneIndex::LKnee, EBoneIndex::LFoot, EBoneIndex::LFoot, EBoneIndex::Test2 });
				DrawSmoothBoneChain({ EBoneIndex::RKnee, EBoneIndex::RFoot, EBoneIndex::RFoot, EBoneIndex::Test });
			} else {
				DrawBoneChain({ EBoneIndex::Neck, EBoneIndex::Pelvis });
				DrawBoneChain({ EBoneIndex::LShoulder, EBoneIndex::LElbow, EBoneIndex::LHand });
				DrawBoneChain({ EBoneIndex::RShoulder, EBoneIndex::RElbow, EBoneIndex::RHand });
				DrawBoneChain({ EBoneIndex::Pelvis, EBoneIndex::LHip, EBoneIndex::LKnee, EBoneIndex::LFoot });
				DrawBoneChain({ EBoneIndex::Pelvis, EBoneIndex::RHip, EBoneIndex::RKnee, EBoneIndex::RFoot });
				DrawBoneChain({ EBoneIndex::LShoulder, EBoneIndex::RShoulder });
				DrawBoneChain({ EBoneIndex::LKnee, EBoneIndex::LFoot, EBoneIndex::LFoot, EBoneIndex::Test2 });
				DrawBoneChain({ EBoneIndex::RKnee, EBoneIndex::RFoot, EBoneIndex::RFoot, EBoneIndex::Test });
			}

			if (allBones.bone_valid[0] && allBones.bone_valid[2]) {
				Vector2 head2D = ProjectWorldToScreen(allBones.bone_positions[0]);
				Vector2 neck2D = ProjectWorldToScreen(allBones.bone_positions[2]);
				if (head2D.x > 0 && head2D.y > 0 && neck2D.x > 0 && neck2D.y > 0) {
					if (Options::PlayerFeatures::bSkeletonOutline) {
						float* outlineColorSrc = isVisible ? Options::PlayerColor::SkeletonOutlineVisible : Options::PlayerColor::SkeletonOutlineNotVisible;
						ImU32 outlineColor = IM_COL32(outlineColorSrc[0] * 255, outlineColorSrc[1] * 255, outlineColorSrc[2] * 255, 255);
						float dx = neck2D.x - head2D.x, dy = neck2D.y - head2D.y;
						float len = sqrtf(dx * dx + dy * dy);
						float ox = 0.0f, oy = 0.0f;
						if (len > 0.0f) { ox = -dy / len; oy = dx / len; }
						bg_draw_list->AddLine(ImVec2(head2D.x+ox, head2D.y+oy), ImVec2(neck2D.x+ox, neck2D.y+oy), outlineColor, 1.0f);
						bg_draw_list->AddLine(ImVec2(head2D.x-ox, head2D.y-oy), ImVec2(neck2D.x-ox, neck2D.y-oy), outlineColor, 1.0f);
					}
					bg_draw_list->AddLine(mvec(head2D.x, head2D.y), mvec(neck2D.x, neck2D.y), skeletonColor, Options::fSkeletonThickness);
				}
			}
		}
		if (Options::bCharacters) {
			float BoxHeight = Bottom2D.y - Head2D.y;
			float BoxWidth = BoxHeight * 0.60f;
			int X = Bottom2D.x - (BoxWidth / 2);
			RenderBox(X, Head2D.y, BoxWidth, BoxHeight, ColorBox, Options::CharacterModes::fBoxThickness, true, Options::CharacterModes::box_type, isVisible);
		}

		std::string player_info_text = "";
		mvec player_info_text_size = mvec(0, 0);

		if (Options::bPlayerInfo) {
			bool IsTargetting = (playerInfo.targetFlags & (1 << 1)) != 0;
			bool bIsPlayingEmote = (playerInfo.emoteFlags & 0x80) != 0;
			bool bIsDBNO = playerInfo.bIsDBNO >> 7 & 1;

			if (bIsDBNO) player_info_text = "Knocked";
			else if (bIsPlayingEmote) player_info_text = "Emoting";
			else if (playerInfo.IsSkydiving) player_info_text = "Skydiving";
			else if (playerInfo.IsEquipping) player_info_text = "Swapping";
			else if (IsTargetting) player_info_text = "Aiming";
			else if (playerInfo.IsCharging) player_info_text = "Reloading";
			else if (isVisible) player_info_text = "Visible";
			else player_info_text = "Not Visible";

			if (!player_info_text.empty()) {
				mvec info_text_size = Render::CalcTextSizeWithFont(player_info_text.c_str());
				float textX = Bottom2D.x - (info_text_size.x / 2);
				float textY = Bottom2D.y + BottomTextOffset + 4;
				Render::MakeText(player_info_text.c_str(), mvec(textX, textY), colBottomText, true, Options::PlayerFeatures::OutlinedText);
				BottomTextOffset += TextY;
			}
		}

		if (Options::bTeamIndex) {
			char teamText[32];
			snprintf(teamText, sizeof(teamText), "T:%d", playerInfo.teamIndex);
			std::string teamStr = teamText;
			snprintf(teamText, sizeof(teamText), "%s", teamStr.c_str());
			mvec team_text_size = Render::CalcTextSizeWithFont(teamText);
			float textX = Bottom2D.x - (team_text_size.x / 2);
			float textY = Bottom2D.y + BottomTextOffset + 4;
			Render::MakeText(teamText, mvec(textX, textY), colBottomText, true, Options::PlayerFeatures::OutlinedText);
			BottomTextOffset += TextY;
		}

		if (Options::bTeamScore) {
			char scoreText[32];
			snprintf(scoreText, sizeof(scoreText), "K:%d", playerInfo.teamScore);
			std::string scoreStr = scoreText;
			snprintf(scoreText, sizeof(scoreText), "%s", scoreStr.c_str());
			mvec score_text_size = Render::CalcTextSizeWithFont(scoreText);
			float textX = Bottom2D.x - (score_text_size.x / 2);
			float textY = Bottom2D.y + BottomTextOffset + 4;
			Render::MakeText(scoreText, mvec(textX, textY), colBottomText, true, Options::PlayerFeatures::OutlinedText);
			BottomTextOffset += TextY;
		}

		if (Options::bWeaponName) {
			mvec weapon_text_size = Render::CalcTextSizeWithFont(e.displayWeapon.c_str());
			float textX = Bottom2D.x - (weapon_text_size.x / 2);
			float textY = Bottom2D.y + BottomTextOffset + 4;
			Render::MakeText(e.displayWeapon.c_str(), mvec(textX, textY), colBottomText, true, Options::PlayerFeatures::OutlinedText);
			BottomTextOffset += TextY;
		}
	}

	lastAimTarget = LocalPtrs::closest_mesh;

	bool aimKeyPressed = false;
	if (Aimkey::bShift && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bMouse4 && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bMouse5 && (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bMouseRight && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bCapsLock && (GetAsyncKeyState(VK_CAPITAL) & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bR && (GetAsyncKeyState('R') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bF && (GetAsyncKeyState('F') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bE && (GetAsyncKeyState('E') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bG && (GetAsyncKeyState('G') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bQ && (GetAsyncKeyState('Q') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bT && (GetAsyncKeyState('T') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bV && (GetAsyncKeyState('V') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bB && (GetAsyncKeyState('B') & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bCtrl && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) aimKeyPressed = true;
	if (Aimkey::bTab && (GetAsyncKeyState(VK_TAB) & 0x8000)) aimKeyPressed = true;

	if (Options::AssaultRifle::bAimbot && aimKeyPressed) {
		if (LocalPtrs::closest_mesh) {
			for (auto& e : snap.entities) {
				if (e.Mesh == LocalPtrs::closest_mesh && snap.playerController) {
					Vector3 aimOffset = memoryaim(e);
					write<Vector3>(snap.playerController + Offsets::PlayerAimOffset, aimOffset);
					break;
				}
			}
		}
	}

	if (Options::AssaultRifle::bFillFOV) {
		ImVec4 fovCol;
		if (Options::RGBMode::bEnabled && Options::AssaultRifle::bFOVCircleRGB) {
			float hue = fmodf(Options::RGBMode::rgbTime, 1.0f);
			fovCol = ImVec4(1.0f, 1.0f, 1.0f, Options::AssaultRifle::FOVCircleColor[3]);
			ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, fovCol.x, fovCol.y, fovCol.z);
		} else {
			fovCol = ImVec4(Options::AssaultRifle::FOVCircleColor[0], Options::AssaultRifle::FOVCircleColor[1],
				Options::AssaultRifle::FOVCircleColor[2], Options::AssaultRifle::FOVCircleColor[3]);
		}
		ImVec2 fovCenter = mvec(settings::screen_center_x, settings::screen_center_y);
		float fovRadius = Options::AssaultRifle::fAimbotFOV;
		if (Options::AssaultRifle::bFillFOVInside) {
			ImVec4 fillCol = fovCol;
			fillCol.w *= 0.25f;
			ImU32 fovFillColor = ImGui::GetColorU32(fillCol);
			bg_draw_list->AddCircleFilled(fovCenter, fovRadius, fovFillColor, 64);
		}
		ImVec4 outlineCol = fovCol;
		outlineCol.w = 1.0f;
		ImU32 fovOutlineColor = ImGui::GetColorU32(outlineCol);
		bg_draw_list->AddCircle(fovCenter, fovRadius, fovOutlineColor, 64, Options::AssaultRifle::fFOVCircleThickness);
	}

	if (Options::AssaultRifle::bAimbotLine && LocalPtrs::closest_mesh && LocalPtrs::closest_distance) {
		bool renderLine = true;
		if (Options::AssaultRifle::bAimbotLineHoldWhileAiming) {
			if (!aimKeyPressed) renderLine = false;
		}
		if (renderLine) {
			for (auto& e : snap.entities) {
				if (e.Mesh != LocalPtrs::closest_mesh) continue;

				int lineBoneId = Options::AssaultRifle::current_hitbox;
				if (AimTarget::bClosestBone) {
					float closestDist = FLT_MAX;
					int idxMap[] = {0, 2, 1, 3, 4};
					int bones[] = { BoneIDS::head, BoneIDS::chest, BoneIDS::neck, BoneIDS::rightThigh, BoneIDS::leftThigh };
					for (int i = 0; i < 5; i++) {
						Vector2 b2d = ProjectWorldToScreen(e.aimbotBones[idxMap[i]]);
						if (b2d.x == 0 || b2d.y == 0) continue;
						float d = sqrtf(powf(b2d.x - settings::screen_center_x, 2) + powf(b2d.y - settings::screen_center_y, 2));
						if (d < closestDist) { closestDist = d; lineBoneId = bones[i]; }
					}
				}
				int aimbotIdx = 0;
				if (lineBoneId == BoneIDS::head) aimbotIdx = 0;
				else if (lineBoneId == BoneIDS::neck) aimbotIdx = 1;
				else if (lineBoneId == BoneIDS::chest) aimbotIdx = 2;
				else if (lineBoneId == BoneIDS::rightThigh) aimbotIdx = 3;
				else if (lineBoneId == BoneIDS::leftThigh) aimbotIdx = 4;

				Vector2 target2D = ProjectWorldToScreen(e.aimbotBones[aimbotIdx]);
				if (target2D.x > 0 && target2D.y > 0) {
					ImU32 aimbotLineColor;
					if (PlayerColor::bAimbotLineColorRGB) {
						aimbotLineColor = ImGui::GetColorU32(ImVec4(PlayerColor::AimbotLineRGBColor[0], PlayerColor::AimbotLineRGBColor[1],
							PlayerColor::AimbotLineRGBColor[2], PlayerColor::AimbotLineRGBColor[3]));
					} else {
						if (e.isVisible) {
							aimbotLineColor = ImGui::GetColorU32(ImVec4(PlayerColor::AimbotLineVisible[0], PlayerColor::AimbotLineVisible[1],
								PlayerColor::AimbotLineVisible[2], PlayerColor::AimbotLineVisible[3]));
						} else {
							aimbotLineColor = ImGui::GetColorU32(ImVec4(PlayerColor::AimbotLineNotVisible[0], PlayerColor::AimbotLineNotVisible[1],
								PlayerColor::AimbotLineNotVisible[2], PlayerColor::AimbotLineNotVisible[3]));
						}
					}
					bg_draw_list->AddLine(mvec(settings::screen_center_x, settings::screen_center_y), mvec(target2D.x, target2D.y), aimbotLineColor, Options::AssaultRifle::fAimbotLineThickness);
				}
				break;
			}
		}
	}

	if (Options::AssaultRifle::bRenderRaycastGeometry) {
		g_raycast->render_geometry();
	}

	g_readingSlot.store(-1, std::memory_order_release);
}
