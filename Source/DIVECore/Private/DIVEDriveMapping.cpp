// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDriveMapping.h"

namespace DIVE
{
float MapScreenDeltaToAxisAngle(
	const FVector2D ScreenDelta,
	const FRotator ViewRotation,
	const FVector AxisWorld,
	const float DegreesPerPixel)
{
	if (ScreenDelta.IsNearlyZero() || FMath::IsNearlyZero(DegreesPerPixel))
	{
		return 0.f;
	}

	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return 0.f;
	}

	const FVector ViewFwd = ViewRotation.RotateVector(FVector::ForwardVector);
	const FVector ViewRight = ViewRotation.RotateVector(FVector::RightVector);
	const FVector ViewUp = ViewRotation.RotateVector(FVector::UpVector);
	const FVector MotionWS = ViewRight * ScreenDelta.X + ViewUp * (-ScreenDelta.Y);

	const FVector AxisOnView = (Axis - ViewFwd * FVector::DotProduct(Axis, ViewFwd));
	if (AxisOnView.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		// Face-on: treat horizontal mouse as twist around the view/axis.
		return ScreenDelta.X * DegreesPerPixel;
	}

	const FVector PerpOnView = FVector::CrossProduct(ViewFwd, AxisOnView.GetSafeNormal()).GetSafeNormal();
	if (PerpOnView.IsNearlyZero())
	{
		return ScreenDelta.X * DegreesPerPixel;
	}

	return FVector::DotProduct(MotionWS, PerpOnView) * DegreesPerPixel;
}

float MapScreenDeltaToAxisTravel(
	const FVector2D ScreenDelta,
	const FRotator ViewRotation,
	const FVector AxisWorld,
	const float CmPerPixel)
{
	if (ScreenDelta.IsNearlyZero() || FMath::IsNearlyZero(CmPerPixel))
	{
		return 0.f;
	}

	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return 0.f;
	}

	const FVector ViewRight = ViewRotation.RotateVector(FVector::RightVector);
	const FVector ViewUp = ViewRotation.RotateVector(FVector::UpVector);
	const FVector MotionWS = ViewRight * ScreenDelta.X + ViewUp * (-ScreenDelta.Y);
	return FVector::DotProduct(MotionWS, Axis) * CmPerPixel;
}
}
