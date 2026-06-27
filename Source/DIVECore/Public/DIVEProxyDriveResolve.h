// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UPrimitiveComponent;

class DIVECORE_API IDIVEProxyDrive;

namespace DIVEProxyDriveResolve
{
DIVECORE_API IDIVEProxyDrive* FindProxyDriveForHit(UPrimitiveComponent* HitComponent);
}
