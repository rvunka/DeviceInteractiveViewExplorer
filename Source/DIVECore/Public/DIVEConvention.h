// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace DIVE
{
inline const FName kActionOpenDIVE = TEXT("OpenDIVE");
inline constexpr float kDefaultOrbitDistance = 200.f;

inline const FName kContextFocus = TEXT("DIVE.Context.Focus");
inline const FName kContextIsolate = TEXT("DIVE.Context.Isolate");
inline const FName kContextToggleMeshPhysics = TEXT("DIVE.Context.ToggleMeshPhysics");
inline const FName kContextDeleteMesh = TEXT("DIVE.Context.DeleteMesh");

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
}
