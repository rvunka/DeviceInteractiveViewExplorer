// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionIsolationOps.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "DIVESessionSubsystem.h"
#include "Session/DIVESessionPickOps.h"

void FDIVESessionIsolationOps::ClearIsolation(UDIVESessionSubsystem& Session)
{
	if (!Session.bIsolationActive)
	{
		return;
	}

	FDIVESessionPickOps::ClearPickHover(Session);

	for (const UDIVESessionSubsystem::FIsolatedPrimitiveRecord& Record : Session.IsolatedHiddenPrimitives)
	{
		if (UPrimitiveComponent* Primitive = Record.Primitive.Get())
		{
			Primitive->SetHiddenInGame(Record.bWasHiddenInGame);
		}
	}

	Session.IsolatedHiddenPrimitives.Reset();
	Session.bIsolationActive = false;
	Session.IsolationTarget = FDIVEFocusTarget::MakeDeviceRoot();
}

bool FDIVESessionIsolationOps::ApplyIsolationForTarget(UDIVESessionSubsystem& Session, const FDIVEFocusTarget& Target)
{
	if (!Session.IsSessionActive() || Target.Kind == EDIVEFocusKind::DeviceRoot || !Target.IsValidFocus())
	{
		return false;
	}

	FDIVESessionPickOps::ClearPickHover(Session);

	TArray<UPrimitiveComponent*> VisiblePrimitives;
	CollectIsolationVisiblePrimitives(Session, Target, VisiblePrimitives);
	if (VisiblePrimitives.IsEmpty())
	{
		return false;
	}

	TArray<UPrimitiveComponent*> DevicePrimitives;
	DIVE::CollectDevicePrimitives(Session.ActiveDeviceHost.Get(), DevicePrimitives);

	TSet<UPrimitiveComponent*> VisibleSet(VisiblePrimitives);
	TSet<UPrimitiveComponent*> ShouldHide;
	for (UPrimitiveComponent* Primitive : DevicePrimitives)
	{
		if (Primitive && !VisibleSet.Contains(Primitive))
		{
			ShouldHide.Add(Primitive);
		}
	}

	TMap<UPrimitiveComponent*, bool> PreviouslyHiddenWasState;
	for (const UDIVESessionSubsystem::FIsolatedPrimitiveRecord& Record : Session.IsolatedHiddenPrimitives)
	{
		if (UPrimitiveComponent* Primitive = Record.Primitive.Get())
		{
			PreviouslyHiddenWasState.Add(Primitive, Record.bWasHiddenInGame);
			if (!ShouldHide.Contains(Primitive))
			{
				Primitive->SetHiddenInGame(Record.bWasHiddenInGame);
			}
		}
	}

	Session.IsolatedHiddenPrimitives.Reset();
	for (UPrimitiveComponent* Primitive : ShouldHide)
	{
		bool bWasHiddenInGame = Primitive->bHiddenInGame;
		if (const bool* Previous = PreviouslyHiddenWasState.Find(Primitive))
		{
			bWasHiddenInGame = *Previous;
		}
		else
		{
			Primitive->SetHiddenInGame(true);
		}

		UDIVESessionSubsystem::FIsolatedPrimitiveRecord Record;
		Record.Primitive = Primitive;
		Record.bWasHiddenInGame = bWasHiddenInGame;
		Session.IsolatedHiddenPrimitives.Add(Record);
	}

	Session.bIsolationActive = !Session.IsolatedHiddenPrimitives.IsEmpty();
	if (Session.bIsolationActive)
	{
		Session.IsolationTarget = Target;
	}
	else
	{
		Session.IsolationTarget = FDIVEFocusTarget::MakeDeviceRoot();
	}

	return Session.bIsolationActive;
}

void FDIVESessionIsolationOps::CollectIsolationVisiblePrimitives(
	const UDIVESessionSubsystem& Session,
	const FDIVEFocusTarget& Target,
	TArray<UPrimitiveComponent*>& OutVisible)
{
	AActor* DeviceHost = Session.ActiveDeviceHost.Get();
	if (!DeviceHost)
	{
		return;
	}

	auto AddAncestorPrimitives = [DeviceHost, &OutVisible](USceneComponent* StartComponent)
	{
		for (USceneComponent* Current = StartComponent; Current; Current = Current->GetAttachParent())
		{
			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Current))
			{
				OutVisible.AddUnique(Primitive);
			}

			if (Current->GetOwner() == DeviceHost && Current == DeviceHost->GetRootComponent())
			{
				break;
			}
		}
	};

	if (Target.Kind == EDIVEFocusKind::Primitive && Target.Primitive)
	{
		UPrimitiveComponent* Primitive = Target.Primitive.Get();
		AddAncestorPrimitives(Primitive);
		DIVE::CollectAttachedPrimitives(Primitive, OutVisible);

		const UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
		if (Inspectable && Inspectable->IsPickProxyPrimitive(Primitive))
		{
			TArray<UPrimitiveComponent*> DevicePrimitives;
			DIVE::CollectDevicePrimitives(DeviceHost, DevicePrimitives);
			for (UPrimitiveComponent* DevicePrimitive : DevicePrimitives)
			{
				if (Cast<UMeshComponent>(DevicePrimitive))
				{
					OutVisible.AddUnique(DevicePrimitive);
				}
			}
		}
	}
	else if (Target.Kind == EDIVEFocusKind::Anchor && Target.Anchor)
	{
		DIVE::CollectAttachedPrimitives(Target.Anchor.Get(), OutVisible);
		AddAncestorPrimitives(Target.Anchor.Get());
	}
}
