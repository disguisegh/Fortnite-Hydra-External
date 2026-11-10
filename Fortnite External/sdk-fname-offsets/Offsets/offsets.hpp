#pragma once
#include <cstdint>

namespace Offsets {

	uint64_t
		UWorld = 0x1B2439A0,
		GEngine = 0x1B245388,
		GObjects = 0x1B1D3830,
		GObjectsCount = 0x1B1D3848,
		GNames = 0x1B0F6E80;

	uint64_t
		GameState = 0x1D8,
		OwningGameInstance = 0x250,
		Levels = 0x1F0,
		PersistentLevel = 0x38,
		OwningWorld = 0x70,

		Actors = 0x28,
		ActorCluster = 0x90,
		CachedViewInfoRenderedLastFrame = 0x188,
		TimeSeconds = 0x190,
		NetDriver = 0x40,
		ServerWorldTime = 0x2E0;

	uint64_t
		PlayerCameraManager = 0x328,
		AcknowledgedPawn = 0x318,
		bIsLocalPlayerController = 0x64C,

		ControlRotation = 0x2E8,

		RotationInput = 0x518,

		TargetSourceOffset = 0x1128,
		TargetSourceOffsetWhileCrouched = 0x1140,
		TargetSourceOffsetWhileTargeting = 0x1158,
		bTraceThroughWorld = 0x1109,
		LocationUnderReticle = 0x20A0,
		NetConnection = 0x4A8,
		GameViewport = 0xB70,
		FOV = 0x284,
		TargetedFortPawn = 0x1650,
		CameraLocation = 0x180,
		CameraCachePrivate = 0x1590,
		LastFrameCameraCachePrivate = 0x1EB0,
		ViewTarget = 0x300;

	uint64_t
		CameraCache_Timestamp = 0x0,
		CameraCache_POV = 0x10,
		POV_Location = 0x0,
		POV_Rotation = 0x18,
		POV_FOV = 0x30;

	uint64_t
		LocalPlayers = 0x38,
		PlayerController = 0x30,
		PlayerArray = 0x288;

	uint64_t
		GameInstance = 0x1100;

	uint64_t
		RootComponent = 0x1B0,
		relative_location = 0x140,
		RelativeRotation = 0x158,
		bComponentToWorldUpdated = 0x1A0,
		ComponentVelocity = 0x188;

	uint64_t
		BoneArray = 0x650,
		BoneCache = 0x660,
		ComponentToWorld = 0x1E0,
		CameraRotation = 0x188,
		ViewState = 0xD0,
		LastRenderTime = 0x7DC,
		LastSubmitTime = 0x7B8,
		VisibilityTolerance = 0x338,
		CachedBoneSpaceTransforms = 0x9A0,
		CachedComponentSpaceTransforms = 0x9B0,
		VisibilityBasedAnimTickOption = 0x7CC;

	uint64_t
		PlayerNamePrivate = 0x308,
		bIsABot = 0x27A;

	uint64_t
		Mesh = 0x2F0,
		CurrentWeapon = 0x998,
		CharacterMovement = 0x2F8,
		bIsCrouched = 0x430,
		bIsDying = 0x720,
		bIsDBNO = 0x849,
		LastUpdateLocation = 0x360,
		OverlappingBuildings = 0x1C08,
		CurrentVehicle = 0x2B00,
		LastDamagedTime = 0xD50,
		Spectators = 0xA90;

	uint64_t
		bIsTargeting = 0x1F7C,
		bIsPlayingEmote = 0x218B,
		IsSkydiving = 0x2187;

	uint64_t
		WeaponData = 0x618,
		Ammocount = 0x11B4,
		bIsReloadingWeapon = 0x381,
		IsEquipping = 0x368,
		IsCharging = 0x380,

		ProjectileSpeed = 0x0,
		ProjectileGravityScale = 0x0,
		LastFireTime = 0x10EC,
		LastFireTimeVerified = 0x10F4;

	uint64_t
		PawnPrivate = 0x2E8,
		PlayerState = 0x290,
		plattform = 0x400,
		PlayerName = 0x308,
		PlayerAimOffset = 0x2220,
		AdditionalAimOffset = 0x21F0,
		WeaponRecoilOffset = 0x2238,
		WeaponOffsetCorrection = 0x2250,
		HabaneroComponent = 0x918;

	uint64_t
		teamindex = 0xF31,
		KillScore = 0xF48,
		SeasonLevelUIDisplay = 0xF4C,
		TeamKillScore = 0xF0C,
		HumanKillScore = 0xF54,
		RebootCount = 0x15CC;

	uint64_t
		RankedProgress = 0xD8,
		HabaneroRank = 0xE8;

	uint64_t
		InputPitchScale = 0x4CC,
		InputYawScale = 0x4C8;

	uint64_t
		ItemName = 0x38,
		Itemname = 0x38;

	uint64_t
		NAME_PRIVATE = 0x18,
		CachedWorldBounds = 0x850;

	uint64_t
		bAlwaysRelevant = 0x58;

	uint64_t
		StaticMesh = 0x6F8,
		StaticMeshComponent = 0x7A8;

	uint64_t
		BodySetup = 0x168;

	uint64_t
		StaticMeshComponent_Mesh = 0x5D8;
};
