// Copyright (c) 2026. All Rights Reserved.

#pragma once

class AActor;

namespace DIVEGripLegacyDevQuery
{
	/** True when GRIP owns mouse wheel during grab (Legacy Dev: defer DIVE orbit zoom). */
	bool ShouldDeferMouseWheelToGrip(const AActor* Owner);

	/** While grabbing, forward wheel to DIVE bridge or GRIP Input. Returns true when handled. */
	bool TryForwardMouseWheelToGrip(const AActor* Owner, float WheelDelta);
}
