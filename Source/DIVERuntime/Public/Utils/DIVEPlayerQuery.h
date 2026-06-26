// Copyright (c) 2026. All Rights Reserved.

#pragma once

class APlayerController;
class UWorld;

namespace DIVEPlayerQuery
{
	DIVERUNTIME_API APlayerController* FindLocalPlayerController(UWorld* World);
}
