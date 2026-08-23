// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEProxyDriveTypes.h"
#include "UObject/Interface.h"

#include "DIVEProxyDrive.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UDIVEProxyDrive : public UInterface
{
	GENERATED_BODY()
};

/**
 * Monitor adapter for the **interact** verb on a device-owned control (tier 2).
 * DIVERuntime ships **zero** implementors — host/device modules implement this when a
 * control must live on the device (MESS, VR parity, reuse outside the session).
 * Interact-mode knobs/nuts/sliders today use DIVERuntime continuous drive actions
 * (kinematic mesh transform), not this interface.
 * Session routing today still starts this path from Physical primary (see
 * `TryBeginProxyDriveAtScreenPosition`); target policy moves it to Interact.
 */
class DIVECORE_API IDIVEProxyDrive
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool CanProxyDrive(const FDIVEProxyDriveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool BeginProxyDrive(const FDIVEProxyDriveContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void ApplyProxyDriveDelta(FVector2D ScreenDelta);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void EndProxyDrive(bool bCommit);

	/** Optional HUD feed. Return false when this drive has no normalized value. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool GetProxyDriveNormalizedValue(UPARAM(ref) float& OutNormalizedValue) const;
};
