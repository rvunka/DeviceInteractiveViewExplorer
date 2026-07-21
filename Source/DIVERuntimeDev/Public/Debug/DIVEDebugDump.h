// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Dev / editor diagnostic dump for DIVE inspectable devices (DIVERuntimeDev).
 *
 * Console (PIE / non-Shipping with Dev module):
 *   DIVE.DumpDevice [optional name substring]  — active session device, or first/name match
 *   DIVE.DumpAll                              — every actor with UDIVEInspectableComponent
 *
 * Writes Output Log (LogDIVE) and Saved/DIVE/Dumps/
 */
namespace DIVEDebugDump
{
DIVERUNTIMEDEV_API FString BuildDeviceDump(AActor* DeviceActor);
DIVERUNTIMEDEV_API FString DumpDevice(AActor* DeviceActor);
DIVERUNTIMEDEV_API FString DumpAllInWorld(UWorld* World);
DIVERUNTIMEDEV_API FString DumpFromConsole(UWorld* World, const TArray<FString>& Args, bool bAll);

DIVERUNTIMEDEV_API void RegisterConsoleCommands();
DIVERUNTIMEDEV_API void UnregisterConsoleCommands();

DIVERUNTIMEDEV_API FString GetLastDumpFilePath();
}
