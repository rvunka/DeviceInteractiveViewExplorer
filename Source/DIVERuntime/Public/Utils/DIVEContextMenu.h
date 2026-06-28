// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVETypes.h"

class UDIVESessionSubsystem;

namespace DIVEContextMenu
{
DIVERUNTIME_API void BuildBuiltInEntries(
	const UDIVESessionSubsystem* Subsystem,
	const FDIVEFocusTarget& PickTarget,
	bool bHasValidPick,
	TArray<FDIVEContextMenuEntry>& InOutEntries);
}
