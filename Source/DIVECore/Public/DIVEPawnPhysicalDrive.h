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

/**
 * Pawn-side Physical drive backend (e.g. DIVEGRIPBridge).
 * Cursor motion is owned by the backend (cursor-pull / tick) — the session does not push screen deltas.
 */
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

	/** Optional: grab backends that support manual rotate (e.g. GRIP). No-op if unsupported. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalManualRotatePressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalManualRotateReleased();
};
