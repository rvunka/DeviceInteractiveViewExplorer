// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEPlayerQuery.h"

#include "GameFramework/PlayerController.h"

namespace DIVEPlayerQuery
{
APlayerController* FindLocalPlayerController(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			if (PlayerController->IsLocalController())
			{
				return PlayerController;
			}
		}
	}

	return nullptr;
}
}
