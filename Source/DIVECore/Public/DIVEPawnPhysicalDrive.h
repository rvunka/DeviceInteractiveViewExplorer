// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVETypes.h"
#include "Engine/HitResult.h"
#include "UObject/Interface.h"

#include "DIVEPawnPhysicalDrive.generated.h"

class UPrimitiveComponent;

/** Pick blob for pawn Physical grab. */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPawnPhysicalDriveContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FDIVEFocusTarget FocusTarget;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FHitResult PickHit;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector ViewLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector PickRayDir = FVector::ForwardVector;
};

UINTERFACE(MinimalAPI, BlueprintType)
class UDIVEPawnPhysicalDrive : public UInterface
{
	GENERATED_BODY()
};

/**
 * Pawn-side Physical grab backend (e.g. DIVE GRIP Physical Drive on DIVE Player).
 * Cursor motion is owned by the backend — the session does not push screen deltas.
 */
class DIVECORE_API IDIVEPawnPhysicalDrive
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	bool CanBeginPawnPhysicalDrive(const FDIVEPawnPhysicalDriveContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	bool BeginPawnPhysicalDrive(const FDIVEPawnPhysicalDriveContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void EndPawnPhysicalDrive(bool bCommit);

	/** Optional: grab backends that support manual rotate (e.g. GRIP). No-op if unsupported. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalManualRotatePressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalManualRotateReleased();

	/** Optional: wheel while pawn physical drive is active (e.g. GRIP hold distance). No-op if unsupported. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|PawnPhysicalDrive")
	void HandlePawnPhysicalGrabHoldDistanceScroll(float WheelDelta);
};
