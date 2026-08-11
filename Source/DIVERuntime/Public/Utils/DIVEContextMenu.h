// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceAction.h"
#include "DIVETypes.h"

class UDIVEInspectableComponent;

namespace DIVEContextMenu
{
DIVERUNTIME_API void BuildEntries(
	UDIVEInspectableComponent* Inspectable,
	const FDIVEFocusTarget& PickTarget,
	bool bHasValidPick,
	TArray<FDIVEContextMenuEntry>& OutEntries);
}
