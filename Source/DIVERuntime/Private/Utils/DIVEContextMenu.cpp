// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEContextMenu.h"

#include "DIVEConvention.h"
#include "DIVESessionSubsystem.h"

namespace DIVEContextMenu
{
void BuildBuiltInEntries(
	const UDIVESessionSubsystem* Subsystem,
	const FDIVEFocusTarget& PickTarget,
	bool bHasValidPick,
	TArray<FDIVEContextMenuEntry>& InOutEntries)
{
	FDIVEContextMenuEntry FocusEntry;
	FocusEntry.ActionId = DIVE::kContextFocus;
	FocusEntry.DisplayName = NSLOCTEXT("DIVE", "ContextMenuFocus", "Focus here");
	FocusEntry.bEnabled = bHasValidPick && PickTarget.IsValidFocus();
	InOutEntries.Add(FocusEntry);

	FDIVEContextMenuEntry IsolateEntry;
	IsolateEntry.ActionId = DIVE::kContextIsolate;
	IsolateEntry.DisplayName = NSLOCTEXT("DIVE", "ContextMenuIsolate", "Isolate");
	IsolateEntry.bEnabled = Subsystem && Subsystem->IsSessionActive() && Subsystem->GetFocusedTarget().IsValidFocus();
	InOutEntries.Add(IsolateEntry);

	if (Subsystem && Subsystem->CanNavigateBack())
	{
		FDIVEContextMenuEntry BackEntry;
		BackEntry.ActionId = DIVE::kContextBack;
		BackEntry.DisplayName = NSLOCTEXT("DIVE", "ContextMenuBack", "Back");
		BackEntry.bEnabled = true;
		InOutEntries.Add(BackEntry);
	}
}
}
