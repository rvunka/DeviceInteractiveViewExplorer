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

	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.f;
	TArray<FHitResult> Hits;
	if (!Context.World->LineTraceMultiByChannel(
		Hits,
		WorldOrigin,
		TraceEnd,
		Context.TraceChannel,
		QueryParams)
		|| Hits.IsEmpty())
	{
		return false;
	}

	// Prefer collision pick-proxies (Box/Sphere/Capsule or DIVE.PickProxy) over the shell mesh
	// that usually sits in front of them on Visibility traces.
	const FHitResult* ChosenHit = nullptr;
	const FHitResult* FirstInteractiveHit = nullptr;

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitPrimitive = Hit.GetComponent();
		if (!HitPrimitive || !IsComponentPartOfDeviceHost(HitPrimitive, Context.DeviceHost))
		{
			continue;
		}

		if (!Context.Inspectable->IsPrimitiveInteractive(HitPrimitive))
		{
			continue;
		}

		if (!FirstInteractiveHit)
		{
			FirstInteractiveHit = &Hit;
		}

		if (Context.Inspectable->IsPickProxyPrimitive(HitPrimitive))
		{
			ChosenHit = &Hit;
			break;
		}
	}

	if (!ChosenHit)
	{
		ChosenHit = FirstInteractiveHit;
	}

	if (!ChosenHit)
	{
		return false;
	}

	UPrimitiveComponent* ChosenPrimitive = ChosenHit->GetComponent();
	if (!ChosenPrimitive)
	{
		return false;
	}

	OutHit = *ChosenHit;
	const FName SemanticPartId = Context.Inspectable->ResolveSemanticPartId(ChosenPrimitive);
	OutTarget = FDIVEFocusTarget::FromPrimitive(ChosenPrimitive, SemanticPartId);
	return true;
}
}
