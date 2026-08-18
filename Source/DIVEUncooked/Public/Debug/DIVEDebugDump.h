// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/** Device dump: DIVE.DumpDevice / DIVE.DumpAll → LogDIVE + Saved/DIVE/Dumps/. */
namespace DIVEDebugDump
{
DIVEUNCOOKED_API FString BuildDeviceDump(AActor* DeviceActor);
DIVEUNCOOKED_API FString DumpDevice(AActor* DeviceActor);
DIVEUNCOOKED_API FString DumpAllInWorld(UWorld* World);
DIVEUNCOOKED_API FString DumpFromConsole(UWorld* World, const TArray<FString>& Args, bool bAll);

DIVEUNCOOKED_API void RegisterConsoleCommands();
DIVEUNCOOKED_API void UnregisterConsoleCommands();

DIVEUNCOOKED_API FString GetLastDumpFilePath();
}
