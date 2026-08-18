// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class APawn;
class APlayerController;

class DIVECORE_API IDIVEPawnPhysicalDrive;

namespace DIVEPawnPhysicalDriveResolve
{
/**
 * Resolve a pawn Physical drive backend.
 * Named lookup is exact. Empty name: unique implementor only — N>1 returns nullptr (no silent first).
 */
DIVECORE_API IDIVEPawnPhysicalDrive* FindOnPawn(APawn* Pawn, FName ComponentName = NAME_None);
DIVECORE_API IDIVEPawnPhysicalDrive* FindOnPlayerController(
	APlayerController* PlayerController,
	FName ComponentName = NAME_None);
}
