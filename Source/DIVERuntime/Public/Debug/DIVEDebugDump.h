// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Runtime / editor diagnostic dump for DIVE inspectable devices.
 *
 * Console (PIE / game):
 *   DIVE.DumpDevice [optional name substring]  — active session device, or first/name match
 *   DIVE.DumpAll                              — every actor with UDIVEInspectableComponent
 *
 * Writes Output Log (LogDIVE) and Saved/DIVE/DIVEDump_*.txt
 */
namespace DIVEDebugDump
{
DIVERUNTIME_API FString BuildDeviceDump(AActor* DeviceActor);
DIVERUNTIME_API FString DumpDevice(AActor* DeviceActor);
DIVERUNTIME_API FString DumpAllInWorld(UWorld* World);
DIVERUNTIME_API FString DumpFromConsole(UWorld* World, const TArray<FString>& Args, bool bAll);

DIVERUNTIME_API void RegisterConsoleCommands();
DIVERUNTIME_API void UnregisterConsoleCommands();

DIVERUNTIME_API FString GetLastDumpFilePath();
}
