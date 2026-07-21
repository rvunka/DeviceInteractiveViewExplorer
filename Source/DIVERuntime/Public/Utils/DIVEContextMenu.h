// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVETypes.h"

class AActor;
class UDIVESessionSubsystem;

namespace DIVEContextMenu
{
DIVERUNTIME_API FText FormatActiveLabelSuffix(const FText& BaseLabel, bool bActive);

DIVERUNTIME_API void BuildStandardEntries(
	const UDIVESessionSubsystem* Subsystem,
	const FDIVEFocusTarget& PickTarget,
	bool bHasValidPick,
	bool bIncludeAdminMeshEntries,
	TArray<FDIVEContextMenuEntry>& InOutEntries);

DIVERUNTIME_API void AppendCustomEntries(
	AActor* DeviceHost,
	const UDIVESessionSubsystem* Subsystem,
	const FDIVEFocusTarget& PickTarget,
	TArray<FDIVEContextMenuEntry>& InOutEntries);
}
