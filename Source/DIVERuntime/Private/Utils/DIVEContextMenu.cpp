// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEContextMenu.h"

#include "DIVEInspectableComponent.h"

namespace DIVEContextMenu
{
void BuildEntries(
	UDIVEInspectableComponent* Inspectable,
	const FDIVEFocusTarget& PickTarget,
	bool bHasValidPick,
	TArray<FDIVEContextMenuEntry>& OutEntries)
{
	OutEntries.Reset();

	if (Inspectable && bHasValidPick && PickTarget.Kind == EDIVEFocusKind::Primitive)
	{
		Inspectable->AppendConfiguredContextMenuEntries(PickTarget, OutEntries);
	}
}

} // namespace DIVEContextMenu
