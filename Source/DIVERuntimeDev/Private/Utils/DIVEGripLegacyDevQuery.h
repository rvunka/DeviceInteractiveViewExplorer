// Copyright (c) 2026. All Rights Reserved.

#pragma once

class AActor;

namespace DIVEGripLegacyDevQuery
{
	/** While grabbing, forward wheel to DIVE pawn physical drive. Returns true when handled. */
	bool TryForwardMouseWheelToGrip(const AActor* Owner, float WheelDelta);
}
