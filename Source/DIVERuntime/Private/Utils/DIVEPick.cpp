// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEPick.h"

#include "DIVEAnchorComponent.h"
#include "DIVEConvention.h"
#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace DIVEPick
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

bool ResolveFocusAtScreenPosition(
	const FSessionPickContext& Context,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FDIVEFocusTarget& OutTarget)
{
	OutTarget = FDIVEFocusTarget::MakeDeviceRoot();

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

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DIVEPick), false);
	if (Context.IgnoredActor)
	{
		QueryParams.AddIgnoredActor(Context.IgnoredActor);
	}

	if (!Context.World->LineTraceSingleByChannel(
		HitResult,
		WorldOrigin,
		WorldOrigin + WorldDirection * 100000.f,
		Context.TraceChannel,
		QueryParams))
	{
		return false;
	}

	UPrimitiveComponent* HitPrimitive = HitResult.GetComponent();
	if (!HitPrimitive || !IsComponentPartOfDeviceHost(HitPrimitive, Context.DeviceHost))
	{
		return false;
	}

	const bool bIsAnchorMarker = HitPrimitive->ComponentHasTag(DIVE::kAnchorMarkerTag);
	if (!bIsAnchorMarker && !Context.Inspectable->IsPrimitivePickable(HitPrimitive))
	{
		return false;
	}

	if (bIsAnchorMarker)
	{
		if (UDIVEAnchorComponent* Anchor = DIVE::FindAncestorComponent<UDIVEAnchorComponent>(HitPrimitive))
		{
			OutTarget = FDIVEFocusTarget::FromAnchor(Anchor, Anchor->GetResolvedPartId());
			return true;
		}

		return false;
	}

	const FName SemanticPartId = Context.Inspectable->ResolveSemanticPartId(HitPrimitive);
	OutTarget = FDIVEFocusTarget::FromPrimitive(HitPrimitive, SemanticPartId);
	return true;
}
}
