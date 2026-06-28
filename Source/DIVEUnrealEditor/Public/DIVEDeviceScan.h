// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UDIVEInspectableComponent;

struct FDIVEDeviceScanMessage
{
	FString Severity;
	FString Text;
};

struct FDIVEDeviceScanReport
{
	FString DeviceName;
	TArray<FDIVEDeviceScanMessage> Messages;
	int32 AnchorCount = 0;

	bool HasErrors() const;
	FString ToLogString() const;
};

namespace DIVEDeviceScan
{
DIVEUNREALEDITOR_API FDIVEDeviceScanReport ScanActor(AActor* DeviceActor);
}
