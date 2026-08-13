// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceScan.h"

#include "DIVEActionBinding.h"
#include "DIVEActionBindingValidation.h"
#include "DIVEAnchorComponent.h"
#include "DIVEInspectableComponent.h"
#include "Misc/DataValidation.h"

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

	TMap<FName, UDIVEAnchorComponent*> PartIdOwners;
	for (UDIVEAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}

		const FName ResolvedPartId = Anchor->GetResolvedPartId();
		if (ResolvedPartId.IsNone())
		{
			AddError(Report, FString::Printf(TEXT("Anchor '%s' resolves to an empty PartId."), *GetNameSafe(Anchor)));
			continue;
		}

		if (UDIVEAnchorComponent** Existing = PartIdOwners.Find(ResolvedPartId))
		{
			AddError(Report, FString::Printf(
				TEXT("Duplicate PartId '%s' on anchors '%s' and '%s'."),
				*ResolvedPartId.ToString(),
				*GetNameSafe(*Existing),
				*GetNameSafe(Anchor)));
		}
		else
		{
			PartIdOwners.Add(ResolvedPartId, Anchor);
		}
	}

	if (!Inspectable->DefaultStartFocusId.IsNone())
	{
		FDIVEFocusTarget Resolved;
		if (!Inspectable->TryResolveStartFocusTarget(Inspectable->DefaultStartFocusId, Resolved))
		{
			AddWarning(Report, FString::Printf(
				TEXT("DefaultStartFocusId '%s' does not match any anchor PartId or pickable mesh."),
				*Inspectable->DefaultStartFocusId.ToString()));
		}
	}

	TArray<FDIVEMenuSection> Sections;
	Inspectable->GatherAuthoredSections(Sections);
	TArray<const FDIVEActionBinding*> Bindings;
	Inspectable->GatherAuthoredBindings(Bindings);

	{
		FDataValidationContext ValidationCtx;
		TSet<FName> KnownSectionIds;
		DIVEActionBindingValidation::ValidateSections(Sections, KnownSectionIds, ValidationCtx);
		DIVEActionBindingValidation::ValidateBindings(Bindings, KnownSectionIds, ValidationCtx);

		for (const FDataValidationContext::FIssue& Issue : ValidationCtx.GetIssues())
		{
			if (Issue.Severity == EMessageSeverity::Error)
			{
				AddError(Report, Issue.Message.ToString());
			}
			else if (Issue.Severity == EMessageSeverity::Warning)
			{
				AddWarning(Report, Issue.Message.ToString());
			}
		}
	}

	{
		FDataValidationContext AuthoringCtx;
		Inspectable->AppendDeviceAuthoringValidation(AuthoringCtx);
		for (const FDataValidationContext::FIssue& Issue : AuthoringCtx.GetIssues())
		{
			if (Issue.Severity == EMessageSeverity::Error)
			{
				AddError(Report, Issue.Message.ToString());
			}
			else if (Issue.Severity == EMessageSeverity::Warning)
			{
				AddWarning(Report, Issue.Message.ToString());
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
	}

	AddInfo(Report, FString::Printf(
		TEXT("Actions: catalog=%s componentBindings=%d authoredBindings=%d sections=%d"),
		Inspectable->ActionCatalog ? *GetNameSafe(Inspectable->ActionCatalog) : TEXT("<none>"),
		Inspectable->Bindings.Num(),
		Bindings.Num(),
		Sections.Num()));

	return Report;
}
