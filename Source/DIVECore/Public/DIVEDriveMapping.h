// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace DIVE
{
/**
 * Degrees around AxisWorld produced by a 2D mouse drag, using the camera as the viewing basis.
 * Face-on axes (nearly parallel to the view) fall back to horizontal screen motion.
 */
DIVECORE_API float MapScreenDeltaToAxisAngle(
	FVector2D ScreenDelta,
	FRotator ViewRotation,
	FVector AxisWorld,
	float DegreesPerPixel);

/** Centimetres along AxisWorld produced by a 2D mouse drag projected onto the viewing plane.
 *  Production rotary/threaded actions use MapScreenDeltaToAxisAngle; this is the linear counterpart
 *  for sliders / host IDIVEProxyDrive implementors. */
DIVECORE_API float MapScreenDeltaToAxisTravel(
	FVector2D ScreenDelta,
	FRotator ViewRotation,
	FVector AxisWorld,
	float CmPerPixel);
}
