// Copyright (c) 2026. All Rights Reserved.

#pragma once

class UDIVESessionSubsystem;
struct FDIVEFocusTarget;

struct FDIVESessionIsolationOps
{
	static bool ApplyIsolationForTarget(UDIVESessionSubsystem& Session, const FDIVEFocusTarget& Target);
	static void CollectIsolationVisiblePrimitives(
		const UDIVESessionSubsystem& Session,
		const FDIVEFocusTarget& Target,
		TArray<class UPrimitiveComponent*>& OutVisible);
	static void ClearIsolation(UDIVESessionSubsystem& Session);
};
