// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "UObject/Interface.h"

#include "DIVEDeviceActionHandler.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UDIVEDeviceActionHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * Preferred device pick-action dispatch. Implement on the device actor;
 * when HandleDeviceAction returns true, DIVE skips the legacy Handle_{Key}_{ActionId} reflection path.
 */
class DIVECORE_API IDIVEDeviceActionHandler
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|DeviceAction")
	bool HandleDeviceAction(
		FName CatalogKey,
		FName ActionId,
		UPrimitiveComponent* Target,
		bool bActiveBefore);
};
