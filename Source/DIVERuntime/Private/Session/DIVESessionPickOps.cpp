// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionPickOps.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DIVECameraRig.h"
#include "DIVEInspectableComponent.h"
#include "DIVESessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Utils/DIVEPick.h"

namespace
{
constexpr float PickHoverScreenEpsilonSq = 4.0f;

void SetMeshOverlayMaterial(UPrimitiveComponent* Primitive, UMaterialInterface* OverlayMaterial)
{
	if (UMeshComponent* Mesh = Cast<UMeshComponent>(Primitive))
	{
		Mesh->SetOverlayMaterial(OverlayMaterial);
	}
}
}

bool FDIVESessionPickOps::ResolvePickAtScreenPositionWithHit(
	const UDIVESessionSubsystem& Session,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FDIVEFocusTarget& OutPickTarget,
	FHitResult& OutHit)
{
	OutPickTarget = FDIVEFocusTarget::MakeDeviceRoot();
	OutHit = FHitResult();

	if (!Session.IsSessionActive() || !PlayerController || !Session.GetWorld())
	{
		return false;
	}

	AActor* DeviceHost = Session.ActiveDeviceHost.Get();
	UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
	if (!DeviceHost || !Inspectable)
	{
		return false;
	}

	DIVEPick::FSessionPickContext Context;
	Context.World = Session.GetWorld();
	Context.DeviceHost = DeviceHost;
	Context.Inspectable = Inspectable;
	Context.IgnoredActor = Cast<AActor>(Session.ActiveCameraRig.Get());
	Context.TraceChannel = Inspectable->PickTraceChannel;

	return DIVEPick::PickAtScreenPosition(Context, ScreenPosition, PlayerController, OutHit, OutPickTarget);
}

bool FDIVESessionPickOps::ExecutePrimaryActionAtScreenPosition(
	UDIVESessionSubsystem& Session,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!Session.IsSessionActive() || Session.InteractionMode != EDIVESessionInteractionMode::Default || !PlayerController)
	{
		return false;
	}

	FDIVEFocusTarget PickTarget;
	FHitResult UnusedHit;
	if (!ResolvePickAtScreenPositionWithHit(Session, ScreenPosition, PlayerController, PickTarget, UnusedHit)
		|| PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
	if (!Inspectable)
	{
		return false;
	}

	FName QualifiedActionId = NAME_None;
	if (!Inspectable->TryResolvePrimaryPickAction(PickTarget, QualifiedActionId))
	{
		return false;
	}

	return Inspectable->NotifyPickContextMenuAction(QualifiedActionId, PickTarget);
}

void FDIVESessionPickOps::UpdatePickHover(
	UDIVESessionSubsystem& Session,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!Session.IsSessionActive()
		|| Session.InteractionMode != EDIVESessionInteractionMode::Default
		|| Session.bContextMenuOpen
		|| !PlayerController)
	{
		ClearPickHover(Session);
		return;
	}

	if (Session.bHasLastPickHoverScreenPosition
		&& Session.PickHoverPrimitive.IsValid()
		&& FVector2D::DistSquared(ScreenPosition, Session.LastPickHoverScreenPosition) <= PickHoverScreenEpsilonSq)
	{
		return;
	}

	Session.LastPickHoverScreenPosition = ScreenPosition;
	Session.bHasLastPickHoverScreenPosition = true;

	FDIVEFocusTarget PickTarget;
	FHitResult UnusedHit;
	if (!ResolvePickAtScreenPositionWithHit(Session, ScreenPosition, PlayerController, PickTarget, UnusedHit)
		|| PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		ClearPickHover(Session);
		return;
	}

	UPrimitiveComponent* Primitive = PickTarget.Primitive.Get();
	UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
	if (!Primitive || !Inspectable || Primitive->bHiddenInGame || !Inspectable->IsPrimitiveInteractive(Primitive))
	{
		ClearPickHover(Session);
		return;
	}

	UMaterialInterface* OverlayMaterial = Inspectable->ResolvePickHoverOverlayMaterial(PickTarget);
	if (!OverlayMaterial)
	{
		ClearPickHover(Session);
		return;
	}

	if (Session.PickHoverPrimitive.Get() == Primitive)
	{
		return;
	}

	ClearPickHover(Session);
	SetMeshOverlayMaterial(Primitive, OverlayMaterial);
	Session.PickHoverPrimitive = Primitive;
}

void FDIVESessionPickOps::ClearPickHover(UDIVESessionSubsystem& Session)
{
	if (UPrimitiveComponent* Primitive = Session.PickHoverPrimitive.Get())
	{
		SetMeshOverlayMaterial(Primitive, nullptr);
	}

	Session.PickHoverPrimitive.Reset();
	Session.bHasLastPickHoverScreenPosition = false;
}
