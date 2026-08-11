// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace DIVE
{
/** Action name for the "open DIVE session" input event. Consumed by the game layer (ACTS); not handled inside DIVE itself. */
inline const FName kActionOpenDIVE = TEXT("OpenDIVE");
inline constexpr float kDefaultOrbitDistance = 200.f;
inline constexpr float kDefaultMinOrbitDistance = 20.f;
inline constexpr float kDefaultMaxOrbitDistance = 2000.f;
inline constexpr float kDefaultZoomDistanceReference = 200.f;
inline constexpr float kDefaultFocusOrbitFitMultiplier = 2.75f;
inline constexpr float kDefaultFocusNearPaddingFactor = 1.2f;
inline const FName kPickProxyTag = TEXT("DIVE.PickProxy");
inline const FName kSectionStandard = TEXT("Standard");
inline const FName kSectionAdmin = TEXT("Admin");

DIVECORE_API FString NormalizeComponentToken(FString Token);
}
