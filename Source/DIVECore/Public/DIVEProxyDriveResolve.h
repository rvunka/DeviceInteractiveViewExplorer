// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UPrimitiveComponent;

class DIVECORE_API IDIVEProxyDrive;

namespace DIVEProxyDriveResolve
{
/** Walk attach-root + ForEachDeviceActor. N>1 registry or N>1 owner drives → nullptr (no silent first). */
DIVECORE_API IDIVEProxyDrive* FindProxyDriveForHit(UPrimitiveComponent* HitComponent);
}
