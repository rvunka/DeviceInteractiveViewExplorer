// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceScan.h"

#include "DIVEAnchorComponent.h"
#include "DIVEConvention.h"
#include "DIVEDeviceActionHandler.h"
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

	if (!Inspectable->PickContextMenuByComponent.IsEmpty()
		&& !DeviceActor->Implements<UDIVEDeviceActionHandler>())
	{
		AddWarning(Report, TEXT("PickContextMenuByComponent is non-empty but the actor does not implement IDIVEDeviceActionHandler."));
	}

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

	TSet<FName> QualifiedActionIds;
	for (const TPair<FName, FDIVEPickContextMenuActionList>& ComponentEntry : Inspectable->PickContextMenuByComponent)
	{
		const FName ComponentName = ComponentEntry.Key;
		if (ComponentName.IsNone())
		{
			AddWarning(Report, TEXT("PickContextMenuByComponent has an entry with an empty component name key."));
			continue;
		}

		if (Inspectable->PickInteractionExclusions.Contains(ComponentName))
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickContextMenuByComponent key '%s' is listed in PickInteractionExclusions and will never receive pick interaction."),
				*ComponentName.ToString()));
		}

		for (const FDIVEPickContextMenuAction& Action : ComponentEntry.Value.Actions)
		{
			if (Action.ActionId.IsNone())
			{
				AddError(Report, FString::Printf(
					TEXT("PickContextMenuByComponent '%s' has an entry with empty ActionId."),
					*ComponentName.ToString()));
				continue;
			}

			if (DIVE::IsReservedContextMenuActionId(Action.ActionId))
			{
				AddError(Report, FString::Printf(
					TEXT("PickContextMenuByComponent ActionId '%s' is reserved by DIVE built-in menu rows."),
					*Action.ActionId.ToString()));
			}

			const FName QualifiedActionId = DIVE::MakeQualifiedPickContextMenuActionId(ComponentName, Action.ActionId);
			if (QualifiedActionIds.Contains(QualifiedActionId))
			{
				AddError(Report, FString::Printf(
					TEXT("Duplicate qualified ActionId '%s' in PickContextMenuByComponent."),
					*QualifiedActionId.ToString()));
			}
			else
			{
				QualifiedActionIds.Add(QualifiedActionId);
			}
		}

		if (ComponentEntry.Value.Actions.IsEmpty())
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickContextMenuByComponent '%s' has no actions."),
				*ComponentName.ToString()));
		}

		FDIVEFocusTarget ResolvedCatalogTarget;
		if (!Inspectable->TryResolveStartFocusTarget(ComponentName, ResolvedCatalogTarget))
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickContextMenuByComponent key '%s' does not match any anchor PartId or pickable mesh component."),
				*ComponentName.ToString()));
		}

		if (!ComponentEntry.Value.PrimaryActionId.IsNone())
		{
			bool bFoundPrimary = false;
			for (const FDIVEPickContextMenuAction& Action : ComponentEntry.Value.Actions)
			{
				if (Action.ActionId == ComponentEntry.Value.PrimaryActionId)
				{
					bFoundPrimary = true;
					if (!Action.bEnabled)
					{
						AddWarning(Report, FString::Printf(
							TEXT("PickContextMenuByComponent '%s' PrimaryActionId '%s' points to a disabled action."),
							*ComponentName.ToString(),
							*ComponentEntry.Value.PrimaryActionId.ToString()));
					}
					break;
				}
			}

			if (!bFoundPrimary)
			{
				AddError(Report, FString::Printf(
					TEXT("PickContextMenuByComponent '%s' PrimaryActionId '%s' does not match any resolved ActionId."),
					*ComponentName.ToString(),
					*ComponentEntry.Value.PrimaryActionId.ToString()));
			}
		}
	}

	for (const FName& ExcludedKey : Inspectable->PickInteractionExclusions)
	{
		if (ExcludedKey.IsNone())
		{
			AddWarning(Report, TEXT("PickInteractionExclusions contains an empty key."));
			continue;
		}

		FDIVEFocusTarget ResolvedTarget;
		if (!Inspectable->TryResolveStartFocusTarget(ExcludedKey, ResolvedTarget))
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickInteractionExclusions key '%s' does not match any anchor PartId or pickable mesh component."),
				*ExcludedKey.ToString()));
		}

		if (Inspectable->PickContextMenuByComponent.Contains(ExcludedKey))
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickInteractionExclusions key '%s' also has a PickContextMenuByComponent entry; pick interaction will never reach it."),
				*ExcludedKey.ToString()));
		}

		if (Inspectable->PickHoverOverlayByComponent.Contains(ExcludedKey))
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickInteractionExclusions key '%s' also has a PickHoverOverlayByComponent entry; hover will never apply."),
				*ExcludedKey.ToString()));
		}
	}

	for (const TPair<FName, TSoftObjectPtr<UMaterialInterface>>& OverlayEntry : Inspectable->PickHoverOverlayByComponent)
	{
		const FName ComponentName = OverlayEntry.Key;
		if (ComponentName.IsNone())
		{
			AddWarning(Report, TEXT("PickHoverOverlayByComponent has an entry with an empty key."));
			continue;
		}

		FDIVEFocusTarget ResolvedTarget;
		if (!Inspectable->TryResolveStartFocusTarget(ComponentName, ResolvedTarget))
		{
			AddWarning(Report, FString::Printf(
				TEXT("PickHoverOverlayByComponent key '%s' does not match any anchor PartId or pickable mesh component."),
				*ComponentName.ToString()));
		}
	}

	return Report;
}
