// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct DIVECORE_API FDIVEPointerAxisAngleResult
{
	bool bApplied = false;
	float DeltaDegrees = 0.f;
	/** Vector from AxisOrigin to the pointer hit on the plane perpendicular to the axis. */
	FVector PlaneVector = FVector::ZeroVector;
};

struct DIVECORE_API FDIVEPointerAxisTravelResult
{
	bool bApplied = false;
	/** True when ParameterCm is valid (including a seed frame with DeltaCm = 0). */
	bool bHasParameter = false;
	float DeltaCm = 0.f;
	/** Signed centimetres from AxisOrigin along the normalised axis. */
	float ParameterCm = 0.f;
};

namespace DIVE
{
/**
 * Degrees around AxisWorld produced by a 2D mouse drag, using the camera as the viewing basis.
 * Face-on axes (nearly parallel to the view) fall back to horizontal screen motion.
 * Rotary/threaded use MapPointerToAxisAngle first; this remains the edge-on / dead-zone fallback.
 */
DIVECORE_API float MapScreenDeltaToAxisAngle(
	FVector2D ScreenDelta,
	FRotator ViewRotation,
	FVector AxisWorld,
	float DegreesPerPixel);

/** Centimetres along AxisWorld produced by a 2D mouse drag projected onto the viewing plane.
 *  Linear drive uses MapPointerToAxisTravel first; this remains the edge-on / unstable-projection fallback.
 *  Looking along the rail (axis nearly parallel to the view) falls back to horizontal pixels. */
DIVECORE_API float MapScreenDeltaToAxisTravel(
	FVector2D ScreenDelta,
	FRotator ViewRotation,
	FVector AxisWorld,
	float CmPerPixel);

/** Build an orthonormal basis (U, V) spanning the plane perpendicular to AxisWorld. */
DIVECORE_API void BuildAxisPlaneBasis(FVector AxisWorld, FVector& OutBasisU, FVector& OutBasisV);

/** Project WorldPoint onto the infinite line (AxisOrigin, AxisWorld). */
DIVECORE_API FVector ProjectPointOntoAxis(FVector WorldPoint, FVector AxisOrigin, FVector AxisWorld);

/**
 * Map a world point onto polar angle around AxisWorld in the plane through AxisOrigin.
 * bApplied is false when the point is inside DeadRadiusCm, or PreviousPlaneVector is unset
 * (caller should seed from PlaneVector without applying delta).
 */
DIVECORE_API FDIVEPointerAxisAngleResult MapWorldPointToAxisAngle(
	FVector AxisOrigin,
	FVector AxisWorld,
	FVector PlaneBasisU,
	FVector PlaneBasisV,
	FVector WorldPoint,
	FVector PreviousPlaneVector,
	float DeadRadiusCm);

/**
 * Map a view ray onto polar angle around AxisWorld in the plane through AxisOrigin.
 * Intersects RayOrigin + RayDir with the plane, then MapWorldPointToAxisAngle.
 * bApplied is also false when the ray grazes the plane or the hit is behind the ray origin.
 */
DIVECORE_API FDIVEPointerAxisAngleResult MapPointerToAxisAngle(
	FVector AxisOrigin,
	FVector AxisWorld,
	FVector PlaneBasisU,
	FVector PlaneBasisV,
	FVector RayOrigin,
	FVector RayDir,
	FVector PreviousPlaneVector,
	float DeadRadiusCm);

/**
 * Map a world point onto travel along AxisWorld through AxisOrigin.
 * ParameterCm is always written when the axis is valid. bApplied is false when seeding
 * (!bHasPrevious); otherwise DeltaCm = ParameterCm - PreviousParameterCm.
 */
DIVECORE_API FDIVEPointerAxisTravelResult MapWorldPointToAxisTravel(
	FVector AxisOrigin,
	FVector AxisWorld,
	FVector WorldPoint,
	bool bHasPrevious,
	float PreviousParameterCm);

/**
 * Map a view ray onto travel along AxisWorld through AxisOrigin.
 * Closest point between ray and axis, then MapWorldPointToAxisTravel.
 * bApplied is also false when the ray is parallel to the axis or the closest ray sample is behind the origin.
 */
DIVECORE_API FDIVEPointerAxisTravelResult MapPointerToAxisTravel(
	FVector AxisOrigin,
	FVector AxisWorld,
	FVector RayOrigin,
	FVector RayDir,
	bool bHasPrevious,
	float PreviousParameterCm);
}
