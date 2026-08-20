// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace DIVE
{
/** Action name for ACTS → DIVE session. Host: OnActionExecuted → TryRequestSessionFromActionId. */
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
inline const FName kBindingBuiltInStandard = TEXT("BuiltIn.Standard");
inline const FName kBindingBuiltInAdmin = TEXT("BuiltIn.Admin");

DIVECORE_API FString NormalizeComponentToken(FString Token);
}
