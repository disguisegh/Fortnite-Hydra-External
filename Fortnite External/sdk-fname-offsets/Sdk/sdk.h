#pragma once
#include <math.h>
#include <cmath>
#include <numbers>
#include <queue>
#include <mutex>
#include <atomic>
#include <cstring>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <iostream>
#include <iomanip>
#include <intrin.h>
#include "../../settings/settings.h"
#include "../Offsets/offsets.hpp"
#include "../../math/math.h"
#include "../../Driver/communication.h"

extern void LogPrintf(const char* fmt, ...);

class raycast_c;
extern raycast_c* g_raycast;

std::string WStringToUtf8(const std::wstring& wstr) {
	if (wstr.empty()) return {};

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);

	std::string result(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &result[0], size_needed, nullptr, nullptr);

	return result;
}

struct NameCacheEntry {
	std::string name;
	std::chrono::high_resolution_clock::time_point lastUpdate;
};

static std::unordered_map<uintptr_t, NameCacheEntry> g_nameCache;
static std::mutex g_nameCacheMutex;
static std::atomic<uint64_t> g_frameCounter{0};

inline void IncrementFrameCounter() {
	g_frameCounter.fetch_add(1, std::memory_order_relaxed);
}

namespace LocalPtrs {
	inline uintptr_t myplayer;
	inline uintptr_t Player;
	inline uintptr_t Mesh;
	inline uintptr_t PlayerState ;
	inline uintptr_t CurrentWeapon;
	inline uintptr_t RootComponent;
	inline uintptr_t LocalPlayers;
	inline uintptr_t PlayerController;
	inline uintptr_t closest_entity;
	inline bool closest_visible;
	inline uintptr_t GameState;
	inline uintptr_t GameInstance;
	inline uintptr_t Gworld;

}

struct GameDataBatch {
	uint64_t Gworld;
	uint64_t GameInstance;
	uint64_t LocalPlayers;
	uint64_t PlayerController;
	uintptr_t GameState;
	uint64_t myplayer;
	uint64_t RootComponent;

	GameDataBatch() : Gworld(0), GameInstance(0), LocalPlayers(0), PlayerController(0),
	                 GameState(0), myplayer(0), RootComponent(0) {}
};

struct PlayerDataBatch {
	uintptr_t PlayerState;
	uintptr_t Player;
	uintptr_t CurrentWeapon;
	uintptr_t Mesh;

	PlayerDataBatch() : PlayerState(0), Player(0), CurrentWeapon(0), Mesh(0) {}
};

struct PlayerInfoBatch {
	uint8_t emoteFlags;
	uint8_t targetFlags;
	bool IsSkydiving;
	bool IsEquipping;
	bool IsCharging;
	uintptr_t bIsDBNO;
	int32_t ammoCount;
	int32_t teamIndex;
	int32_t teamScore;
	int32_t seasonRank;
	PlayerInfoBatch() : emoteFlags(0), targetFlags(0), IsSkydiving(false),
	                   IsEquipping(false), IsCharging(false), bIsDBNO(0), ammoCount(0),
	                   teamIndex(0), teamScore(0), seasonRank(-1) {}
};

struct PlayerInfoCacheKey {
	uintptr_t playerState;
	uintptr_t currentWeapon;
	bool operator==(const PlayerInfoCacheKey& o) const {
		return playerState == o.playerState && currentWeapon == o.currentWeapon;
	}
};

struct PlayerInfoCacheKeyHash {
	size_t operator()(const PlayerInfoCacheKey& k) const {
		return std::hash<uintptr_t>{}(k.playerState) ^ (std::hash<uintptr_t>{}(k.currentWeapon) << 1);
	}
};

struct PlayerInfoCacheEntry {
	PlayerInfoBatch info;
	std::chrono::high_resolution_clock::time_point lastUpdate;
	bool valid = false;
};

static std::unordered_map<PlayerInfoCacheKey, PlayerInfoCacheEntry, PlayerInfoCacheKeyHash> g_playerInfoCache;
static std::mutex g_playerInfoCacheMutex;

struct PlayerDataCacheEntry {
	PlayerDataBatch data;
	std::chrono::high_resolution_clock::time_point lastUpdate;
	bool valid = false;
};

static std::unordered_map<uintptr_t, PlayerDataCacheEntry> g_playerDataCache;
static std::mutex g_playerDataCacheMutex;

struct PlatformCacheEntry {
	std::string platform;
	ImColor color;
	std::chrono::high_resolution_clock::time_point lastUpdate;
	bool valid = false;
};
static std::unordered_map<uintptr_t, PlatformCacheEntry> g_platformCache;
static std::mutex g_platformCacheMutex;

struct WeaponNameCacheEntry {
	std::string name;
	std::chrono::high_resolution_clock::time_point lastUpdate;
	bool valid = false;
};
static std::unordered_map<uintptr_t, WeaponNameCacheEntry> g_weaponNameCache;
static std::mutex g_weaponNameCacheMutex;

inline uint64_t rotl64(uint64_t v, int n) {
	n &= 63;
	return (v << n) | (v >> (64 - n));
}

inline uintptr_t DecryptUWorld(uint64_t encrypted)
{
	if (!encrypted) return 0;
	const uint64_t x = encrypted ^ 0x012F546CULL;
	return static_cast<uintptr_t>(_byteswap_uint64(x) - 1274101633ULL);
}

inline bool LooksLikeUserPtr(uint64_t p) {
	return p >= 0x10000ULL && p <= 0x00007FFFFFFFFFFFULL && (p & 7ULL) == 0;
}

inline bool ValidateWorldChain(uint64_t gworld, uint64_t* outGi = nullptr, uint64_t* outLp = nullptr) {
	if (!LooksLikeUserPtr(gworld)) return false;
	const uint64_t gi = read<uint64_t>(gworld + Offsets::OwningGameInstance);
	if (!LooksLikeUserPtr(gi)) return false;
	const uint64_t lpArr = read<uint64_t>(gi + Offsets::LocalPlayers);
	if (!LooksLikeUserPtr(lpArr)) return false;
	const uint64_t lp = read<uint64_t>(lpArr);
	if (!LooksLikeUserPtr(lp)) return false;
	if (outGi) *outGi = gi;
	if (outLp) *outLp = lp;
	return true;
}

GameDataBatch ReadGameDataBatch(uintptr_t baseAddress) {
	GameDataBatch batch;

	uint64_t encrypted = read<uint64_t>(baseAddress + Offsets::UWorld);
	if (!encrypted) return batch;

	const uint64_t decrypted = DecryptUWorld(encrypted);
	const uint64_t legacy =
		static_cast<uint64_t>(0xA323219E066D436DuLL * encrypted - 0x6FA832BFC5B44018uLL);
	uint64_t candidates[3] = { decrypted, encrypted, legacy };

	uint64_t final_gworld = 0;
	uint64_t temp_gameInstance = 0;
	uint64_t temp_localPlayers = 0;
	int which = -1;
	for (int i = 0; i < 3; ++i) {
		if (ValidateWorldChain(candidates[i], &temp_gameInstance, &temp_localPlayers)) {
			final_gworld = candidates[i];
			which = i;
			break;
		}
	}
	{
		static int s_logOnce = 0;
		if (s_logOnce < 3) {
			++s_logOnce;
			LogPrintf("[GWorld] base=0x%llX enc=0x%llX dec=0x%llX ok=%d mode=%d gi=0x%llX\n",
				(unsigned long long)baseAddress,
				(unsigned long long)encrypted,
				(unsigned long long)decrypted,
				final_gworld ? 1 : 0,
				which,
				(unsigned long long)temp_gameInstance);
		}
	}
	if (!final_gworld) return batch;

	uint64_t temp_playerController = read<uint64_t>(temp_localPlayers + Offsets::PlayerController);
	if (!LooksLikeUserPtr(temp_playerController)) return batch;

	uintptr_t temp_gameState = read<uintptr_t>(final_gworld + Offsets::GameState);
	if (!LooksLikeUserPtr(temp_gameState)) return batch;

	uint64_t temp_myplayer = read<uint64_t>(temp_playerController + Offsets::AcknowledgedPawn);
	uint64_t temp_rootComponent = 0;
	if (LooksLikeUserPtr(temp_myplayer))
		temp_rootComponent = read<uint64_t>(temp_myplayer + Offsets::RootComponent);

	batch.Gworld = final_gworld;
	batch.GameInstance = temp_gameInstance;
	batch.LocalPlayers = temp_localPlayers;
	batch.PlayerController = temp_playerController;
	batch.GameState = temp_gameState;
	batch.myplayer = LooksLikeUserPtr(temp_myplayer) ? temp_myplayer : 0;
	batch.RootComponent = LooksLikeUserPtr(temp_rootComponent) ? temp_rootComponent : 0;

	return batch;
}

inline PlayerDataBatch* AcquirePlayerData() {
	return new PlayerDataBatch();
}

inline void ReleasePlayerData(PlayerDataBatch* batch) {
	delete batch;
}

inline PlayerDataBatch* ReadPlayerDataBatchPooled(uintptr_t playerState) {
	PlayerDataBatch* batch = AcquirePlayerData();
	if (!batch) return nullptr;

	batch->PlayerState = playerState;

	batch->Player = read<uintptr_t>(playerState + Offsets::PawnPrivate);
	if (!batch->Player) {
		ReleasePlayerData(batch);
		return nullptr;
	}

	{
		const size_t span_start = static_cast<size_t>(Offsets::Mesh);
		const size_t span_end = static_cast<size_t>(Offsets::CurrentWeapon) + sizeof(uintptr_t);
		const size_t span_size = span_end - span_start;

		std::vector<char> buf(span_size);
		read_buffer(batch->Player + span_start, buf.data(), static_cast<uint32_t>(span_size));
		memcpy(&batch->Mesh, buf.data(), sizeof(uintptr_t));
		memcpy(&batch->CurrentWeapon, buf.data() + (static_cast<size_t>(Offsets::CurrentWeapon) - span_start), sizeof(uintptr_t));
	}

	return batch;
}

inline PlayerDataBatch* ReadPlayerDataBatchCached(uintptr_t playerState) {
	auto now = std::chrono::high_resolution_clock::now();

	{
		std::lock_guard<std::mutex> lock(g_playerDataCacheMutex);
		auto it = g_playerDataCache.find(playerState);
		if (it != g_playerDataCache.end() && it->second.valid) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastUpdate).count();
			if (elapsed < 100) {
				PlayerDataBatch* batch = AcquirePlayerData();
				if (batch) {
					*batch = it->second.data;
				}
				return batch;
			}
		}
	}

	PlayerDataBatch* fresh = ReadPlayerDataBatchPooled(playerState);
	if (!fresh) return nullptr;

	{
		std::lock_guard<std::mutex> lock(g_playerDataCacheMutex);
		PlayerDataCacheEntry& entry = g_playerDataCache[playerState];
		entry.data = *fresh;
		entry.lastUpdate = now;
		entry.valid = true;
	}

	return fresh;
}

struct BoneCache {
	Vector3 head;
	Vector3 bottom;
	std::chrono::high_resolution_clock::time_point last_update;
	bool valid;
	BoneCache() : head(0, 0, 0), bottom(0, 0, 0), valid(false) {
		last_update = std::chrono::high_resolution_clock::now();
	}
};

class LockFreeBoneCache {
private:
	static constexpr size_t CACHE_SIZE = 256;

	struct CacheEntry {
		uintptr_t mesh;
		BoneCache cache;
		std::atomic<uint32_t> version{0};
	};

	std::array<CacheEntry, CACHE_SIZE> buffers[2];
	std::atomic<int> active_buffer{0};
	std::atomic<uint32_t> frame_counter{0};

	static constexpr uint32_t hash_mesh(uintptr_t mesh) {
		return static_cast<uint32_t>(mesh ^ (mesh >> 16)) % CACHE_SIZE;
	}

public:
	BoneCache* get_or_create(uintptr_t mesh) {
		uint32_t frame = frame_counter.load(std::memory_order_relaxed);
		int buffer_idx = frame & 1;
		uint32_t hash = hash_mesh(mesh);

		CacheEntry& entry = buffers[buffer_idx][hash];

		if (entry.mesh == mesh && entry.cache.valid) {
			auto now = std::chrono::high_resolution_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.cache.last_update).count() < 16) {
				return &entry.cache;
			}
		}

		entry.mesh = mesh;
		entry.cache.valid = false;
		entry.version.store(frame, std::memory_order_release);

		return &entry.cache;
	}

	void next_frame() {
		frame_counter.fetch_add(1, std::memory_order_relaxed);
	}

	void clear() {
		for (int i = 0; i < 2; ++i) {
			for (auto& entry : buffers[i]) {
				entry.mesh = 0;
				entry.cache.valid = false;
				entry.version.store(0, std::memory_order_relaxed);
			}
		}
	}
};

static LockFreeBoneCache lockfree_bone_cache;

struct BoneArrayResolved {
	uint32_t array_off = 0;
	uint32_t active_off = 0;
	uint32_t c2w_off = 0;
	bool     use_cached_cs = false;
	bool     valid = false;
};
inline BoneArrayResolved g_boneRes{};
inline int g_boneProbeLogs = 0;

inline bool LooksLikeBoneArray(const tarray<FTransform>& arr) {
	if (!arr.data || arr.count < 10 || arr.count > 400) return false;
	if (!IsUserPtr(reinterpret_cast<uint64_t>(arr.data))) return false;

	FTransform t0 = read<FTransform>(reinterpret_cast<uintptr_t>(arr.data));
	if (!std::isfinite(t0.Translation.x) || !std::isfinite(t0.Translation.y)) return false;
	return true;
}

inline bool ResolveBoneArray(DWORD_PTR mesh, tarray<FTransform>& outArr, FTransform& outC2W) {
	if (!mesh) return false;

	if (g_boneRes.valid) {
		if (g_boneRes.use_cached_cs) {
			outArr = read<tarray<FTransform>>(mesh + 0x9B0);
			if (LooksLikeBoneArray(outArr)) {
				outC2W = read<FTransform>(mesh + g_boneRes.c2w_off);
				return true;
			}
			g_boneRes.valid = false;
		} else {
			uintptr_t base = mesh + g_boneRes.array_off;
			if (g_boneRes.active_off) {
				int32_t sel = read<int32_t>(mesh + g_boneRes.active_off);
				if (sel < 0 || sel > 1) sel = 0;
				base = mesh + g_boneRes.array_off + (uint32_t)sel * 0x10;
			}
			outArr = read<tarray<FTransform>>(base);
			if (LooksLikeBoneArray(outArr)) {
				outC2W = read<FTransform>(mesh + g_boneRes.c2w_off);
				return true;
			}
			g_boneRes.valid = false;
		}
	}

	static const uint32_t kC2W[] = { 0x1E0, 0x1C0, 0x1D0, 0x1F0, 0x200, 0x1B0, 0x220 };
	uint32_t bestC2W = Offsets::ComponentToWorld;
	for (uint32_t c : kC2W) {
		FTransform ct = read<FTransform>(mesh + c);

		double mag = fabs(ct.Translation.x) + fabs(ct.Translation.y) + fabs(ct.Translation.z);
		if (mag > 10.0 && mag < 1e8 && std::isfinite(mag)) {
			bestC2W = c;
			break;
		}
	}

	{
		auto arr = read<tarray<FTransform>>(mesh + 0x9B0);
		if (LooksLikeBoneArray(arr)) {
			g_boneRes = { 0x9B0, 0, bestC2W, true, true };
			outArr = arr;
			outC2W = read<FTransform>(mesh + bestC2W);
			if (g_boneProbeLogs < 5) {
				++g_boneProbeLogs;
				LogPrintf("[BONE] resolved CachedComponentSpaceTransforms@0x9B0 count=%d c2w=0x%X\n",
					arr.count, bestC2W);
			}
			return true;
		}
	}

	static const uint32_t kArr[] = {
		0x5E8, 0x5F0, 0x5F8, 0x600, 0x608, 0x610, 0x618, 0x620,
		0x628, 0x630, 0x640, 0x648, 0x650, 0x658, 0x660, 0x670,
		0x680, 0x690, 0x6A0, 0x6B0, 0x6C0, 0x700, 0x720
	};
	static const uint32_t kActiveRel[] = { 0x48, 0x50, 0x60, 0x68, 0x70, 0x40 };

	for (uint32_t aoff : kArr) {

		auto arr0 = read<tarray<FTransform>>(mesh + aoff);
		if (LooksLikeBoneArray(arr0)) {
			g_boneRes = { aoff, 0, bestC2W, false, true };
			outArr = arr0;
			outC2W = read<FTransform>(mesh + bestC2W);
			if (g_boneProbeLogs < 5) {
				++g_boneProbeLogs;
				LogPrintf("[BONE] resolved array@0x%X count=%d c2w=0x%X\n", aoff, arr0.count, bestC2W);
			}
			return true;
		}

		for (uint32_t rel : kActiveRel) {
			int32_t sel = read<int32_t>(mesh + aoff + rel);
			if (sel != 0 && sel != 1) continue;
			auto arr = read<tarray<FTransform>>(mesh + aoff + sel * 0x10);
			if (LooksLikeBoneArray(arr)) {
				g_boneRes = { aoff, aoff + rel, bestC2W, false, true };
				outArr = arr;
				outC2W = read<FTransform>(mesh + bestC2W);
				if (g_boneProbeLogs < 5) {
					++g_boneProbeLogs;
					LogPrintf("[BONE] resolved array@0x%X active@+0x%X sel=%d count=%d c2w=0x%X\n",
						aoff, rel, sel, arr.count, bestC2W);
				}
				return true;
			}
		}
	}

	if (g_boneProbeLogs < 8) {
		++g_boneProbeLogs;
		LogPrintf("[BONE] FAILED probe mesh=0x%llX (no bone TArray)\n",
			(unsigned long long)mesh);
	}
	return false;
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id) {
	tarray<FTransform> bone_array{};
	FTransform ComponentToWorld{};
	if (!ResolveBoneArray(mesh, bone_array, ComponentToWorld))
		return Vector3(0, 0, 0);
	if (id < 0 || id >= bone_array.count) return Vector3(0, 0, 0);
	auto BoneTransform = read<FTransform>((uintptr_t)bone_array.data + (id * 0x60));
	Matrix4x4 Matrix = MatrixMultiplication(BoneTransform.ToMatrixWithScale().ToMatrix4x4(), ComponentToWorld.ToMatrixWithScale().ToMatrix4x4());
	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

struct VisibilityEntry {
	bool visible;
	std::chrono::steady_clock::time_point lastUpdate;
};
std::unordered_map<uintptr_t, VisibilityEntry> g_visibilityCache;
std::mutex g_visibilityCacheMutex;

struct BulkBoneData {
	std::array<Vector3, 21> bone_positions;
	std::array<bool, 21> bone_valid;
	int bone_count;
	int valid_count;

	BulkBoneData() : bone_count(0), valid_count(0) {
		bone_valid.fill(false);
	}
};

inline Vector3 GetActorWorldLocation(uintptr_t pawn) {
	if (!pawn) return Vector3{};
	uintptr_t root = read<uintptr_t>(pawn + Offsets::RootComponent);
	if (!root || !IsUserPtr(root)) return Vector3{};

	for (uint32_t c2w : { (uint32_t)0x1E0, (uint32_t)0x1C0, (uint32_t)0x1D0, (uint32_t)Offsets::ComponentToWorld }) {
		FTransform ct = read<FTransform>(root + c2w);
		double mag = fabs(ct.Translation.x) + fabs(ct.Translation.y) + fabs(ct.Translation.z);
		if (mag > 10.0 && mag < 1e8 && std::isfinite(mag))
			return Vector3(ct.Translation.x, ct.Translation.y, ct.Translation.z);
	}

	return read<Vector3>(root + Offsets::relative_location);
}

BulkBoneData GetBonesBulk(DWORD_PTR mesh, const std::vector<int>& bone_ids) {
	BulkBoneData result;
	if (bone_ids.empty() || bone_ids.size() > 21) return result;

	tarray<FTransform> bone_array{};
	FTransform ComponentToWorld{};
	if (!ResolveBoneArray(mesh, bone_array, ComponentToWorld)) {
		result.bone_count = 0;
		result.valid_count = 0;
		return result;
	}

	std::vector<FTransform> all_bones(bone_array.count);
	read_buffer((uintptr_t)bone_array.data, all_bones.data(), bone_array.count * sizeof(FTransform));

	int valid = 0;
	for (size_t i = 0; i < bone_ids.size(); ++i) {
		if (bone_ids[i] < 0 || bone_ids[i] >= bone_array.count) continue;
		Matrix4x4 Matrix = MatrixMultiplication(all_bones[bone_ids[i]].ToMatrixWithScale().ToMatrix4x4(), ComponentToWorld.ToMatrixWithScale().ToMatrix4x4());
		Vector3 p(Matrix._41, Matrix._42, Matrix._43);
		if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) continue;

		if (fabs(p.x) + fabs(p.y) + fabs(p.z) < 1.0) continue;
		result.bone_positions[i] = p;
		result.bone_valid[i] = true;
		++valid;
	}

	result.bone_count = static_cast<int>(bone_ids.size());
	result.valid_count = valid;
	return result;
}

BulkBoneData GetSkeletonBones(DWORD_PTR mesh) {
	static const std::vector<int> skeleton_bone_ids = {
		67, 2,
		9, 10, 11,
		38, 39, 40,
		71, 72, 75,
		78, 79, 82,
		83, 76
	};

	return GetBonesBulk(mesh, skeleton_bone_ids);
}

BulkBoneData GetAllBones(DWORD_PTR mesh) {
	static const std::vector<int> all_bone_ids = {
		110, 0,
		67, 2, 9, 10, 11, 38, 39, 40, 71, 72, 75, 78, 79, 82, 83, 76,
		7, 77, 84
	};

	return GetBonesBulk(mesh, all_bone_ids);
}

Vector3 GetBoneWithRotationCached(DWORD_PTR mesh, int id) {
	if (id != 110 && id != 0) {
		return GetBoneWithRotation(mesh, id);
	}

	BoneCache* cache = lockfree_bone_cache.get_or_create(mesh);

	if (cache->valid) {
		return (id == 110) ? cache->head : cache->bottom;
	}

	Vector3 head = GetBoneWithRotation(mesh, 110);
	Vector3 bottom = GetBoneWithRotation(mesh, 0);

	cache->head = head;
	cache->bottom = bottom;
	cache->last_update = std::chrono::high_resolution_clock::now();
	cache->valid = true;

	return (id == 110) ? head : bottom;
}

template< typename t >
class TArray
{
public:

	TArray() : tData(), iCount(), iMaxCount() {}
	TArray(t* data, int count, int max_count) :
		tData(tData), iCount(iCount), iMaxCount(iMaxCount) {
	}

public:

	auto Get(int idx) -> t
	{
		return read< t >(reinterpret_cast<__int64>(this->tData) + (idx * sizeof(t)));
	}

	auto Size() -> std::uint32_t
	{
		return this->iCount;
	}

	bool IsValid()
	{
		return this->iCount != 0;
	}

	t* tData;
	int iCount;
	int iMaxCount;
};

struct camera_description {
	Vector3 Location;
	Vector3 Rotation;
	float FieldOfView;
	math::matrix4_simd m_world_to_clip{};

	Vector3 GetForwardVector() const {
		float pitch = Rotation.x * (std::numbers::pi / 180.0f);
		float yaw = Rotation.y * (std::numbers::pi / 180.0f);
		Vector3 result(std::sin(yaw) * std::cos(pitch), std::cos(yaw) * std::cos(pitch), std::sin(pitch));
		float len = sqrtf(static_cast<float>(result.x * result.x + result.y * result.y + result.z * result.z));
		if (len == 0.0f) return Vector3(0, 0, 1);
		return Vector3(result.x / len, result.y / len, result.z / len);
	}
};

camera_description camera;
camera_description read_camera;

inline float g_localProjectileSpeed = 0.f;
inline float g_localProjectileGravity = 0.f;
inline bool g_bIsLocalPickaxe = false;

struct EntityRenderData {
	uintptr_t playerStatePtr;
	uintptr_t Player;
	uintptr_t PlayerState;
	uintptr_t CurrentWeapon;
	uintptr_t Mesh;
	bool isBot;
	bool isVisible;
	Vector3 Head3D;
	Vector3 Bottom3D;
	Vector3 TargetVelocity;
	BulkBoneData allBones;
	PlayerInfoBatch playerInfo;
	std::array<Vector3, 5> aimbotBones;

	std::string displayName;
	std::string displayWeapon;
	std::string displayPlatform;
	ImColor platformColor;
	float cameraDistance;
};

struct FrameSnapshot {
	std::vector<EntityRenderData> entities;
	camera_description cam;
	std::vector<uintptr_t> currentPlayerStates;
	int32_t localTeamIndex;
	uintptr_t playerController;
};

constexpr int SNAPSHOT_COUNT = 3;
inline FrameSnapshot g_snapshots[SNAPSHOT_COUNT];
inline std::atomic<int> g_latestSnapshot{-1};
inline std::atomic<int> g_readingSlot{-1};

inline std::atomic<int> g_cameraFrontIdx{0};
inline camera_description g_cameraBuffer[2];

namespace LocalPtrs {
	uintptr_t closest_mesh = 0;
	float closest_distance = FLT_MAX;
}

struct WorldCachedViewInfo {
	math::matrix4 view_matrix;
	math::matrix4 projection_matrix;
	math::matrix4 view_projection_matrix;
	math::matrix4 view_to_world;
};

inline void UpdateCamera(uint64_t gworld) {
	TArray<WorldCachedViewInfo> arr = read<TArray<WorldCachedViewInfo>>(gworld + Offsets::CachedViewInfoRenderedLastFrame);
	if (!arr.IsValid()) return;
	WorldCachedViewInfo info = arr.Get(0);

	read_camera.m_world_to_clip = info.view_projection_matrix.to_simd();

	read_camera.Location.x = info.view_to_world.m[3][0];
	read_camera.Location.y = info.view_to_world.m[3][1];
	read_camera.Location.z = info.view_to_world.m[3][2];

	double pitch = std::asin(info.view_to_world.m[2][2]);
	double yaw = std::atan2(info.view_to_world.m[2][1], info.view_to_world.m[2][0]);
	read_camera.Rotation.x = pitch * math::rad_to_deg;
	read_camera.Rotation.y = yaw * math::rad_to_deg;
	read_camera.Rotation.z = 0.0;

	read_camera.FieldOfView = static_cast<float>(2.0 * std::atan(1.0 / info.projection_matrix.m[0][0]) * math::rad_to_deg);
}

Vector2 ProjectWorldToScreen(Vector3 WorldLocation) {
	if (WorldLocation.x == 0 && WorldLocation.y == 0 && WorldLocation.z == 0) return Vector2(0, 0);
	auto clip = camera.m_world_to_clip.transform(WorldLocation);
	if (clip.w <= 0.0) return Vector2(0, 0);
	double rhw = 1.0 / clip.w;
	return Vector2((clip.x * rhw + 1.0) * 0.5 * Width, (1.0 - clip.y * rhw) * 0.5 * Height);
}

inline bool IsEnemyVisible(uintptr_t mesh)
{

	auto now = std::chrono::steady_clock::now();
	{
		std::lock_guard<std::mutex> lock(g_visibilityCacheMutex);
		auto it = g_visibilityCache.find(mesh);
		if (it != g_visibilityCache.end()) {
			auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastUpdate).count();
			if (age < 50)
				return it->second.visible;
		}
	}

	auto last_render = read<float>(mesh + Offsets::LastRenderTime);
	auto tolerance = read<float>(mesh + Offsets::VisibilityTolerance);
	auto last_submit = read<double>(mesh + Offsets::LastSubmitTime);
	bool visible = fmaxf(0.f, last_render + 0.0001f) >= last_submit - static_cast<double>(tolerance);

	{
		std::lock_guard<std::mutex> lock(g_visibilityCacheMutex);
		g_visibilityCache[mesh] = { visible, now };
	}
	return visible;
}

bool is_dead(uintptr_t Player)
{
	return (read<char>(Player + Offsets::bIsDying) >> 5) & 1;
}

bool is_in_screen_vector2(Vector2 screen_location)
{
	return (screen_location.x >= 0 && screen_location.x <= Width &&
		screen_location.y >= 0 && screen_location.y <= Height);
}

bool is_in_screen_vector3(Vector3 screen_location)
{
	return (screen_location.x >= 0 && screen_location.x <= Width &&
			screen_location.y >= 0 && screen_location.y <= Height);
}

auto GetPlayerName() -> std::string {

	const uintptr_t fstring = LocalPtrs::PlayerState + Offsets::PlayerName;
	const auto data = read<__int64>(fstring + 0x0);
	const auto length = read<int>(fstring + 0x8);

	if (!data || length <= 0 || length > 255)
		return "Bot";

	std::wstring wname(length, L'\0');
	g_driver->Read(
		data,
		wname.data(),
		static_cast<DWORD>(length * sizeof(wchar_t))
	);

	auto count = length - 1;
	int shift = count & 3;
	for (int i = 0; i < count; ++i) {
		shift += 3;
		wname[i] += static_cast<wchar_t>(shift & 7);
	}

	if (auto pos = wname.find(L'\0'); pos != std::wstring::npos)
		wname.erase(pos);

	return WStringToUtf8(wname);
}

inline std::string CachedGetPlayerName(uintptr_t playerStatePtr) {
	auto now = std::chrono::high_resolution_clock::now();
	{
		std::lock_guard<std::mutex> lock(g_nameCacheMutex);
		auto it = g_nameCache.find(playerStatePtr);
		if (it != g_nameCache.end()) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastUpdate).count();
			if (elapsed < 5000) {
				return it->second.name;
			}
		}
	}

	uintptr_t savedPlayerState = LocalPtrs::PlayerState;
	LocalPtrs::PlayerState = playerStatePtr;
	std::string name = GetPlayerName();
	LocalPtrs::PlayerState = savedPlayerState;

	{
		std::lock_guard<std::mutex> lock(g_nameCacheMutex);
		NameCacheEntry& entry = g_nameCache[playerStatePtr];
		entry.name = name;
		entry.lastUpdate = now;
	}

	return name;
}

inline void PurgeStalePlayers(const std::vector<uintptr_t>& currentPlayerStates) {
	{
		std::lock_guard<std::mutex> lock(g_nameCacheMutex);
		std::unordered_set<uintptr_t> currentSet(currentPlayerStates.begin(), currentPlayerStates.end());
		for (auto it = g_nameCache.begin(); it != g_nameCache.end(); ) {
			if (currentSet.find(it->first) == currentSet.end()) {
				it = g_nameCache.erase(it);
			} else {
				++it;
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(g_playerInfoCacheMutex);
		for (auto it = g_playerInfoCache.begin(); it != g_playerInfoCache.end(); ) {
			if (currentPlayerStates.empty() || std::find(currentPlayerStates.begin(), currentPlayerStates.end(), it->first.playerState) == currentPlayerStates.end()) {
				it = g_playerInfoCache.erase(it);
			} else {
				++it;
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(g_platformCacheMutex);
		for (auto it = g_platformCache.begin(); it != g_platformCache.end(); ) {
			if (currentPlayerStates.empty() || std::find(currentPlayerStates.begin(), currentPlayerStates.end(), it->first) == currentPlayerStates.end()) {
				it = g_platformCache.erase(it);
			} else {
				++it;
			}
		}
	}
}

inline PlayerInfoBatch ReadPlayerInfoBatchCached(uintptr_t playerState, uintptr_t player, uintptr_t currentWeapon) {
	auto now = std::chrono::high_resolution_clock::now();
	PlayerInfoCacheKey key{playerState, currentWeapon};

	{
		std::lock_guard<std::mutex> lock(g_playerInfoCacheMutex);
		auto it = g_playerInfoCache.find(key);
		if (it != g_playerInfoCache.end() && it->second.valid) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastUpdate).count();
			if (elapsed < 50) {
				return it->second.info;
			}
		}
	}

	PlayerInfoBatch info;

	info.emoteFlags = read<uint8_t>(player + Offsets::bIsPlayingEmote);

	info.targetFlags = read<uint8_t>(player + Offsets::bIsTargeting);

	info.IsSkydiving = read<uint8_t>(player + Offsets::IsSkydiving) != 0;

	if (currentWeapon) {
		info.IsEquipping = read<uint8_t>(currentWeapon + Offsets::IsEquipping) != 0;
		info.IsCharging = read<uint8_t>(currentWeapon + Offsets::IsCharging) != 0;
		info.ammoCount = read<int32_t>(currentWeapon + Offsets::Ammocount);
	}

	info.bIsDBNO = read<uint8_t>(player + Offsets::bIsDBNO);

	info.teamIndex = read<int32_t>(playerState + Offsets::teamindex);
	info.teamScore = read<int32_t>(playerState + Offsets::KillScore);

	info.seasonRank = -1;
	uintptr_t habanero = read<uintptr_t>(playerState + Offsets::HabaneroComponent);
	if (habanero)
		info.seasonRank = read<int32_t>(habanero + Offsets::HabaneroRank);

	{
		std::lock_guard<std::mutex> lock(g_playerInfoCacheMutex);
		PlayerInfoCacheEntry& entry = g_playerInfoCache[key];
		entry.info = info;
		entry.lastUpdate = now;
		entry.valid = true;
	}

	return info;
}

std::string GetPlatform(ImColor& resultColor) {
	std::string result;

	__int64 FString = read<__int64>(LocalPtrs::PlayerState + Offsets::plattform);
	if (!FString) {
		resultColor = ImColor(255, 255, 255);
		return "Bot";
	}

	wchar_t platform[64] = { 0 };

	g_driver->Read(
		FString,
		platform,
		sizeof(platform)
	);

	std::wstring platform_wstr(platform);
	std::string platform_str(platform_wstr.begin(), platform_wstr.end());

	if (platform_str == "XBL")        { result = "Xbox"; resultColor = ImColor(0.0f, 1.0f, 0.0f, 1.0f); }
	else if (platform_str == "PSN")   { result = "PlayStation"; resultColor = ImColor(0.4f, 0.6f, 1.0f, 1.0f); }
	else if (platform_str == "PS5")   { result = "PlayStation"; resultColor = ImColor(0.4f, 0.6f, 1.0f, 1.0f); }
	else if (platform_str == "XSX")   { result = "Xbox"; resultColor = ImColor(0.0f, 1.0f, 0.0f, 1.0f); }
	else if (platform_str == "SWT")   { result = "Nintendo Switch"; resultColor = ImColor(1.0f, 0.5f, 0.5f, 1.0f); }
	else if (platform_str == "WIN")   { result = "Windows"; resultColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f); }
	else if (platform_str == "MAC")   { result = "Mac"; resultColor = ImColor(0.5f, 0.5f, 0.5f, 1.0f); }
	else if (platform_str == "LNX")   { result = "Linux"; resultColor = ImColor(1.0f, 0.5f, 0.0f, 1.0f); }
	else if (platform_str == "AND")   { result = "Android"; resultColor = ImColor(0.0f, 1.0f, 0.5f, 1.0f); }
	else if (platform_str == "IOS")   { result = "iOS"; resultColor = ImColor(0.5f, 0.5f, 1.0f, 1.0f); }
	else                               { result = "?"; resultColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f); }

	return result;
}

std::string GetWeaponName(uintptr_t pawn) {
	auto curWep = read<uintptr_t>(pawn + Offsets::CurrentWeapon);
	if (!curWep) return "Nothing";

	auto weaponData = read<uintptr_t>(curWep + Offsets::WeaponData);
	if (!weaponData) return "Nothing";

	auto textDataPtr = read<uintptr_t>(weaponData + Offsets::Itemname);
	if (!textDataPtr) return "Nothing";

	auto fstringData = read<uintptr_t>(textDataPtr + 0x20);
	if (!fstringData) return "Nothing";

	int length = read<int>(textDataPtr + 0x28);
	if (!length || length > 100) return "Nothing";

	std::vector<wchar_t> buffer(static_cast<size_t>(length));
	g_driver->Read(
		fstringData,
		buffer.data(),
		static_cast<DWORD>(length * sizeof(wchar_t))
	);

	std::wstring wstr(buffer.begin(), buffer.end());
	std::string result(wstr.begin(), wstr.end());
	return result.empty() ? "Nothing" : result;
}

std::string GetPlatformCached(ImColor& resultColor, uintptr_t playerState) {
	auto now = std::chrono::high_resolution_clock::now();

	{
		std::lock_guard<std::mutex> lock(g_platformCacheMutex);
		auto it = g_platformCache.find(playerState);
		if (it != g_platformCache.end() && it->second.valid) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastUpdate).count();
			if (elapsed < 5000) {
				resultColor = it->second.color;
				return it->second.platform;
			}
		}
	}

	uintptr_t savedPlayerState = LocalPtrs::PlayerState;
	LocalPtrs::PlayerState = playerState;
	std::string result = GetPlatform(resultColor);
	LocalPtrs::PlayerState = savedPlayerState;

	{
		std::lock_guard<std::mutex> lock(g_platformCacheMutex);
		PlatformCacheEntry& entry = g_platformCache[playerState];
		entry.platform = result;
		entry.color = resultColor;
		entry.lastUpdate = now;
		entry.valid = true;
	}

	return result;
}

std::string GetWeaponNameCached(uintptr_t pawn) {
	auto now = std::chrono::high_resolution_clock::now();
	uintptr_t curWep = read<uintptr_t>(pawn + Offsets::CurrentWeapon);

	{
		std::lock_guard<std::mutex> lock(g_weaponNameCacheMutex);
		auto it = g_weaponNameCache.find(curWep);
		if (it != g_weaponNameCache.end() && it->second.valid) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastUpdate).count();
			if (elapsed < 500) {
				return it->second.name;
			}
		}
	}

	std::string result = GetWeaponName(pawn);

	{
		std::lock_guard<std::mutex> lock(g_weaponNameCacheMutex);
		WeaponNameCacheEntry& entry = g_weaponNameCache[curWep];
		entry.name = result;
		entry.lastUpdate = now;
		entry.valid = true;
	}

	return result;
}

class StringBufferPool {
private:
    static constexpr size_t BUFFER_SIZE = 256;
    static constexpr size_t POOL_SIZE = 50;

    std::array<char[BUFFER_SIZE], POOL_SIZE> buffers;
    std::array<bool, POOL_SIZE> in_use;
    std::mutex pool_mutex;

public:
    StringBufferPool() {
        std::fill(in_use.begin(), in_use.end(), false);
    }

    char* acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            if (!in_use[i]) {
                in_use[i] = true;
                return buffers[i];
            }
        }
        static char fallback_buffer[BUFFER_SIZE];
        return fallback_buffer;
    }

    void release(char* buffer) {
        if (!buffer) return;

        std::lock_guard<std::mutex> lock(pool_mutex);
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            if (buffers[i] == buffer) {
                in_use[i] = false;
                break;
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        std::fill(in_use.begin(), in_use.end(), false);
    }
};

static StringBufferPool string_buffer_pool;

inline char* GetStringBuffer() {
    return string_buffer_pool.acquire();
}

inline void ReleaseStringBuffer(char* buffer) {
    string_buffer_pool.release(buffer);
}

#define SPRINTF_BUFFER(buffer, format, ...) \
    char* buffer = GetStringBuffer(); \
    snprintf(buffer, 256, format, __VA_ARGS__)

#define RELEASE_BUFFER(buffer) \
    ReleaseStringBuffer(buffer)

bool inline force_cache_refresh = false;

enum EBoneIndex : int {
	Head = 110,
	Neck = 67,
	Chest = 7,
	Pelvis = 2,
	LShoulder = 9,
	LElbow = 10,
	LHand = 11,
	RShoulder = 38,
	RElbow = 39,
	RHand = 40,
	LHip = 71,
	LKnee = 72,
	LFoot = 75,
	RHip = 78,
	RKnee = 79,
	RFoot = 82,
	Root = 0,
	Test = 83,
	Test2 = 76
};

constexpr std::array<std::pair<EBoneIndex, EBoneIndex>, 15> eBonePairs =
{
	{ {EBoneIndex::Neck, EBoneIndex::Pelvis },
	{ EBoneIndex::LShoulder, EBoneIndex::LElbow },
	{ EBoneIndex::LElbow, EBoneIndex::LHand },
	{ EBoneIndex::RShoulder, EBoneIndex::RElbow },
	{ EBoneIndex::RElbow, EBoneIndex::RHand },
	{ EBoneIndex::Pelvis, EBoneIndex::LHip },
	{ EBoneIndex::Pelvis, EBoneIndex::RHip },
	{ EBoneIndex::LHip, EBoneIndex::LKnee },
	{ EBoneIndex::LKnee, EBoneIndex::LFoot },
	{ EBoneIndex::RHip, EBoneIndex::RKnee },
	{ EBoneIndex::RKnee, EBoneIndex::RFoot },
	{ EBoneIndex::RFoot, EBoneIndex::Test },
	{ EBoneIndex::LFoot, EBoneIndex::Test2 },
	{ EBoneIndex::LShoulder, EBoneIndex::RShoulder } }
};
