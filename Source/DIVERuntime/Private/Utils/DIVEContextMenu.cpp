// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEContextMenu.h"

#include "Components/PrimitiveComponent.h"
#include "DIVEConvention.h"
#include "DIVEInspectableComponent.h"
#include "DIVESessionSubsystem.h"

namespace DIVEContextMenu
{
FText FormatActiveLabelSuffix(const FText& BaseLabel, const bool bActive)
{
	if (!bActive)
	{
		return BaseLabel;
	}

	return FText::Format(NSLOCTEXT("DIVE", "ContextMenuActiveSuffix", "{0}*"), BaseLabel);
}

namespace
{
void AppendSeparator(TArray<FDIVEContextMenuEntry>& InOutEntries)
{
	if (!InOutEntries.IsEmpty() && InOutEntries.Last().bIsSeparator)
	{
		return;
	}

	FDIVEContextMenuEntry SeparatorEntry;
	SeparatorEntry.bIsSeparator = true;
	InOutEntries.Add(SeparatorEntry);
}

void AppendStandardMeshEntries(const FDIVEFocusTarget& PickTarget, TArray<FDIVEContextMenuEntry>& InOutEntries)
{
#if UE_BUILD_SHIPPING
	(void)PickTarget;
	(void)InOutEntries;
#else
	UPrimitiveComponent* Primitive = PickTarget.Primitive.Get();
	if (PickTarget.Kind != EDIVEFocusKind::Primitive || !Primitive)
	{
		return;
	}

	AppendSeparator(InOutEntries);

	FDIVEContextMenuEntry PhysicsEntry;
	PhysicsEntry.ActionId = DIVE::kContextToggleMeshPhysics;
	PhysicsEntry.DisplayName = FormatActiveLabelSuffix(
		NSLOCTEXT("DIVE", "ContextMenuSimulatePhysics", "Simulate Physics"),
		Primitive->IsSimulatingPhysics());
	PhysicsEntry.bEnabled = true;
	InOutEntries.Add(PhysicsEntry);

	FDIVEContextMenuEntry DeleteEntry;
	DeleteEntry.ActionId = DIVE::kContextDeleteMesh;
	DeleteEntry.DisplayName = NSLOCTEXT("DIVE", "ContextMenuDeleteMesh", "Delete Mesh");
	DeleteEntry.bEnabled = true;
	InOutEntries.Add(DeleteEntry);
#endif
}
} // namespace

void BuildStandardEntries(
	const UDIVESessionSubsystem* Subsystem,
	const FDIVEFocusTarget& PickTarget,
	bool bHasValidPick,
	bool bIncludeAdminMeshEntries,
	TArray<FDIVEContextMenuEntry>& InOutEntries)
{
	const bool bIsMeshPick = bHasValidPick
		&& PickTarget.IsValidFocus()
		&& PickTarget.Kind == EDIVEFocusKind::Primitive;

	FDIVEContextMenuEntry FocusEntry;
	FocusEntry.ActionId = DIVE::kContextFocus;
	FocusEntry.DisplayName = NSLOCTEXT("DIVE", "ContextMenuFocus", "Focus");
	FocusEntry.bEnabled = bIsMeshPick;
	InOutEntries.Add(FocusEntry);

	const bool bIsolateActive = Subsystem && Subsystem->IsIsolationActiveForTarget(PickTarget);

	FDIVEContextMenuEntry IsolateEntry;
	IsolateEntry.ActionId = DIVE::kContextIsolate;
	IsolateEntry.DisplayName = FormatActiveLabelSuffix(
		NSLOCTEXT("DIVE", "ContextMenuIsolate", "Isolate"),
		bIsolateActive);
	IsolateEntry.bEnabled = Subsystem && Subsystem->IsSessionActive() && bIsMeshPick;
	InOutEntries.Add(IsolateEntry);

	if (bIncludeAdminMeshEntries)
	{
		AppendStandardMeshEntries(PickTarget, InOutEntries);
	}
}

void AppendCustomEntries(
	AActor* DeviceHost,
	const UDIVESessionSubsystem* Subsystem,
	const FDIVEFocusTarget& PickTarget,
	TArray<FDIVEContextMenuEntry>& InOutEntries)
{
	if (!DeviceHost || PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return;
	}

	UDIVEInspectableComponent* Inspectable = Subsystem ? Subsystem->GetActiveInspectable() : nullptr;
	if (!Inspectable)
	{
		return;
	}

	const int32 EntryCountBefore = InOutEntries.Num();
	Inspectable->AppendConfiguredPickContextMenuEntries(PickTarget, InOutEntries);

	if (InOutEntries.Num() > EntryCountBefore)
	{
		FDIVEContextMenuEntry SeparatorEntry;
		SeparatorEntry.bIsSeparator = true;
		InOutEntries.Insert(SeparatorEntry, EntryCountBefore);
	}
}

}
