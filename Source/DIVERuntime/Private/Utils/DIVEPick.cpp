// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEPick.h"

#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace DIVEPick
{
namespace
{
bool IsComponentPartOfDeviceHost(const USceneComponent* Component, const AActor* DeviceHost)
{
	if (!Component || !DeviceHost)
	{
		return false;
	}

	if (const AActor* Owner = Component->GetOwner())
	{
		if (Owner == DeviceHost || Owner->IsAttachedTo(DeviceHost))
		{
			return true;
		}
	}

	for (const USceneComponent* Current = Component; Current; Current = Current->GetAttachParent())
	{
		if (Current->GetOwner() == DeviceHost)
		{
			return true;
		}
	}

	return false;
}
} // namespace

bool PickAtScreenPosition(
	const FSessionPickContext& Context,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FHitResult& OutHit,
	FDIVEFocusTarget& OutTarget)
{
	OutTarget = FDIVEFocusTarget::MakeDeviceRoot();
	OutHit = FHitResult();

	if (!Context.World || !Context.DeviceHost || !Context.Inspectable || !PlayerController)
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(PlayerController, ScreenPosition, WorldOrigin, WorldDirection))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DIVEPick), false);
	if (Context.IgnoredActor)
	{
		QueryParams.AddIgnoredActor(Context.IgnoredActor);
	}

	if (!Context.World->LineTraceSingleByChannel(
		OutHit,
		WorldOrigin,
		WorldOrigin + WorldDirection * 100000.f,
		Context.TraceChannel,
		QueryParams))
	{
		return false;
	}

	UPrimitiveComponent* HitPrimitive = OutHit.GetComponent();
	if (!HitPrimitive || !IsComponentPartOfDeviceHost(HitPrimitive, Context.DeviceHost))
	{
		return false;
	}

	if (!Context.Inspectable->IsPrimitiveInteractive(HitPrimitive))
	{
		return false;
	}

	const FName SemanticPartId = Context.Inspectable->ResolveSemanticPartId(HitPrimitive);
	OutTarget = FDIVEFocusTarget::FromPrimitive(HitPrimitive, SemanticPartId);
	return true;
}
}
