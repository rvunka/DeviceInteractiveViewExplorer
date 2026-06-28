// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceScan.h"

#include "DIVEAnchorComponent.h"
#include "DIVEInspectableComponent.h"

namespace
{
void AddMessage(FDIVEDeviceScanReport& Report, const FString& Severity, const FString& Text)
{
	Report.Messages.Add({Severity, Text});
}

void AddInfo(FDIVEDeviceScanReport& Report, const FString& Text)
{
	AddMessage(Report, TEXT("Info"), Text);
}

void AddWarning(FDIVEDeviceScanReport& Report, const FString& Text)
{
	AddMessage(Report, TEXT("Warning"), Text);
}

void AddError(FDIVEDeviceScanReport& Report, const FString& Text)
{
	AddMessage(Report, TEXT("Error"), Text);
}
}

bool FDIVEDeviceScanReport::HasErrors() const
{
	return Messages.ContainsByPredicate([](const FDIVEDeviceScanMessage& Message)
	{
		return Message.Severity == TEXT("Error");
	});
}

FString FDIVEDeviceScanReport::ToLogString() const
{
	FString Output = FString::Printf(TEXT("DIVE Scan: %s (%d anchors)\n"), *DeviceName, AnchorCount);
	for (const FDIVEDeviceScanMessage& Message : Messages)
	{
		Output += FString::Printf(TEXT("  [%s] %s\n"), *Message.Severity, *Message.Text);
	}

	return Output;
}

FDIVEDeviceScanReport DIVEDeviceScan::ScanActor(AActor* DeviceActor)
{
	FDIVEDeviceScanReport Report;
	if (!DeviceActor)
	{
		AddError(Report, TEXT("No actor selected."));
		return Report;
	}

	Report.DeviceName = DeviceActor->GetName();

	UDIVEInspectableComponent* Inspectable = DeviceActor->FindComponentByClass<UDIVEInspectableComponent>();
	if (!Inspectable)
	{
		AddError(Report, TEXT("Missing UDIVEInspectableComponent."));
		return Report;
	}

	Inspectable->BuildSemanticRegistry();

	TArray<UDIVEAnchorComponent*> Anchors;
	DeviceActor->GetComponents<UDIVEAnchorComponent>(Anchors);
	Report.AnchorCount = Anchors.Num();

	if (Anchors.IsEmpty())
	{
		AddWarning(Report, TEXT("No UDIVEAnchorComponent instances found."));
	}

	TSet<FName> SeenPartIds;
	for (UDIVEAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}

		const FName PartId = Anchor->GetResolvedPartId();
		if (PartId.IsNone())
		{
			AddError(Report, FString::Printf(TEXT("Anchor '%s' has empty PartId."), *Anchor->GetName()));
			continue;
		}

		if (SeenPartIds.Contains(PartId))
		{
			AddError(Report, FString::Printf(TEXT("Duplicate PartId '%s'."), *PartId.ToString()));
		}
		else
		{
			SeenPartIds.Add(PartId);
		}
	}

	if (!Inspectable->DefaultStartFocusId.IsNone())
	{
		FDIVEFocusTarget ResolvedTarget;
		if (!Inspectable->TryResolveStartFocusTarget(Inspectable->DefaultStartFocusId, ResolvedTarget))
		{
			AddWarning(Report, FString::Printf(
				TEXT("DefaultStartFocusId '%s' does not match any anchor PartId or pickable mesh component."),
				*Inspectable->DefaultStartFocusId.ToString()));
		}
	}

	AddInfo(Report, FString::Printf(TEXT("World dim policy: %d"), static_cast<int32>(Inspectable->GetEffectiveWorldDimPolicy())));
	return Report;
}
