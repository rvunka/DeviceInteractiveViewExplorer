// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDIVESessionSubsystem;
struct FDIVEFocusTarget;
struct FHitResult;

struct FDIVESessionPickOps
{
	static bool ResolvePickAtScreenPositionWithHit(
		const UDIVESessionSubsystem& Session,
		const FVector2D& ScreenPosition,
		class APlayerController* PlayerController,
		FDIVEFocusTarget& OutPickTarget,
		FHitResult& OutHit);

	static bool ExecutePrimaryActionAtScreenPosition(
		UDIVESessionSubsystem& Session,
		const FVector2D& ScreenPosition,
		class APlayerController* PlayerController);

	static void UpdatePickHover(
		UDIVESessionSubsystem& Session,
		const FVector2D& ScreenPosition,
		class APlayerController* PlayerController);

	static void ClearPickHover(UDIVESessionSubsystem& Session);
};
