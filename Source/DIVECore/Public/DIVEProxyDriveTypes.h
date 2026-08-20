// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVETypes.h"
#include "Engine/HitResult.h"

#include "DIVEProxyDriveTypes.generated.h"

class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEProxyDriveContext
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
