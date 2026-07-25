// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace DIVE
{
inline const FName kActionOpenDIVE = TEXT("OpenDIVE");
inline constexpr float kDefaultOrbitDistance = 200.f;
inline constexpr float kDefaultMinOrbitDistance = 20.f;
inline constexpr float kDefaultMaxOrbitDistance = 2000.f;
/** At this orbit distance (cm), ZoomSensitivity maps 1:1 to a zoom step. */
inline constexpr float kDefaultZoomDistanceReference = 200.f;
/** Orbit distance ≈ SphereRadius × this when focusing a primitive. */
inline constexpr float kDefaultFocusOrbitFitMultiplier = 2.75f;
/** Min orbit distance floor ≈ SphereRadius × this while a primitive is focused. */
inline constexpr float kDefaultFocusNearPaddingFactor = 1.2f;
/** Tag for hidden mesh proxies that should still receive DIVE pick (shapes need no tag). */
inline const FName kPickProxyTag = TEXT("DIVE.PickProxy");

inline const FName kContextFocus = TEXT("DIVE.Context.Focus");
inline const FName kContextIsolate = TEXT("DIVE.Context.Isolate");
inline const FName kContextToggleMeshPhysics = TEXT("DIVE.Context.ToggleMeshPhysics");
inline const FName kContextDeleteMesh = TEXT("DIVE.Context.DeleteMesh");

inline bool IsReservedContextMenuActionId(const FName ActionId)
{
	return ActionId == kContextFocus
		|| ActionId == kContextIsolate
		|| ActionId == kContextToggleMeshPhysics
		|| ActionId == kContextDeleteMesh;
}

inline FName MakeQualifiedPickContextMenuActionId(const FName ComponentName, const FName LocalActionId)
{
	if (ComponentName.IsNone() || LocalActionId.IsNone())
	{
		return NAME_None;
	}

	return FName(*FString::Printf(TEXT("%s_%s"), *ComponentName.ToString(), *LocalActionId.ToString()));
}

/** Blueprint function name convention: Handle_{ComponentName}_{ActionId} (e.g. Handle_Screw1_Unscrew). */
inline FName MakePickContextMenuHandlerName(const FName ComponentName, const FName LocalActionId)
{
	if (ComponentName.IsNone() || LocalActionId.IsNone())
	{
		return NAME_None;
	}

	return FName(*FString::Printf(TEXT("Handle_%s_%s"), *ComponentName.ToString(), *LocalActionId.ToString()));
}

/** Optional toggle query: Is_{ComponentName}_{ActionId} returns whether to append "*" to the menu label. */
inline FName MakePickContextMenuActiveStateName(const FName ComponentName, const FName LocalActionId)
{
	if (ComponentName.IsNone() || LocalActionId.IsNone())
	{
		return NAME_None;
	}

	return FName(*FString::Printf(TEXT("Is_%s_%s"), *ComponentName.ToString(), *LocalActionId.ToString()));
}

/** Strip SCS "_GEN_VARIABLE" and Duplicate suffixes like "_1", "_12" for catalog keys. */
DIVECORE_API FString NormalizeComponentToken(FString Token);
}
