// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEDeviceAction.h"
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
 * Bind `UDIVEProxyDriveForwardAction` as an Interact catalog primary / menu continuous
 * action to forward the gesture onto this interface. Physical mode is pawn GRIP grab only.
 */
class DIVECORE_API IDIVEProxyDrive
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool CanProxyDrive(const FDIVEProxyDriveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool BeginProxyDrive(const FDIVEProxyDriveContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void ApplyProxyDriveDelta(FVector2D ScreenDelta);

	/**
	 * Per-frame Interact update (cursor, ray, view). Default forwards ScreenDelta to ApplyProxyDriveDelta.
	 * Override when the device needs PickRayDir / ViewLocation (zero ScreenDelta must not drop the frame).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void ApplyProxyDriveUpdate(const FDIVEInteractionUpdate& Update);
	virtual void ApplyProxyDriveUpdate_Implementation(const FDIVEInteractionUpdate& Update)
	{
		if (UObject* Self = Cast<UObject>(this))
		{
			Execute_ApplyProxyDriveDelta(Self, Update.ScreenDelta);
		}
	}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void EndProxyDrive(bool bCommit);

	/** Optional HUD feed. Return false when this drive has no normalized value. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool GetProxyDriveNormalizedValue(UPARAM(ref) float& OutNormalizedValue) const;
};
