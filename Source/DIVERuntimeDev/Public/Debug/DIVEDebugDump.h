// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/** Device dump: DIVE.DumpDevice / DIVE.DumpAll → LogDIVE + Saved/DIVE/Dumps/. */
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
