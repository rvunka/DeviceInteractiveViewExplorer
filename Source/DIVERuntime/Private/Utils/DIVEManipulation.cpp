// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEManipulation.h"

#include "DIVEAnchorComponent.h"

namespace DIVEManipulation
{
float SnapAngle(float AngleDegrees, const TArray<float>& SnapAngles)
{
	if (SnapAngles.IsEmpty())
	{
		return AngleDegrees;
	}

	float BestAngle = SnapAngles[0];
	float BestDistance = FMath::Abs(AngleDegrees - BestAngle);
	for (int32 Index = 1; Index < SnapAngles.Num(); ++Index)
	{
		const float Distance = FMath::Abs(AngleDegrees - SnapAngles[Index]);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestAngle = SnapAngles[Index];
		}
	}

	return BestAngle;
}

void ApplyHingeDelta(UDIVEAnchorComponent* Anchor, float DeltaDegrees)
{
	if (!Anchor || !Anchor->SupportsManipulation())
	{
		return;
	}

	Anchor->SetHingeAngleDegrees(Anchor->GetHingeAngleDegrees() + DeltaDegrees);
}

void CommitHingeSnap(UDIVEAnchorComponent* Anchor)
{
	if (!Anchor || !Anchor->SupportsManipulation())
	{
		return;
	}

	const float SnappedAngle = SnapAngle(Anchor->GetHingeAngleDegrees(), Anchor->HingeSnapAngles);
	Anchor->SetHingeAngleDegrees(SnappedAngle);
}
}
