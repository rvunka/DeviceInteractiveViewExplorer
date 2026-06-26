// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "DIVETypes.h"

class AActor;
class APlayerController;
class UDIVEInspectableComponent;
class UWorld;

namespace DIVEPick
{
struct FSessionPickContext
{
	UWorld* World = nullptr;
	AActor* DeviceHost = nullptr;
	UDIVEInspectableComponent* Inspectable = nullptr;
	AActor* IgnoredActor = nullptr;
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

DIVERUNTIME_API bool IsComponentPartOfDeviceHost(const USceneComponent* Component, const AActor* DeviceHost);

DIVERUNTIME_API bool ResolveFocusAtScreenPosition(
	const FSessionPickContext& Context,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FDIVEFocusTarget& OutTarget);
}
