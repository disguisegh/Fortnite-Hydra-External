#pragma once
#include <Windows.h>
#include <stdio.h>
#include <cfloat>
#include "../../settings/settings.h"
#include "../../math/math.h"
#include "../../sdk-fname-offsets/Sdk/sdk.h"
#include "../../Driver/communication.h"

using namespace Options::AssaultRifle;

inline Vector3 PredictTarget(const Vector3& bone_pos, const Vector3& velocity)
{
	if (!Options::AssaultRifle::bPrediction || g_localProjectileSpeed <= 0.f || g_bIsLocalPickaxe)
		return bone_pos;

	float dist = static_cast<float>(camera.Location.Distance(bone_pos));
	float travel_time = dist / g_localProjectileSpeed;
	if (travel_time > 2.0f) travel_time = 2.0f;

	Vector3 predicted = bone_pos + velocity * travel_time;

	float new_dist = static_cast<float>(camera.Location.Distance(predicted));
	travel_time = new_dist / g_localProjectileSpeed;
	if (travel_time > 2.0f) travel_time = 2.0f;

	predicted = bone_pos + velocity * travel_time;

	float drop = 0.5f * 980.0f * fabsf(g_localProjectileGravity) * travel_time * travel_time;
	if (drop > 200.0f) drop = 200.0f;
	predicted.z += drop;

	return predicted;
}

Vector3 memoryaim(const EntityRenderData& erd)
{
	if (!erd.Mesh) return Vector3{};
	if (bVisibleOnly && !erd.isVisible) return Vector3{};

	int bone_id = current_hitbox;

	auto getAimbotIndex = [](int id) -> int {
		if (id == BoneIDS::head) return 0;
		if (id == BoneIDS::neck) return 1;
		if (id == BoneIDS::chest) return 2;
		if (id == BoneIDS::rightThigh) return 3;
		if (id == BoneIDS::leftThigh) return 4;
		return 0;
	};

	if (AimTarget::bClosestBone) {
		float closestDist = FLT_MAX;
		int closestBone = BoneIDS::head;
		int idxMap[] = {0, 2, 1, 3, 4};
		int bones[] = { BoneIDS::head, BoneIDS::chest, BoneIDS::neck, BoneIDS::rightThigh, BoneIDS::leftThigh };
		static int activeBone = BoneIDS::head;
		float activeDist = FLT_MAX;

		for (int i = 0; i < 5; i++) {
			Vector2 bone2d = ProjectWorldToScreen(erd.aimbotBones[idxMap[i]]);
			if (bone2d.x == 0 || bone2d.y == 0) continue;

			float dist = sqrtf(powf(bone2d.x - settings::screen_center_x, 2) +
				powf(bone2d.y - settings::screen_center_y, 2));
			if (bones[i] == activeBone)
				activeDist = dist;
			if (dist < closestDist) {
				closestDist = dist;
				closestBone = bones[i];
			}
		}

		if (activeDist != FLT_MAX && closestDist >= activeDist * 0.8f)
			closestBone = activeBone;

		activeBone = closestBone;
		bone_id = closestBone;
	}

	Vector3 head3d = erd.aimbotBones[getAimbotIndex(bone_id)];
	head3d = PredictTarget(head3d, erd.TargetVelocity);
	Vector2 target2d = ProjectWorldToScreen(head3d);

	if (target2d.x == 0 || target2d.y == 0) return Vector3{};
	if (target2d.x < 0 || target2d.y < 0) return Vector3{};

	if (!std::isfinite(target2d.x) || !std::isfinite(target2d.y)) return Vector3{};
	if (target2d.x < 0 || target2d.x > Width) return Vector3{};
	if (target2d.y < 0 || target2d.y > Height) return Vector3{};

	float delta_x = target2d.x - (float)settings::screen_center_x;
	float delta_y = target2d.y - (float)settings::screen_center_y;

	if (fabsf(delta_x) < 1.0f && fabsf(delta_y) < 1.0f)
		return Vector3{};

	float deg_per_px = camera.FieldOfView / (Height / 2.0f) * 0.5f;

	float pitch = (-delta_y / (fAimbotSmooth * 0.5f)) * deg_per_px;
	float yaw = (delta_x / (fAimbotSmooth * 0.5f)) * deg_per_px;

	return Vector3(pitch, yaw, 0);
}
