// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDIVEAnchorComponent;

namespace DIVEManipulation
{
DIVERUNTIME_API float SnapAngle(float AngleDegrees, const TArray<float>& SnapAngles);

DIVERUNTIME_API void ApplyHingeDelta(UDIVEAnchorComponent* Anchor, float DeltaDegrees);

DIVERUNTIME_API void CommitHingeSnap(UDIVEAnchorComponent* Anchor);
}
