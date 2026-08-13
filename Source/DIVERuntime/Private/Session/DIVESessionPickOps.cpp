// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionPickOps.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DIVECameraRig.h"
#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"
#include "DIVESessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
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
	FHitResult PickHit;
	if (!ResolvePickAtScreenPositionWithHit(Session, ScreenPosition, PlayerController, PickTarget, PickHit)
		|| PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
	if (!Inspectable)
	{
		return false;
	}

	UDIVEDeviceAction* PrimaryAction = nullptr;
	FName TargetKey = NAME_None;
	FName BindingId = NAME_None;
	if (!Inspectable->TryResolvePrimaryAction(PickTarget, PrimaryAction, TargetKey, BindingId) || !PrimaryAction)
	{
		return false;
	}

	const FDIVEActionContext Context = Inspectable->MakeActionContext(
		PickTarget,
		TargetKey,
		ScreenPosition,
		PickHit,
		BindingId);

	{
		FDIVEActionWorldScope WorldScope(PrimaryAction, Session.GetWorld());
		if (!PrimaryAction->CanExecute(Context))
		{
			return false;
		}

		if (UDIVEContinuousDeviceAction* Continuous = Cast<UDIVEContinuousDeviceAction>(PrimaryAction))
		{
			return Session.TryBeginContinuousAction(Continuous, Context);
		}

		if (!PrimaryAction->Execute(Context))
		{
			return false;
		}

		Inspectable->NotifyActionExecuted(PrimaryAction, Context);
		return true;
	}
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
	if (!Primitive || !Inspectable || !Inspectable->IsPrimitiveInteractive(Primitive))
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

	UMaterialInterface* PreviousOverlay = nullptr;
	if (UMeshComponent* Mesh = Cast<UMeshComponent>(Primitive))
	{
		PreviousOverlay = Mesh->GetOverlayMaterial();
	}
	SetMeshOverlayMaterial(Primitive, OverlayMaterial);
	Session.PickHoverPrimitive = Primitive;
	Session.PickHoverPreviousOverlay = PreviousOverlay;
}

void FDIVESessionPickOps::ClearPickHover(UDIVESessionSubsystem& Session)
{
	if (UPrimitiveComponent* Primitive = Session.PickHoverPrimitive.Get())
	{
		// Restore whatever overlay material the primitive had before DIVE applied the hover
		// highlight, so we don't destroy visual states owned by the device or other systems.
		UMaterialInterface* Restore = Session.PickHoverPreviousOverlay.Get();
		SetMeshOverlayMaterial(Primitive, Restore);
	}

	Session.PickHoverPrimitive.Reset();
	Session.PickHoverPreviousOverlay.Reset();
	Session.bHasLastPickHoverScreenPosition = false;
}
