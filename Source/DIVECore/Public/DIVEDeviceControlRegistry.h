// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "DIVEDeviceControlRegistry.generated.h"

class UPrimitiveComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class UDIVEDeviceControlRegistry : public UInterface
{
	GENERATED_BODY()
};

class DIVECORE_API IDIVEDeviceControlRegistry
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|DeviceControl")
	UObject* ResolveProxyDriveObject(UPrimitiveComponent* HitComponent);
};
