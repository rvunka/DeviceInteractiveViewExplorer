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

	const FVector ViewFwd = ViewRotation.RotateVector(FVector::ForwardVector);
	const FVector ViewRight = ViewRotation.RotateVector(FVector::RightVector);
	const FVector ViewUp = ViewRotation.RotateVector(FVector::UpVector);
	const FVector MotionWS = ViewRight * ScreenDelta.X + ViewUp * (-ScreenDelta.Y);

	const FVector AxisOnView = (Axis - ViewFwd * FVector::DotProduct(Axis, ViewFwd));
	if (AxisOnView.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		// Looking along the rail: MotionWS is perpendicular to Axis, so the projection
		// is ~0. Treat horizontal mouse as travel — same face-on convention as rotate.
		return ScreenDelta.X * CmPerPixel;
	}

	return FVector::DotProduct(MotionWS, Axis) * CmPerPixel;
}

void BuildAxisPlaneBasis(const FVector AxisWorld, FVector& OutBasisU, FVector& OutBasisV)
{
	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		OutBasisU = FVector::RightVector;
		OutBasisV = FVector::ForwardVector;
		return;
	}

	const FVector Reference = FMath::Abs(Axis.Z) < 0.9f ? FVector::UpVector : FVector::RightVector;
	OutBasisU = FVector::CrossProduct(Reference, Axis).GetSafeNormal();
	OutBasisV = FVector::CrossProduct(Axis, OutBasisU).GetSafeNormal();
}

FVector ProjectPointOntoAxis(const FVector WorldPoint, const FVector AxisOrigin, const FVector AxisWorld)
{
	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return AxisOrigin;
	}

	return AxisOrigin + Axis * FVector::DotProduct(WorldPoint - AxisOrigin, Axis);
}

float PlaneAngleDegrees(const FVector& PlaneVector, const FVector& BasisU, const FVector& BasisV)
{
	return FMath::RadiansToDegrees(
		FMath::Atan2(FVector::DotProduct(PlaneVector, BasisV), FVector::DotProduct(PlaneVector, BasisU)));
}

FDIVEPointerAxisAngleResult MapWorldPointToAxisAngle(
	const FVector AxisOrigin,
	const FVector AxisWorld,
	const FVector PlaneBasisU,
	const FVector PlaneBasisV,
	const FVector WorldPoint,
	const FVector PreviousPlaneVector,
	const float DeadRadiusCm)
{
	FDIVEPointerAxisAngleResult Result;

	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return Result;
	}

	Result.PlaneVector = FVector::VectorPlaneProject(WorldPoint - AxisOrigin, Axis);

	const float DeadRadiusSq = FMath::Square(FMath::Max(DeadRadiusCm, 0.f));
	if (Result.PlaneVector.SizeSquared() < DeadRadiusSq)
	{
		return Result;
	}

	if (PreviousPlaneVector.IsNearlyZero() || PreviousPlaneVector.SizeSquared() < DeadRadiusSq)
	{
		// Seed frame: expose the radius vector but do not apply a delta yet.
		return Result;
	}

	const float PrevAngle = PlaneAngleDegrees(PreviousPlaneVector, PlaneBasisU, PlaneBasisV);
	const float CurAngle = PlaneAngleDegrees(Result.PlaneVector, PlaneBasisU, PlaneBasisV);

	Result.DeltaDegrees = FMath::FindDeltaAngleDegrees(PrevAngle, CurAngle);
	Result.bApplied = true;
	return Result;
}

FDIVEPointerAxisAngleResult MapPointerToAxisAngle(
	const FVector AxisOrigin,
	const FVector AxisWorld,
	const FVector PlaneBasisU,
	const FVector PlaneBasisV,
	const FVector RayOrigin,
	const FVector RayDir,
	const FVector PreviousPlaneVector,
	const float DeadRadiusCm)
{
	FDIVEPointerAxisAngleResult Result;

	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return Result;
	}

	const FVector Dir = RayDir.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return Result;
	}

	const float Denom = FVector::DotProduct(Dir, Axis);
	if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER)
	{
		// Ray nearly parallel to the plane (grazing / edge-on).
		return Result;
	}

	const float T = FVector::DotProduct(AxisOrigin - RayOrigin, Axis) / Denom;
	if (T < 0.f)
	{
		return Result;
	}

	const FVector HitPoint = RayOrigin + Dir * T;
	return MapWorldPointToAxisAngle(
		AxisOrigin,
		Axis,
		PlaneBasisU,
		PlaneBasisV,
		HitPoint,
		PreviousPlaneVector,
		DeadRadiusCm);
}

FDIVEPointerAxisTravelResult MapWorldPointToAxisTravel(
	const FVector AxisOrigin,
	const FVector AxisWorld,
	const FVector WorldPoint,
	const bool bHasPrevious,
	const float PreviousParameterCm)
{
	FDIVEPointerAxisTravelResult Result;

	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return Result;
	}

	Result.ParameterCm = FVector::DotProduct(WorldPoint - AxisOrigin, Axis);
	Result.bHasParameter = true;
	if (!bHasPrevious)
	{
		// Seed frame: expose the parameter but do not apply a delta yet.
		return Result;
	}

	Result.DeltaCm = Result.ParameterCm - PreviousParameterCm;
	Result.bApplied = true;
	return Result;
}

FDIVEPointerAxisTravelResult MapPointerToAxisTravel(
	const FVector AxisOrigin,
	const FVector AxisWorld,
	const FVector RayOrigin,
	const FVector RayDir,
	const bool bHasPrevious,
	const float PreviousParameterCm)
{
	FDIVEPointerAxisTravelResult Result;

	const FVector Axis = AxisWorld.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return Result;
	}

	const FVector Dir = RayDir.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return Result;
	}

	const FVector W0 = RayOrigin - AxisOrigin;
	const float A = FVector::DotProduct(Dir, Dir);
	const float B = FVector::DotProduct(Dir, Axis);
	const float C = FVector::DotProduct(Axis, Axis);
	const float D = FVector::DotProduct(Dir, W0);
	const float E = FVector::DotProduct(Axis, W0);
	const float Denom = A * C - B * B;
	if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER)
	{
		// Ray nearly parallel to the axis (looking along the rail).
		return Result;
	}

	const float TRay = (B * E - C * D) / Denom;
	if (TRay < 0.f)
	{
		return Result;
	}

	const FVector ClosestOnAxis = AxisOrigin + Axis * ((A * E - B * D) / Denom);
	return MapWorldPointToAxisTravel(
		AxisOrigin,
		Axis,
		ClosestOnAxis,
		bHasPrevious,
		PreviousParameterCm);
}
}
