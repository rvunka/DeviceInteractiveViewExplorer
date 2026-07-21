// Copyright (c) 2026. All Rights Reserved.

#pragma once

class UDIVESessionSubsystem;

struct FDIVESessionFocusOps
{
	static bool ApplyFocusTarget(
		UDIVESessionSubsystem& Session,
		const struct FDIVEFocusTarget& Target,
		bool bPushToStack,
		bool bBlendCamera,
		bool bResetOrbitDistance);

	static bool ApplyInitialSessionFocus(UDIVESessionSubsystem& Session, FName InitialFocusId);
};
