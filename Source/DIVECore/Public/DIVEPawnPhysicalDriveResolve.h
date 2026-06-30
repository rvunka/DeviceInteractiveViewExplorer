// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class APawn;
class APlayerController;

class DIVECORE_API IDIVEPawnPhysicalDrive;

namespace DIVEPawnPhysicalDriveResolve
{
DIVECORE_API IDIVEPawnPhysicalDrive* FindOnPawn(APawn* Pawn);
DIVECORE_API IDIVEPawnPhysicalDrive* FindOnPlayerController(APlayerController* PlayerController);
}
