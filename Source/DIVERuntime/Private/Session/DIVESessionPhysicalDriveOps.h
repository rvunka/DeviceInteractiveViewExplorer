// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDIVESessionSubsystem;
struct FDIVEFocusTarget;
struct FHitResult;

struct FDIVESessionPhysicalDriveOps
{
	static void EndActivePhysicalDrive(UDIVESessionSubsystem& Session, bool bCommit);
	static void ResetPhysicalDriveState(UDIVESessionSubsystem& Session);
	static void ClearProxyDrive(UDIVESessionSubsystem& Session);
	static bool TryBeginProxyDriveAtScreenPosition(
		UDIVESessionSubsystem& Session,
		const FVector2D& ScreenPosition,
		class APlayerController* PlayerController);
	static void UpdateActiveInteraction(UDIVESessionSubsystem& Session, const FVector2D& ScreenDelta);
	static void EndProxyDrive(UDIVESessionSubsystem& Session, bool bCommit);
	static void HandleActivePawnPhysicalManualRotatePressed(UDIVESessionSubsystem& Session);
	static void HandleActivePawnPhysicalManualRotateReleased(UDIVESessionSubsystem& Session);
};
