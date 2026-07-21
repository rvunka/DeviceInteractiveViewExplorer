// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEProxyDriveTypes.h"
#include "UObject/Interface.h"

#include "DIVEPawnPhysicalDrive.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UDIVEPawnPhysicalDrive : public UInterface
{
	GENERATED_BODY()
};

class DIVECORE_API IDIVEPawnPhysicalDrive
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	bool CanBeginPawnPhysicalDrive(const FDIVEProxyDriveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	bool BeginPawnPhysicalDrive(const FDIVEProxyDriveContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void EndPawnPhysicalDrive(bool bCommit);

	/**
	 * Optional: backends that push cursor deltas from the subsystem.
	 * GRIP bridge ignores this and pulls cursor in its own tick — preferred for pawn grab.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void ApplyPawnPhysicalDriveDelta(FVector2D ScreenDelta);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalManualRotatePressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalManualRotateReleased();
};
